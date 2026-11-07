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
#include <QFile>
#include <QJsonDocument>
#include <limits>
#include "osmcost.h"

class RoutePlanner : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
public:
    explicit RoutePlanner(QObject *parent = nullptr) : QObject(parent) {}

    // points: [{lat, lon}, ...] in route order
    Q_INVOKABLE QVariantMap plan(const QVariantList &points, double cellMetres = 30.0) const {
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

        GeoGrid gg(south - dLat, west - dLon, north + dLat, east + dLon, cellMetres);
        if (qint64(gg.width()) * gg.height() > 4000000) {
            out["error"] = "AREA TOO LARGE";
            return out;
        }

        // terrain costs from OSM data (temporary: fixed file, later the working space data)
        QFile osm(QDir::homePath() + "/osm/area.geojson");
        if (osm.open(QIODevice::ReadOnly))
            OsmCost::fill(gg, QJsonDocument::fromJson(osm.readAll()));
        else
            qWarning() << "no OSM data at" << osm.fileName() << "- routing on an empty grid";

        // one search per leg, legs joined into one path
        std::vector<Cell> full;
        for (int i = 0; i + 1 < points.size(); i++) {
            const QVariantMap a = points[i].toMap();
            const QVariantMap b = points[i + 1].toMap();
            const Cell s = gg.toCell(a.value("lat").toDouble(), a.value("lon").toDouble());
            const Cell g = gg.toCell(b.value("lat").toDouble(), b.value("lon").toDouble());

            const PathResult r = Router::findPath(gg.grid(), s, g);
            if (!r.found) {
                out["error"] = "NO PATH FOR LEG " + QString::number(i + 1);
                out["failedLeg"] = i + 1;
                return out; // no straight-line fallback
            }
            // later legs skip their first cell: it is the last cell of the previous leg
            full.insert(full.end(), r.path.begin() + (i == 0 ? 0 : 1), r.path.end());
        }

        // cells -> [lon, lat] pairs for GeoJSON, plus the total length
        QVariantList coords;
        double length = 0, prevLat = 0, prevLon = 0;
        for (size_t k = 0; k < full.size(); k++) {
            double lat, lon;
            gg.toLatLon(full[k], lat, lon);
            if (k > 0) length += distanceMetres(prevLat, prevLon, lat, lon);
            prevLat = lat;
            prevLon = lon;
            coords.append(QVariant(QVariantList{ lon, lat }));
        }

        out["ok"] = true;
        out["coordinates"] = coords;
        out["lengthMeters"] = length;
        return out;
    }

private:
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