#include <QtTest>
#include <cmath>
#include <limits>
#include "router.h"
#include "geogrid.h"
#include "osmcost.h"

static const float BLOCK = std::numeric_limits<float>::infinity();

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
};

QTEST_APPLESS_MAIN(TstRouter)
#include "tst_router.moc"