#pragma once
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointF>
#include <QVector>
#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>
#include "geogrid.h"

namespace OsmCost {

constexpr float kBlocked = std::numeric_limits<float>::infinity();
constexpr float kStreamCost = 20.0f;
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

// Calls fn(cell) for every cell the line passes through (walks it in steps of about half a cell)
template <typename F>
inline void forEachLineCell(const GeoGrid &gg, const Ring &line, F fn) {
    for (int i = 0; i + 1 < line.size(); i++) {
        const QPointF &p = line[i], &q = line[i + 1];
        if (!gg.intersects(std::min(p.y(), q.y()), std::min(p.x(), q.x()),
                           std::max(p.y(), q.y()), std::max(p.x(), q.x())))
            continue;
        // count steps from the real (unclamped) length: a segment that starts outside
        // a small grid must not be sampled more coarsely than half a cell
        const double cells = std::max(std::abs(gg.cellX(q.x()) - gg.cellX(p.x())),
                                      std::abs(gg.cellY(q.y()) - gg.cellY(p.y())));
        const int steps = std::max(1, int(std::ceil(2 * cells)));
        for (int s = 0; s <= steps; s++) {
            const double t = double(s) / steps;
            const double lat = line[i].y() + t * (line[i + 1].y() - line[i].y());
            const double lon = line[i].x() + t * (line[i + 1].x() - line[i].x());
            if (gg.contains(lat, lon)) fn(gg.toCell(lat, lon));
        }
    }
}

// Calls fn(cell) for every cell whose centre is inside the ring
template <typename F>
inline void forEachInteriorCell(const GeoGrid &gg, const Ring &ring, F fn) {
    if (ring.size() < 3) return;
    double minLat = ring[0].y(), maxLat = minLat, minLon = ring[0].x(), maxLon = minLon;
    for (const QPointF &p : ring) {
        minLat = std::min(minLat, p.y()); maxLat = std::max(maxLat, p.y());
        minLon = std::min(minLon, p.x()); maxLon = std::max(maxLon, p.x());
    }
    if (!gg.intersects(minLat, minLon, maxLat, maxLon)) return;   // else it scans a clamped edge strip
    const Cell a = gg.toCell(maxLat, minLon);   // north-west
    const Cell b = gg.toCell(minLat, maxLon);   // south-east
    for (int y = a.y; y <= b.y; y++)
        for (int x = a.x; x <= b.x; x++) {
            double lat, lon;
            gg.toLatLon(Cell{x, y}, lat, lon);
            if (inside(ring, lon, lat)) fn(Cell{x, y});
        }
}

inline float highwayCost(const QString &h) {
    if (h == "footway" || h == "path" || h == "track" || h == "steps"
        || h == "pedestrian" || h == "cycleway" || h == "bridleway")
        return 0.6f;
    return 0.8f;   // roads
}

// OSM data parsed once into what fill() needs. Parsing the GeoJSON is the slow part
// (14 MB, 35.7k features for Zakopane); with a bounding box per feature, a fill touches
// only the features that overlap its grid.
struct Feature {
    enum Kind : quint8 { Area, River, Barrier, Stream, Highway };
    Kind kind;
    bool bridge = false;       // highways only
    float cost = 0;            // highways only
    QVector<Ring> rings;       // Area: outer rings; lines: one ring
    double s = 90, w = 180, n = -90, e = -180;   // bounding box
};

struct Features {
    std::vector<Feature> obstacles;   // pass 1, in file order (order matters: see fill)
    std::vector<Feature> highways;    // pass 2
    bool hasCoverage = false;         // GeoJSON "bbox" member [west, south, east, north]
    double covS = 0, covW = 0, covN = 0, covE = 0;

