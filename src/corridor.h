#pragma once
// Coarse-to-fine refinement: a leg found on the coarse grid is searched again on a fine grid,
// but only inside a corridor around the coarse path. Long legs are cut into pieces so every
// fine grid stays small, whatever the route length.

#include <QJsonDocument>
#include <QtGlobal>
#include <algorithm>
#include <cmath>
#include <functional>
#include <utility>
#include <vector>
#include "geogrid.h"
#include "osmcost.h"
#include "router.h"

namespace Corridor {

using LatLon = std::pair<double, double>;

struct Settings {
    double fineMetres = 1.0;
    double corridorMetres = 30.0;       // half-width around the coarse path
    qint64 maxPieceCells = 50000;       // fine cells per piece (measured: 1M 1.4 s, 50k 0.34 s, same route)
    bool allowFallback = true;          // false: a failed piece fails the leg (use with an optimistic coarse grid)
};

struct Result {
    bool ok = true;             // false only when allowFallback is off and a piece failed
    std::vector<LatLon> line;   // vertices, first = exact start, last = exact goal
    int pieces = 0;
    int retries = 0;            // pieces that needed the 2x corridor
    int fallbacks = 0;          // pieces that kept the coarse path
};

struct Box {
    double s = 90, w = 180, n = -90, e = -180;
    void add(const LatLon &p) {
        s = std::min(s, p.first);  n = std::max(n, p.first);
        w = std::min(w, p.second); e = std::max(e, p.second);
    }
    // fine cells of this box after growing it by `pad` metres on every side
    qint64 cells(double pad, double cellM) const {
        const double cosLat = std::cos((s + n) / 2 * M_PI / 180);
        const double h = (n - s) * 111320.0 + 2 * pad;
        const double wd = (e - w) * 111320.0 * cosLat + 2 * pad;
        return qint64(std::ceil(h / cellM)) * qint64(std::ceil(wd / cellM));
    }
};

// Fine search for one piece. `coarse` holds the coarse cell centres of the piece.
// `from` is kept exactly (a checkpoint, or where the previous piece ended); `to` is kept
// exactly only if exactTo (the goal checkpoint), else the piece ends at its fine goal cell centre.
inline bool refinePiece(const OsmCost::Features &osm, const std::vector<LatLon> &coarse,
                        LatLon from, LatLon to, bool exactTo, double snapMetres, double corridorM,
                        double fineM, std::vector<LatLon> &out) {
    Box box;
    for (const LatLon &p : coarse) box.add(p);
    box.add(from);
    const double pad = corridorM + 2 * fineM;
    const double cosLat = std::cos((box.s + box.n) / 2 * M_PI / 180);
    const double dLat = pad / 111320.0, dLon = pad / (111320.0 * cosLat);
    GeoGrid fg(box.s - dLat, box.w - dLon, box.n + dLat, box.e + dLon, fineM);
    OsmCost::fill(fg, osm);

    // block every fine cell farther than corridorM from all coarse cell centres
    Grid &g = fg.grid();
    std::vector<char> near(size_t(g.width()) * g.height(), 0);
    const int r = int(std::ceil(corridorM / fineM));
    for (const LatLon &p : coarse) {
        const Cell c = fg.toCell(p.first, p.second);
        for (int dy = -r; dy <= r; dy++)
            for (int dx = -r; dx <= r; dx++) {
                const int x = c.x + dx, y = c.y + dy;
                if (dx * dx + dy * dy <= r * r && x >= 0 && y >= 0 && x < g.width() && y < g.height())
                    near[size_t(y) * g.width() + x] = 1;
            }
    }
    for (int y = 0; y < g.height(); y++)
        for (int x = 0; x < g.width(); x++)
            if (!near[size_t(y) * g.width() + x]) g.setCost(x, y, OsmCost::kBlocked);

    // the ends may sit on a cell that is blocked only at fine resolution: snap them,
    // but never farther than one coarse cell
    const int snap = int(std::ceil(snapMetres / fineM));
    Cell a, b;
    if (!Router::nearestFree(g, fg.toCell(from.first, from.second), snap, a)) return false;
    if (!Router::nearestFree(g, fg.toCell(to.first, to.second), snap, b)) return false;

    const PathResult pr = Router::findPath(g, a, b);
    if (!pr.found) return false;

    const std::vector<Cell> v = Router::smooth(g, pr.path);
    out.clear();
    out.push_back(from);
    for (size_t k = 1; k < v.size(); k++) {
        double lat, lon;
        fg.toLatLon(v[k], lat, lon);
        out.push_back({ lat, lon });
    }
    if (out.size() == 1) out.push_back(exactTo ? to : from);   // start and goal in one cell
    else if (exactTo) out.back() = to;
    return true;
}

// Refines one coarse leg. `path` is the raw coarse A* path (cells of `coarse`),
// a and b are the exact checkpoints.
inline Result refineLeg(const OsmCost::Features &osm, const GeoGrid &coarse, const std::vector<Cell> &path,
                        LatLon a, LatLon b, const Settings &st = {},
                        const std::function<void(double)> &progress = {}) {   // progress: 0..1 of this leg
    Result res;
    if (path.size() < 2) { res.line = { a, b }; return res; }

    std::vector<LatLon> pts(path.size());
    for (size_t k = 0; k < path.size(); k++) coarse.toLatLon(path[k], pts[k].first, pts[k].second);
    pts.front() = a;
    pts.back() = b;

    const double pad = st.corridorMetres + 2 * st.fineMetres;
    LatLon from = a;    // where the next piece starts: the exact checkpoint, then each piece's end
    size_t i = 0;
    while (i + 1 < pts.size()) {
        // grow the piece while its fine grid stays under the budget
        Box box;
        box.add(pts[i]);
        box.add(pts[i + 1]);
        size_t j = i + 1;
        while (j + 1 < pts.size()) {
            Box next = box;
            next.add(pts[j + 1]);
            if (next.cells(pad, st.fineMetres) > st.maxPieceCells) break;
            box = next;
            j++;
        }

        const std::vector<LatLon> piece(pts.begin() + i, pts.begin() + j + 1);
        const bool last = j + 1 == pts.size();
        std::vector<LatLon> fine;
        bool ok = refinePiece(osm, piece, from, pts[j], last, coarse.cellMetres(),
                              st.corridorMetres, st.fineMetres, fine);
        if (!ok) {
            res.retries++;
            ok = refinePiece(osm, piece, from, pts[j], last, coarse.cellMetres(),
                             2 * st.corridorMetres, st.fineMetres, fine);
        }
        if (!ok && !st.allowFallback) { res.ok = false; return res; }
        if (!ok) {
            // keep the coarse path for this piece: still a real path, only coarser
            res.fallbacks++;
            const std::vector<Cell> sub(path.begin() + i, path.begin() + j + 1);
            const std::vector<Cell> v = Router::smooth(coarse.grid(), sub);
            fine.clear();
            fine.push_back(from);
            for (size_t k = 1; k + 1 < v.size(); k++) {
                double lat, lon;
                coarse.toLatLon(v[k], lat, lon);
                fine.push_back({ lat, lon });
            }
            fine.push_back(pts[j]);
        }

        // pieces share their boundary point: skip it on all but the first piece
        res.line.insert(res.line.end(), fine.begin() + (res.line.empty() ? 0 : 1), fine.end());
        res.pieces++;
        from = fine.back();
        i = j;
        if (progress) progress(double(i) / double(pts.size() - 1));
    }
    return res;
}

} // namespace Corridor
