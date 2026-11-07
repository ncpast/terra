#ifndef ROUTEPLANNER_H
#define ROUTEPLANNER_H

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>
#include <algorithm>
#include <cmath>
#include <vector>
#include "geogrid.h"
#include "router.h"

#include <QDebug>
#include <QDir>
#include <QDateTime>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <limits>
#include <functional>
#include <memory>
#include <QPointer>
#include <QThread>
#include "osmcost.h"
#include "corridor.h"

class RoutePlanner : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(double progress READ progress NOTIFY progressChanged)   // 0..1 while busy
public:
    explicit RoutePlanner(QObject *parent = nullptr) : QObject(parent) {}
    ~RoutePlanner() override { if (m_thread) m_thread->wait(); }   // let a running plan finish

    bool busy() const { return m_busy; }
    double progress() const { return m_progress; }

    // Plans on a worker thread so the UI keeps drawing; progress is reported through `progress`,
    // the result arrives in planned(routeId, result) on the UI thread. One plan at a time
    // (the feature cache is not shared between threads): returns false while busy.
    Q_INVOKABLE bool planAsync(int routeId, const QVariantList &points) {
        if (m_busy) return false;
        setBusy(true);
        setProgress(0);
        QThread *t = QThread::create([this, routeId, points] {
            const QVariantMap res = planImpl(points, 5.0, 1.0, 30.0, [this](double p) {
                QMetaObject::invokeMethod(this, [this, p] { setProgress(p); }, Qt::QueuedConnection);
            });
            QMetaObject::invokeMethod(this, [this, routeId, res] {
                setProgress(1.0);
                setBusy(false);
                emit planned(routeId, res);
            }, Qt::QueuedConnection);
        });
        connect(t, &QThread::finished, t, &QObject::deleteLater);
        m_thread = t;
        t->start();
        return true;
    }

    // Same planning, blocking the caller (tests, tools). points: [{lat, lon}, ...] in route order.
    // Two passes per leg: A* on a coarse grid (cellMetres, grows only past kMaxCells), then
    // A* again at fineMetres inside a corridor around the coarse path (see corridor.h).
    // fineMetres <= 0 skips the refinement.
    Q_INVOKABLE QVariantMap plan(const QVariantList &points, double cellMetres = 5.0,
                                 double fineMetres = 1.0, double corridorMetres = 30.0) const {
        if (m_busy) return QVariantMap{ { "ok", false }, { "error", "PLANNER BUSY" } };
        return planImpl(points, cellMetres, fineMetres, corridorMetres, {});
    }

signals:
    void busyChanged();
    void progressChanged();
    void planned(int routeId, const QVariantMap &result);

