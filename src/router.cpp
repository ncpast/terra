#include "router.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>

namespace {

    constexpr double INF = std::numeric_limits<double>::infinity();
    const double SQRT2 = std::sqrt(2.0);

    // Estimate of the remaining cost: octile distance scaled by the cheapest cell on the grid,
    // so the estimate never exceeds the real cost (keeps A* optimal when some cells cost < 1)
    double heuristic(Cell a, Cell b, double minCost) {
        int dx = std::abs(a.x - b.x);
        int dy = std::abs(a.y - b.y);
        return minCost * (dx + dy + (SQRT2 - 2.0) * std::min(dx, dy));
    }

    // An entry in the priority queue: a cell and its f = g + h
    struct Node {
        double f;
        int index;
        bool operator>(const Node &o) const { return f > o.f; } // makes the queue pop the smallest f
    };

}   // namespace

PathResult Router::findPath(const Grid &grid, Cell start, Cell goal) {
    PathResult result;
    const int w = grid.width();
    const int h = grid.height();

    auto inside = [&](int x, int y) { return x >= 0 && y >= 0 && x < w && y < h; };

    if (!inside(start.x, start.y) || !inside(goal.x, goal.y)) return result;
    if (grid.blocked(start.x, start.y) || grid.blocked(goal.x, goal.y)) return result;

    // cheapest cell on the grid (blocked cells are skipped)
    double minCost = 1.0;
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
            if (!grid.blocked(x, y))
                minCost = std::min(minCost, double(grid.cost(x, y)));

    std::vector<double> g(w * h, INF);          // cheapest known cost from start to each cell
    std::vector<int> cameFrom(w * h, -1);       // previous cell on that cheapest path
    std::vector<bool> closed(w * h, false);     // cells whose best cost is final
    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> open;

    const int s = start.y * w + start.x;
    const int t = goal.y * w + goal.x;
    g[s] = 0;
    open.push({heuristic(start, goal, minCost), s});

    bool reached = false;

    while (!open.empty()) {
        const int cur = open.top().index;
        open.pop();

        if (closed[cur]) continue;  // an outdated duplicate entry, skip it
        closed[cur] = true;

        if (cur == t) { reached = true; break; }

        const int cx = cur % w;
        const int cy = cur / w;

        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                if (dx == 0 && dy == 0) continue;

                const int nx = cx + dx;
                const int ny = cy + dy;
                if (!inside(nx, ny) || grid.blocked(nx, ny)) continue;

                const bool diagonal = (dx != 0 && dy != 0);

                // don't squeeze diagonally between two blocked cells
                if (diagonal && (grid.blocked(cx + dx, cy) || grid.blocked(cx, cy + dy))) continue;

                const int ni = ny * w + nx;
                if (closed[ni]) continue;

                const double step = (diagonal ? SQRT2 : 1.0) * grid.cost(nx, ny);
                const double ng = g[cur] + step;

                if (ng < g[ni]) {   // found a cheaper way to reach this cell
                    g[ni] = ng;
                    cameFrom[ni] = cur;
                    open.push({ng + heuristic({nx, ny}, goal, minCost), ni});
                }
            }
        }
    }

    if (!reached) return result;    // found == false

    for (int i = t; i != -1; i = cameFrom[i])
        result.path.push_back({i % w, i / w});
    std::reverse(result.path.begin(), result.path.end());
    result.cost = g[t];
    result.found = true;
    return result;
}