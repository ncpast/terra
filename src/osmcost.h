#pragma once
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointF>
#include <QVector>
#include <algorithm>
#include <cmath>
#include <limits>
#include "geogrid.h"

namespace OsmCost {

constexpr float kBlocked = std::numeric_limits<float>::infinity();
using Ring = QVector<QPointF>;  // x = lon, y = lat

inline Ring toRing(const QJsonArray &coords) {
    Ring r;
    for (const QJsonValue &c : coords) {
        const QJsonArray p = c.toArray();
        r.append(QPointF(p.at(0).toDouble(), p.at(1).toDouble()));
    }
    return r;
}

// Outer rings only (holes ignored for now)
inline QVector<Ring> outerRings(const QJsonObject &geom) {
    QVector<Ring> out;
    const QString type = geom.value("type").toString();
    const QJsonArray c = geom.value("coordinates").toArray();
    if (type == "Polygon" && !c.isEmpty())
        out.append(toRing(c.at(0).toArray()));
    else if (type == "MultiPolygon")
        for (const QJsonValue &poly : c)
            if (!poly.toArray().isEmpty())
                out.append(toRing(poly.toArray().at(0).toArray()));
    return out;
}

// Ray casting
inline bool inside(const Ring &ring, double lon, double lat) {
    bool in = false;
    for (int i = 0, j = ring.size() - 1; i < ring.size(); j = i++) {
        const QPointF &a = ring[i];
        const QPointF &b = ring[j];
        if ((a.y() > lat) != (b.y() > lat)
            && lon < (b.x() - a.x()) * (lat - a.y()) / (b.y() - a.y()) + a.x())
            in = !in;
    }
    return in;
}

inline void fillPolygon(GeoGrid &gg, const Ring &ring, float cost) {
    if (ring.size() < 3) return;
    double minLat = ring[0].y(), maxLat = minLat, minLon = ring[0].x(), maxLon = minLon;
    for (const QPointF &p : ring) {
        minLat = std::min(minLat, p.y()); maxLat = std::max(maxLat, p.y());
        minLon = std::min(minLon, p.x()); maxLon = std::max(maxLon, p.x());
    }
    const Cell a = gg.toCell(maxLat, minLon);   // north-west
    const Cell b = gg.toCell(minLat, maxLon);   // south-east
    for (int y = a.y; y <= b.y; y++)
        for (int x = a.x; x <= b.x; x++) {
            double lat, lon;
            gg.toLatLon(Cell{x, y}, lat, lon);
            if (inside(ring, lon, lat))
                gg.grid().setCost(x, y, cost);
        }
}

// Walk the line in steps of about half a cell
inline void stampLine(GeoGrid &gg, const Ring &line, float cost, bool keepBlocked) {
    auto stamp = [&](double lat, double lon) {
        if (!gg.contains(lat, lon)) return;
        const Cell c = gg.toCell(lat, lon);
        if (keepBlocked && std::isinf(gg.grid().cost(c.x, c.y))) return;
        gg.grid().setCost(c.x, c.y, cost);
    };
    for (int i = 0; i + 1 < line.size(); i++) {
        const Cell a = gg.toCell(line[i].y(), line[i].x());
        const Cell b = gg.toCell(line[i + 1].y(), line[i + 1].x());
        const int steps = std::max(1, 2 * std::max(std::abs(a.x - b.x), std::abs(a.y - b.y)));
        for (int s = 0; s <= steps; s++) {
            const double t = double(s) / steps;
            stamp(line[i].y() + t * (line[i + 1].y() - line[i].y()),
                  line[i].x() + t * (line[i + 1].x() - line[i].x()));
        }
    }
}

inline float highwayCost(const QString &h) {
    if (h == "footway" || h == "path" || h == "track" || h == "steps"
        || h == "pedestrian" || h == "cycleway" || h == "bridleway")
        return 0.6f;
    return 0.8f;   // roads
}

inline void fill(GeoGrid &gg, const QJsonDocument &doc) {
    const QJsonArray feats = doc.object().value("features").toArray();

    // pass 1: obstacles
    for (const QJsonValue &fv : feats) {
        const QJsonObject f = fv.toObject();
        const QJsonObject g = f.value("geometry").toObject();
        const QJsonObject p = f.value("properties").toObject();
        const QString type = g.value("type").toString();

        const bool areaObstacle = p.contains("building") || p.value("natural").toString() == "water";
        if (areaObstacle && (type == "Polygon" || type == "MultiPolygon")) {
            for (const Ring &r : outerRings(g)) fillPolygon(gg, r, kBlocked);
        } else if (type == "LineString") {
            const QString barrier = p.value("barrier").toString();
            if (barrier == "fence" || barrier == "wall" || p.contains("waterway"))
                stampLine(gg, toRing(g.value("coordinates").toArray()), kBlocked, false);
        }
    }

    // pass 2: roads and paths (bridges may cross obstacles, everything else only free cells)
    for (const QJsonValue &fv : feats) {
        const QJsonObject f = fv.toObject();
        const QJsonObject g = f.value("geometry").toObject();
        const QJsonObject p = f.value("properties").toObject();
        if (g.value("type").toString() != "LineString" || !p.contains("highway")) continue;
        stampLine(gg, toRing(g.value("coordinates").toArray()),
                  highwayCost(p.value("highway").toString()),
                  /*keepBlocked=*/ !p.contains("bridge"));
    }
}

} // namespace OsmCost