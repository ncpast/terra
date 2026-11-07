#include <QtTest>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <cmath>
#include <limits>
#include "router.h"
#include "geogrid.h"
#include "osmcost.h"
#include "corridor.h"

static const float BLOCK = std::numeric_limits<float>::infinity();

static QVector<OsmCost::Ring> buildingRings(const QJsonDocument &doc) {
    QVector<OsmCost::Ring> out;
    for (const QJsonValue &fv : doc.object().value("features").toArray()) {
        const QJsonObject o = fv.toObject();
        if (o.value("properties").toObject().contains("building"))
            out += OsmCost::outerRings(o.value("geometry").toObject());
    }
    return out;
}

static double segMetres(const Corridor::LatLon &p, const Corridor::LatLon &q) {
    return std::hypot((q.first - p.first) * 111320.0,
                      (q.second - p.second) * 111320.0 * std::cos(p.first * M_PI / 180));
}

static double lineMetres(const std::vector<Corridor::LatLon> &line) {
    double m = 0;
    for (size_t k = 1; k < line.size(); k++) m += segMetres(line[k - 1], line[k]);
    return m;
}

// walk every segment in 0.5 m steps and count samples inside a building
static int samplesInBuildings(const std::vector<Corridor::LatLon> &line,
                              const QVector<OsmCost::Ring> &buildings) {
    struct Box { double w = 180, s = 90, e = -180, n = -90; };
    QVector<Box> boxes;
    for (const OsmCost::Ring &b : buildings) {
        Box r;
        for (const QPointF &p : b) {   // x = lon, y = lat
            r.w = std::min(r.w, p.x()); r.e = std::max(r.e, p.x());
            r.s = std::min(r.s, p.y()); r.n = std::max(r.n, p.y());
        }
        boxes.append(r);
    }
    int n = 0;
    for (size_t k = 1; k < line.size(); k++) {
        const Corridor::LatLon &p = line[k - 1], &q = line[k];
        const int steps = std::max(1, int(segMetres(p, q) / 0.5));
        for (int i = 0; i <= steps; i++) {
            const double t = double(i) / steps;
            const double la = p.first + t * (q.first - p.first);
            const double lo = p.second + t * (q.second - p.second);
            for (int bi = 0; bi < buildings.size(); bi++) {
                const Box &r = boxes[bi];
                if (lo < r.w || lo > r.e || la < r.s || la > r.n) continue;
                if (OsmCost::inside(buildings[bi], lo, la)) { n++; break; }
            }
        }
    }
    return n;
}

// geometric crossings of the line with fence / wall / river ways
static int barrierCrossings(const std::vector<Corridor::LatLon> &line, const QJsonDocument &doc) {
    auto ccw = [](QPointF a, QPointF b, QPointF c) {
        return (c.y() - a.y()) * (b.x() - a.x()) - (b.y() - a.y()) * (c.x() - a.x());
    };
    int n = 0;
    for (const QJsonValue &fv : doc.object().value("features").toArray()) {
        const QJsonObject o = fv.toObject();
        const QJsonObject p = o.value("properties").toObject();
        const QJsonObject g = o.value("geometry").toObject();
        const QString barrier = p.value("barrier").toString();
        if (g.value("type").toString() != "LineString") continue;
        if (barrier != "fence" && barrier != "wall" && p.value("waterway").toString() != "river") continue;
        const OsmCost::Ring way = OsmCost::toRing(g.value("coordinates").toArray());
        for (int q = 0; q + 1 < way.size(); q++)
            for (size_t k = 1; k < line.size(); k++) {
                const QPointF A(line[k - 1].second, line[k - 1].first), B(line[k].second, line[k].first);
                if ((ccw(A, B, way[q]) > 0) != (ccw(A, B, way[q + 1]) > 0)
                    && (ccw(way[q], way[q + 1], A) > 0) != (ccw(way[q], way[q + 1], B) > 0))
                    n++;
            }
    }
    return n;
}

class TstRouter : public QObject {
    Q_OBJECT
private slots:

    void openGridCostMatchesOctile() {
        Grid g(10, 10);
        PathResult r = Router::findPath(g, {0, 0}, {9, 4});
        QVERIFY(r.found);
        QCOMPARE(r.cost, 4 * std::sqrt(2.0) + 5);   // 4 diagonal + 5 straight steps
        QCOMPARE(r.path.front().x, 0);
        QCOMPARE(r.path.back().x, 9);
    }