    bool covers(double lat, double lon) const {
        return hasCoverage && lat >= covS && lat <= covN && lon >= covW && lon <= covE;
    }
};

inline Features parse(const QJsonDocument &doc) {
    Features out;
    const QJsonObject root = doc.object();
    const QJsonArray bbox = root.value("bbox").toArray();
    if (bbox.size() == 4) {
        out.hasCoverage = true;
        out.covW = bbox[0].toDouble(); out.covS = bbox[1].toDouble();
        out.covE = bbox[2].toDouble(); out.covN = bbox[3].toDouble();
    }

    auto finish = [](Feature &f) {
        for (const Ring &r : f.rings)
            for (const QPointF &p : r) {
                f.s = std::min(f.s, p.y()); f.n = std::max(f.n, p.y());
                f.w = std::min(f.w, p.x()); f.e = std::max(f.e, p.x());
            }
    };

    for (const QJsonValue &fv : root.value("features").toArray()) {
        const QJsonObject o = fv.toObject();
        const QJsonObject g = o.value("geometry").toObject();
        const QJsonObject p = o.value("properties").toObject();
        const QString type = g.value("type").toString();

        Feature f;
        const bool areaObstacle = p.contains("building") || p.value("natural").toString() == "water";
        if (areaObstacle && (type == "Polygon" || type == "MultiPolygon")) {
            f.kind = Feature::Area;
            f.rings = outerRings(g);
        } else if (type == "LineString") {
            const QString barrier = p.value("barrier").toString();
            const QString waterway = p.value("waterway").toString();
            if (waterway == "river") f.kind = Feature::River;
            else if (barrier == "fence" || barrier == "wall") f.kind = Feature::Barrier;
            else if (p.contains("waterway")) f.kind = Feature::Stream;   // streams, ditches, drains
            else f.kind = Feature::Highway;
            if (f.kind == Feature::Highway && !p.contains("highway")) continue;
            f.rings.append(toRing(g.value("coordinates").toArray()));
            if (f.kind == Feature::Highway) {
                f.cost = highwayCost(p.value("highway").toString());
                f.bridge = p.contains("bridge");
            }
        } else {
            continue;
        }
        finish(f);
        const Feature::Kind kind = f.kind;
        (kind == Feature::Highway ? out.highways : out.obstacles).push_back(std::move(f));

        // a line that is both a highway and an obstacle (rare: e.g. a stream mapped on a track)
        // keeps its old behaviour of counting in both passes
        if (type == "LineString" && kind != Feature::Highway && p.contains("highway")) {
            Feature h;
            h.kind = Feature::Highway;
            h.rings.append(toRing(g.value("coordinates").toArray()));
            h.cost = highwayCost(p.value("highway").toString());
            h.bridge = p.contains("bridge");
            finish(h);
            out.highways.push_back(std::move(h));
        }
    }
    return out;
}

// Blocked cells come in two kinds:
//  hard: building/lake interiors (cell centre inside) and rivers. Only bridges cross them.
//  soft: a building/lake outline, fence or wall only passes through the cell. Blocked, unless a
//        mapped road or path runs through it too: at 5 m a cell cannot tell "path beside a wall"
//        from "path through a wall", so the mapped path wins (sidewalks must stay walkable).
// outlines = false gives the optimistic grid (no soft blocking of area outlines), see corridor.h.
inline void fill(GeoGrid &gg, const Features &feats, bool outlines = true) {
    Grid &grid = gg.grid();
    std::vector<char> hard(size_t(grid.width()) * grid.height(), 0);
    auto blockHard = [&](Cell c) { grid.setCost(c.x, c.y, kBlocked); hard[size_t(c.y) * grid.width() + c.x] = 1; };
    auto blockSoft = [&](Cell c) { grid.setCost(c.x, c.y, kBlocked); };

    // pass 1: obstacles
    for (const Feature &f : feats.obstacles) {
        if (!gg.intersects(f.s, f.w, f.n, f.e)) continue;
        switch (f.kind) {
        case Feature::Area:
            for (const Ring &r : f.rings) {
                forEachInteriorCell(gg, r, blockHard);
                if (outlines) forEachLineCell(gg, r, blockSoft);   // polygons smaller than a cell still mark it
            }
            break;
        case Feature::River:   forEachLineCell(gg, f.rings[0], blockHard); break;
        case Feature::Barrier: forEachLineCell(gg, f.rings[0], blockSoft); break;
        case Feature::Stream:  // wadeable but expensive; never unblocks anything
            forEachLineCell(gg, f.rings[0], [&](Cell c) {
                if (!grid.blocked(c.x, c.y)) grid.setCost(c.x, c.y, kStreamCost);
            });
            break;
        case Feature::Highway: break;
        }
    }

    // pass 2: roads and paths overwrite everything except hard obstacles; bridges cross those too
    for (const Feature &f : feats.highways) {
        if (!gg.intersects(f.s, f.w, f.n, f.e)) continue;
        forEachLineCell(gg, f.rings[0], [&](Cell c) {
            if (f.bridge || !hard[size_t(c.y) * grid.width() + c.x]) grid.setCost(c.x, c.y, f.cost);
        });
    }
}

// convenience for one-off fills (tests): parses every call, so cache a Features instead
inline void fill(GeoGrid &gg, const QJsonDocument &doc, bool outlines = true) {
    fill(gg, parse(doc), outlines);
}

} // namespace OsmCost