private:
    QVariantMap planImpl(const QVariantList &points, double cellMetres, double fineMetres,
                         double corridorMetres, const std::function<void(double)> &onProgress) const {
        // progress never goes backwards (a leg redone on the conservative grid restarts its pieces)
        // and is only sent in steps of at least 1%
        double shown = 0;
        auto report = [&](double p) {
            if (!onProgress || p < shown + 0.01) return;
            shown = p;
            onProgress(p);
        };

        QVariantMap out;
        out["ok"] = false;

        if (points.size() < 2) {
            out["error"] = "A ROUTE NEEDS AT LEAST 2 CHECKPOINTS";
            return out;
        }

        // bounding box of the checkpoints
        double south = 90, north = -90, west = 180, east = -180;
        for (const QVariant &p : points) {
            const QVariantMap m = p.toMap();
            const double lat = m.value("lat").toDouble();
            const double lon = m.value("lon").toDouble();
            south = std::min(south, lat);  north = std::max(north, lat);
            west  = std::min(west, lon);   east  = std::max(east, lon);
        }

        // add a margin so the path has room to go around obstacles
        const double cosLat = std::cos((south + north) / 2 * M_PI / 180);
        const double spanM = std::max((north - south) * 111320.0, (east - west) * 111320.0 * cosLat);
        const double marginM = std::max(500.0, spanM * 0.5);
        const double dLat = marginM / 111320.0;
        const double dLon = marginM / (111320.0 * cosLat);

        // grow the cells if the area is too big for the finest size
        const double areaM2 = ((north - south) * 111320.0 + 2 * marginM)
                              * ((east - west) * 111320.0 * cosLat + 2 * marginM);
        cellMetres = std::max(cellMetres, std::ceil(std::sqrt(areaM2 / kMaxCells)));
        if (cellMetres > 10.0)
            qWarning() << "routing with" << cellMetres << "m cells: small buildings may be missed";

        GeoGrid gg(south - dLat, west - dLon, north + dLat, east + dLon, cellMetres);
        if (qint64(gg.width()) * gg.height() > kMaxCells) {
            out["error"] = "AREA TOO LARGE";
            return out;
        }

        // terrain costs from OSM data (temporary: fixed file, later the working space data)
        QString loadError;
        const OsmCost::Features *data = features(QDir::homePath() + "/osm/area.geojson", loadError);
        if (!data) {
            out["error"] = loadError;   // an empty grid would only give straight lines
            return out;
        }
        const OsmCost::Features &feats = *data;
        report(0.05);

        // Outside the extract the grid is empty and every route is a straight line, so refuse.
        // Coverage = the GeoJSON "bbox" member [west, south, east, north], written when the
        // extract is cut (the feature coordinates overhang it: clipped ways are kept whole).
        for (int i = 0; i < points.size(); i++) {
            const QVariantMap m = points[i].toMap();
            if (!feats.covers(m.value("lat").toDouble(), m.value("lon").toDouble())) {
                out["error"] = "CHECKPOINT " + QString::number(i + 1) + " OUTSIDE MAP DATA";
                out["failedCheckpoint"] = i + 1;
                return out;
            }
        }

        // With refinement the coarse grid is optimistic (no outline blocking): it keeps passages
        // narrower than a coarse cell, which the fine pass then checks. Measured on Zakopane:
        // conservative coarse + 1 m costs 26% more than a full 1 m search, optimistic only 6%.
        const bool refine = fineMetres > 0 && fineMetres < cellMetres;
        // cells outside the coverage box have no data: block them so no detour runs through them
        auto blockOutside = [&](GeoGrid &g) {
            for (int y = 0; y < g.height(); y++)
                for (int x = 0; x < g.width(); x++) {
                    double lat, lon;
                    g.toLatLon(Cell{x, y}, lat, lon);
                    if (!feats.covers(lat, lon)) g.grid().setCost(x, y, OsmCost::kBlocked);
                }
        };
        OsmCost::fill(gg, feats, /*outlines=*/ !refine);
        blockOutside(gg);
        report(0.10);

        // conservative coarse grid, built only if an optimistic leg cannot be refined
        std::unique_ptr<GeoGrid> safe;
        auto safeGrid = [&]() -> const GeoGrid & {
            if (!safe) {
                safe = std::make_unique<GeoGrid>(south - dLat, west - dLon, north + dLat, east + dLon, cellMetres);
                OsmCost::fill(*safe, feats);
                blockOutside(*safe);
            }
            return *safe;
        };

        Corridor::Settings st;
        st.fineMetres = fineMetres;
        st.corridorMetres = corridorMetres;
        Corridor::Settings strict = st;
        strict.allowFallback = false;   // never fall back onto an optimistic coarse path
        int pieces = 0, retries = 0, fallbacks = 0, conservativeLegs = 0;

        // one search per leg, legs joined into one line of [lat, lon] vertices
        std::vector<std::pair<double, double>> line;
        const int legs = int(points.size()) - 1;
        for (int i = 0; i + 1 < points.size(); i++) {
            // each leg gets an equal share of 10%..100%: 10% of it for the coarse search, the rest refining
            const double legStart = 0.10 + 0.90 * i / legs, legSpan = 0.90 / legs;
            auto legProgress = [&](double f) { report(legStart + legSpan * (0.1 + 0.9 * f)); };
            const QVariantMap a = points[i].toMap();
            const QVariantMap b = points[i + 1].toMap();
            const double aLat = a.value("lat").toDouble(), aLon = a.value("lon").toDouble();
            const double bLat = b.value("lat").toDouble(), bLon = b.value("lon").toDouble();

            const PathResult r = Router::findPath(gg.grid(), gg.toCell(aLat, aLon), gg.toCell(bLat, bLon));
            if (!r.found) {
                out["error"] = "NO PATH FOR LEG " + QString::number(i + 1);
                out["failedLeg"] = i + 1;
                return out; // no straight-line fallback
            }
            report(legStart + legSpan * 0.1);

            if (refine) {
                Corridor::Result fine =
                    Corridor::refineLeg(feats, gg, r.path, { aLat, aLon }, { bLat, bLon }, strict, legProgress);
                if (!fine.ok) {
                    // the optimistic path squeezes through somewhere the fine grid cannot:
                    // redo the leg on the conservative coarse grid, where a coarse fallback is safe
                    conservativeLegs++;
                    const GeoGrid &cg = safeGrid();
                    const PathResult rs = Router::findPath(cg.grid(), cg.toCell(aLat, aLon), cg.toCell(bLat, bLon));
                    if (!rs.found) {
                        out["error"] = "NO PATH FOR LEG " + QString::number(i + 1);
                        out["failedLeg"] = i + 1;
                        return out;
                    }
                    fine = Corridor::refineLeg(feats, cg, rs.path, { aLat, aLon }, { bLat, bLon }, st, legProgress);
                }
                pieces += fine.pieces; retries += fine.retries; fallbacks += fine.fallbacks;
                // later legs skip their first vertex: it is the last vertex of the previous leg
                line.insert(line.end(), fine.line.begin() + (i == 0 ? 0 : 1), fine.line.end());
                continue;
            }

            const std::vector<Cell> verts = Router::smooth(gg.grid(), r.path);
            // later legs skip their first vertex: it is the last vertex of the previous leg
            for (size_t k = (i == 0 ? 0 : 1); k < verts.size(); k++) {
                double lat, lon;
                gg.toLatLon(verts[k], lat, lon);
                // the ends are the exact checkpoints, not the centres of their cells
                if (k == 0) { lat = aLat; lon = aLon; }
                if (k + 1 == verts.size()) { lat = bLat; lon = bLon; }
                line.push_back({ lat, lon });
            }
        }

        // [lon, lat] pairs for GeoJSON, plus the total length
        QVariantList coords;
        double length = 0;
        for (size_t k = 0; k < line.size(); k++) {
            if (k > 0) length += distanceMetres(line[k - 1].first, line[k - 1].second,
                                                line[k].first, line[k].second);
            coords.append(QVariant(QVariantList{ line[k].second, line[k].first }));
        }

        out["ok"] = true;
        out["coordinates"] = coords;
        out["lengthMeters"] = length;
        out["cellMetres"] = refine ? fineMetres : cellMetres;
        out["coarseCellMetres"] = cellMetres;
        out["pieces"] = pieces;
        out["corridorRetries"] = retries;
        out["coarseFallbacks"] = fallbacks;   // pieces left at coarse precision
        out["conservativeLegs"] = conservativeLegs;
        if (fallbacks) qWarning() << fallbacks << "route pieces kept at" << cellMetres << "m precision";
        return out;
    }

    void setBusy(bool b) { if (m_busy != b) { m_busy = b; emit busyChanged(); } }
    void setProgress(double p) { if (m_progress != p) { m_progress = p; emit progressChanged(); } }

    bool m_busy = false;
    double m_progress = 0;
    QPointer<QThread> m_thread;

    static constexpr qint64 kMaxCells = 4000000;

    // Parsed OSM data, kept between plans; reloaded only when the file changes on disk.
    mutable std::unique_ptr<OsmCost::Features> m_features;
    mutable QString m_featuresPath;
    mutable QDateTime m_featuresModified;

    const OsmCost::Features *features(const QString &path, QString &error) const {
        const QFileInfo info(path);
        if (!info.exists()) { error = "NO MAP DATA"; return nullptr; }
        if (!m_features || m_featuresPath != path || m_featuresModified != info.lastModified()) {
            QFile f(path);
            if (!f.open(QIODevice::ReadOnly)) { error = "NO MAP DATA"; return nullptr; }
            QElapsedTimer t;
            t.start();
            auto parsed = std::make_unique<OsmCost::Features>(OsmCost::parse(QJsonDocument::fromJson(f.readAll())));
            if (!parsed->hasCoverage) { error = "MAP DATA HAS NO COVERAGE BOX"; return nullptr; }
            qInfo() << "loaded" << parsed->obstacles.size() << "obstacles," << parsed->highways.size()
                    << "highways from" << path << "in" << t.elapsed() << "ms";
            m_features = std::move(parsed);
            m_featuresPath = path;
            m_featuresModified = info.lastModified();
        }
        return m_features.get();
    }

    static double distanceMetres(double lat1, double lon1, double lat2, double lon2) {  // haversine
        const double R = 6371000.0;
        const double p1 = lat1 * M_PI / 180, p2 = lat2 * M_PI / 180;
        const double dp = p2 - p1, dl = (lon2 - lon1) * M_PI / 180;
        const double a = std::sin(dp / 2) * std::sin(dp / 2)
                         + std::cos(p1) * std::cos(p2) * std::sin(dl / 2) * std::sin(dl / 2);
        return 2 * R * std::asin(std::sqrt(a));
    }
};

#endif