    void wallForcesDetour() {
        Grid g(10, 10);
        for (int y = 0; y < 9; y++) g.setCost(5, y, BLOCK); // wall at x = 5, gap at y = 9
        PathResult r = Router::findPath(g, {0, 0}, {9, 0});
        QVERIFY(r.found);
        QVERIFY(r.cost > 9);    // longer than the straight line
        for (const Cell &c : r.path)
            QVERIFY(!g.blocked(c.x, c.y));
    }

    void enclosedGoalHasNoPath() {
        Grid g(10, 10);
        for (int dy = -1; dy <= 1; dy++)
            for (int dx = -1; dx <= 1; dx++)
                if (dx || dy) g.setCost(5 + dx, 5 + dy, BLOCK);
        PathResult r = Router::findPath(g, {0, 0}, {5, 5});
        QVERIFY(!r.found);
    }

    void startEqualsGoal() {
        Grid g(5, 5);
        PathResult r = Router::findPath(g, {2, 2}, {2, 2});
        QVERIFY(r.found);
        QCOMPARE(r.path.size(), size_t(1));
        QCOMPARE(r.cost, 0.0);
    }

    void geoGridRoundTrip() {
        GeoGrid gg(49.10, 19.90, 49.30, 20.20, 30.0);   // about 22 km x 22 km, 30 m cells
        Cell c = gg.toCell(49.18, 20.07);
        double lat, lon;
        gg.toLatLon(c, lat, lon);
        QVERIFY(std::abs(lat - 49.18) < 30.0 / 111320.0);
        QVERIFY(std::abs(lon - 20.07) < 30.0 / (111320.0 * std::cos(49.2 * M_PI / 180.0)));
    }

    void osmPolygonBlocksCellsAndFootwayIsCheap() {
        GeoGrid gg(49.000, 19.000, 49.010, 19.010, 30.0);

        const QByteArray json = R"({"type":"FeatureCollection","features":[
        {"type":"Feature","properties":{"building":"yes"},"geometry":{"type":"Polygon",
        "coordinates":[[[19.004,49.004],[19.006,49.004],[19.006,49.006],[19.004,49.006],[19.004,49.004]]]}},
        {"type":"Feature","properties":{"highway":"footway"},"geometry":{"type":"LineString",
        "coordinates":[[19.001,49.001],[19.009,49.001]]}}]})";
        OsmCost::fill(gg, QJsonDocument::fromJson(json));

        const Cell mid = gg.toCell(49.005, 19.005);
        QVERIFY(std::isinf(gg.grid().cost(mid.x, mid.y)));
        const Cell free = gg.toCell(49.009, 19.001);
        QCOMPARE(gg.grid().cost(free.x, free.y), 1.0f);
        const Cell path = gg.toCell(49.001, 19.005);
        QCOMPARE(gg.grid().cost(path.x, path.y), 0.6f);
    }

    void osmStreamIsCrossableRiverIsNot() {
        // one waterway running north-south across the whole grid at lon 19.005
        // (KIND placeholder: moc's lexer chokes on a raw string ending in `")`)
        auto waterway = [](const char *kind) {
            QByteArray json = R"({"type":"FeatureCollection","features":[
            {"type":"Feature","properties":{"waterway":"KIND"},"geometry":{"type":"LineString",
            "coordinates":[[19.005,49.000],[19.005,49.010]]}}]})";
            return json.replace("KIND", kind);
        };

        GeoGrid stream(49.000, 19.000, 49.010, 19.010, 30.0);
        OsmCost::fill(stream, QJsonDocument::fromJson(waterway("stream")));
        const Cell sc = stream.toCell(49.005, 19.005);
        QCOMPARE(stream.grid().cost(sc.x, sc.y), OsmCost::kStreamCost);

        const Cell west = stream.toCell(49.005, 19.001);
        const Cell east = stream.toCell(49.005, 19.009);
        PathResult r = Router::findPath(stream.grid(), west, east);
        QVERIFY(r.found);
        bool crossed = false;
        for (const Cell &c : r.path)
            crossed |= stream.grid().cost(c.x, c.y) == OsmCost::kStreamCost;
        QVERIFY(crossed);   // no way round: the path has to wade through

        GeoGrid river(49.000, 19.000, 49.010, 19.010, 30.0);
        OsmCost::fill(river, QJsonDocument::fromJson(waterway("river")));
        const Cell rc = river.toCell(49.005, 19.005);
        QVERIFY(std::isinf(river.grid().cost(rc.x, rc.y)));
        QVERIFY(!Router::findPath(river.grid(), west, east).found);
    }

    void osmBuildingSmallerThanCellIsBlocked() {
        // 10 m x 10 m house on 30 m cells: no cell centre falls inside it
        GeoGrid gg(49.000, 19.000, 49.010, 19.010, 30.0);
        const QByteArray json = R"({"type":"FeatureCollection","features":[
        {"type":"Feature","properties":{"building":"house"},"geometry":{"type":"Polygon",
        "coordinates":[[[19.00500,49.00500],[19.00514,49.00500],[19.00514,49.00509],[19.00500,49.00509],[19.00500,49.00500]]]}}]})";
        OsmCost::fill(gg, QJsonDocument::fromJson(json));
        const Cell c = gg.toCell(49.005045, 19.00507);
        QVERIFY(std::isinf(gg.grid().cost(c.x, c.y)));
    }

    void smoothOpenGridIsOneSegment() {
        Grid g(10, 10);
        PathResult r = Router::findPath(g, {0, 0}, {9, 4});
        const std::vector<Cell> s = Router::smooth(g, r.path);
        QCOMPARE(s.size(), size_t(2));
        QCOMPARE(s.back().x, 9);
        QCOMPARE(s.back().y, 4);
    }

    void smoothNeverCrossesBlockedOrCostsMore() {
        Grid g(10, 10);
        for (int y = 0; y < 9; y++) g.setCost(5, y, BLOCK);  // wall at x = 5, gap at y = 9
        for (int x = 0; x < 10; x++) g.setCost(x, 9, 0.6f);  // cheap path along the gap row
        PathResult r = Router::findPath(g, {0, 0}, {9, 0});
        const std::vector<Cell> s = Router::smooth(g, r.path);
        QVERIFY(s.size() < r.path.size());
        double total = 0;
        for (size_t k = 1; k < s.size(); k++) {
            double c;
            QVERIFY(Router::lineCost(g, s[k - 1], s[k], c));
            total += c;
        }
        QVERIFY(total <= r.cost + 1e-9);
    }

    // Real data: a route across Zakopane at 5 m cells must not enter any building
    void zakopaneRouteAvoidsBuildings() {
        QFile f(QDir::homePath() + "/osm/area.geojson");
        if (!f.open(QIODevice::ReadOnly)) QSKIP("no ~/osm/area.geojson");
        const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());

        const QVector<OsmCost::Ring> buildings = buildingRings(doc);

        const OsmCost::Features feats = OsmCost::parse(doc);
        for (double cellM : {30.0, 10.0, 5.0, 2.5}) {
            QElapsedTimer timer;
            timer.start();
            GeoGrid gg(49.288, 19.940, 49.298, 19.960, cellM);
            OsmCost::fill(gg, feats);
            const qint64 fillMs = timer.restart();
            const PathResult r = Router::findPath(gg.grid(), gg.toCell(49.2887495, 19.94466),   // both ends on footways
                                                  gg.toCell(49.2972495, 19.9588535));
            const qint64 searchMs = timer.restart();
            if (!r.found) {   // expected at coarse cells: every touched cell is blocked
                qInfo("cell %4.1f m: grid %dx%d, fill %lld ms, A* %lld ms, NO PATH",
                      cellM, gg.width(), gg.height(), fillMs, searchMs);
                QVERIFY(cellM > 5.0);
                continue;
            }
            const std::vector<Cell> s = Router::smooth(gg.grid(), r.path);
            const qint64 smoothMs = timer.elapsed();

            std::vector<Corridor::LatLon> line;
            for (const Cell &c : s) {
                double lat, lon;
                gg.toLatLon(c, lat, lon);
                line.push_back({ lat, lon });
            }
            const int inBuilding = samplesInBuildings(line, buildings);
            qInfo("cell %4.1f m: grid %dx%d, fill %lld ms, A* %lld ms, smooth %lld ms, "
                  "%zu cells -> %zu vertices, samples inside buildings: %d",
                  cellM, gg.width(), gg.height(), fillMs, searchMs, smoothMs,
                  r.path.size(), s.size(), inBuilding);
            if (cellM <= 5.0) QCOMPARE(inBuilding, 0);
        }
    }

    void longLineCrossingSmallGridHasNoGaps() {
        // a wall far longer than the grid: steps must follow its real length, not the clamped one
        GeoGrid gg(49.0000, 19.0000, 49.0010, 19.0010, 1.0);
        const QByteArray json = R"({"type":"FeatureCollection","features":[
        {"type":"Feature","properties":{"barrier":"wall"},"geometry":{"type":"LineString",
        "coordinates":[[18.9,49.0005],[19.1,49.0005]]}}]})";
        OsmCost::fill(gg, QJsonDocument::fromJson(json));
        const int row = gg.toCell(49.0005, 19.0005).y;
        for (int x = 0; x < gg.width(); x++)
            QVERIFY2(gg.grid().blocked(x, row), qPrintable(QString("gap at x = %1").arg(x)));
        QVERIFY(!Router::findPath(gg.grid(), {5, 5}, {5, gg.height() - 5}).found);
    }

    void nearestFreeFindsClosest() {
        Grid g(9, 9);
        for (int y = 0; y < 9; y++)
            for (int x = 0; x < 9; x++) g.setCost(x, y, BLOCK);
        g.setCost(7, 7, 1.0f);   // ring 3 (corner), squared distance 18
        g.setCost(8, 4, 1.0f);   // ring 4, squared distance 16: closer although in a farther ring
        Cell c;
        QVERIFY(Router::nearestFree(g, {4, 4}, 4, c));
        QCOMPARE(c.x, 8);
        QCOMPARE(c.y, 4);
        QVERIFY(Router::nearestFree(g, {4, 4}, 3, c));
        QCOMPARE(c.x, 7);
        QVERIFY(!Router::nearestFree(g, {4, 4}, 2, c));   // nothing free within 2 cells
    }

    // Real data: 5 m coarse path refined at 1 m inside a 30 m corridor
    // Real data: optimistic 5 m coarse path refined at 1 m, compared with a full 1 m search
    void zakopaneCorridorRefinement() {
        QFile f(QDir::homePath() + "/osm/area.geojson");
        if (!f.open(QIODevice::ReadOnly)) QSKIP("no ~/osm/area.geojson");
        const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
        const QVector<OsmCost::Ring> buildings = buildingRings(doc);
        const Corridor::LatLon a{ 49.2887495, 19.94466 }, b{ 49.2972495, 19.9588535 };
        const OsmCost::Features feats = OsmCost::parse(doc);

        // reference: the whole area at 1 m
        QElapsedTimer timer;
        timer.start();
        GeoGrid ref(49.288, 19.940, 49.298, 19.960, 1.0);
        OsmCost::fill(ref, feats);
        const PathResult rr = Router::findPath(ref.grid(), ref.toCell(a.first, a.second),
                                               ref.toCell(b.first, b.second));
        QVERIFY(rr.found);
        std::vector<Corridor::LatLon> refLine;
        for (const Cell &c : Router::smooth(ref.grid(), rr.path)) {
            double lat, lon;
            ref.toLatLon(c, lat, lon);
            refLine.push_back({ lat, lon });
        }
        const qint64 refMs = timer.restart();

        GeoGrid coarse(49.288, 19.940, 49.298, 19.960, 5.0);
        OsmCost::fill(coarse, feats, /*outlines=*/ false);   // optimistic, as RoutePlanner does
        const PathResult r = Router::findPath(coarse.grid(), coarse.toCell(a.first, a.second),
                                              coarse.toCell(b.first, b.second));
        QVERIFY(r.found);
        const qint64 coarseMs = timer.restart();

        // default budget, and a small one that forces many joined pieces
        for (qint64 budget : {qint64(50000), qint64(10000)}) {
            Corridor::Settings st;
            st.maxPieceCells = budget;
            st.allowFallback = false;
            timer.restart();
            const Corridor::Result fine = Corridor::refineLeg(feats, coarse, r.path, a, b, st);
            const qint64 fineMs = timer.elapsed();

            QVERIFY(fine.ok);
            QCOMPARE(fine.line.front(), a);   // exact checkpoints at both ends
            QCOMPARE(fine.line.back(), b);
            const int inside = samplesInBuildings(fine.line, buildings);
            const int crossings = barrierCrossings(fine.line, doc);
            const double m = lineMetres(fine.line), refM = lineMetres(refLine);
            qInfo("budget %6lld: full 1 m %lld ms %.0f m | coarse %lld ms + refine %lld ms, %d pieces, "
                  "%d retries, %.0f m (%+.1f%%), inside buildings %d, barrier crossings %d",
                  budget, refMs, refM, coarseMs, fineMs, fine.pieces, fine.retries, m,
                  100 * (m / refM - 1), inside, crossings);
            QCOMPARE(inside, 0);
            QCOMPARE(crossings, 0);
            QVERIFY(m < refM * 1.05);   // within 5% of the full 1 m search
        }
    }

    void refineWithoutFallbackFailsCleanly() {
        // the coarse grid knows nothing, the OSM data has a wall across the whole area
        const OsmCost::Features wall = OsmCost::parse(QJsonDocument::fromJson(R"({"type":"FeatureCollection","features":[
        {"type":"Feature","properties":{"barrier":"wall"},"geometry":{"type":"LineString",
        "coordinates":[[18.99,49.0005],[19.01,49.0005]]}}]})"));
        GeoGrid coarse(49.000, 19.000, 49.001, 19.001, 5.0);
        const Corridor::LatLon a{ 49.0001, 19.0005 }, b{ 49.0009, 19.0005 };
        const PathResult r = Router::findPath(coarse.grid(), coarse.toCell(a.first, a.second),
                                              coarse.toCell(b.first, b.second));
        QVERIFY(r.found);

        Corridor::Settings strict;
        strict.allowFallback = false;
        const Corridor::Result fail = Corridor::refineLeg(wall, coarse, r.path, a, b, strict);
        QVERIFY(!fail.ok);

        const Corridor::Result kept = Corridor::refineLeg(wall, coarse, r.path, a, b);  // fallback on
        QVERIFY(kept.ok);
        QCOMPARE(kept.fallbacks, 1);
        QCOMPARE(kept.line.front(), a);
        QCOMPARE(kept.line.back(), b);
    }

    void parseReadsCoverageBox() {
        const OsmCost::Features none = OsmCost::parse(QJsonDocument::fromJson(R"({"type":"FeatureCollection","features":[]})"));
        QVERIFY(!none.hasCoverage);
        QVERIFY(!none.covers(49.0, 19.0));   // no box: nothing is covered
        const OsmCost::Features f = OsmCost::parse(QJsonDocument::fromJson(
            R"({"type":"FeatureCollection","bbox":[19.90,49.25,20.05,49.32],"features":[]})"));
        QVERIFY(f.covers(49.30, 19.96));
        QVERIFY(!f.covers(49.232, 19.9817));   // Kasprowy Wierch, south of the box
    }

    void osmStreamDoesNotUnblockBuilding() {
        // building listed first, stream runs through it: the building must stay blocked
        GeoGrid gg(49.000, 19.000, 49.010, 19.010, 30.0);
        const QByteArray json = R"({"type":"FeatureCollection","features":[
        {"type":"Feature","properties":{"building":"yes"},"geometry":{"type":"Polygon",
        "coordinates":[[[19.004,49.004],[19.006,49.004],[19.006,49.006],[19.004,49.006],[19.004,49.004]]]}},
        {"type":"Feature","properties":{"waterway":"stream"},"geometry":{"type":"LineString",
        "coordinates":[[19.005,49.000],[19.005,49.010]]}}]})";
        OsmCost::fill(gg, QJsonDocument::fromJson(json));

        const Cell inBuilding = gg.toCell(49.005, 19.005);
        QVERIFY(std::isinf(gg.grid().cost(inBuilding.x, inBuilding.y)));
        const Cell outside = gg.toCell(49.001, 19.005);
        QCOMPARE(gg.grid().cost(outside.x, outside.y), OsmCost::kStreamCost);
    }
};

QTEST_APPLESS_MAIN(TstRouter)
#include "tst_router.moc"