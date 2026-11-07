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

// Amanatides-Woo grid traversal from the centre of a to the centre of b
bool Router::lineCost(const Grid &grid, Cell a, Cell b, double &cost) {
    const double dx = b.x - a.x, dy = b.y - a.y;
    const double len = std::hypot(dx, dy);
    const int sx = dx > 0 ? 1 : (dx < 0 ? -1 : 0);
    const int sy = dy > 0 ? 1 : (dy < 0 ? -1 : 0);

    // t runs 0..1 along the segment; tDelta = t needed to cross one whole cell
    const double tDeltaX = sx ? 1.0 / std::abs(dx) : INF;
    const double tDeltaY = sy ? 1.0 / std::abs(dy) : INF;
    double tMaxX = 0.5 * tDeltaX;   // from a centre it is half a cell to the first border
    double tMaxY = 0.5 * tDeltaY;

    int x = a.x, y = a.y;
    double t = 0;
    cost = 0;
    while (true) {
        if (grid.blocked(x, y)) return false;
        const double next = std::min({tMaxX, tMaxY, 1.0});
        cost += (next - t) * len * grid.cost(x, y);
        t = next;
        if (t >= 1.0) return true;

        if (std::abs(tMaxX - tMaxY) < 1e-9) {   // exactly through a corner
            if (grid.blocked(x + sx, y) || grid.blocked(x, y + sy)) return false;
            x += sx; y += sy;
            tMaxX += tDeltaX; tMaxY += tDeltaY;
        } else if (tMaxX < tMaxY) {
            x += sx; tMaxX += tDeltaX;
        } else {
            y += sy; tMaxY += tDeltaY;
        }
    }
}

bool Router::nearestFree(const Grid &grid, Cell c, int maxRadius, Cell &out) {
    int best = -1;  // squared distance of the best cell so far
    for (int r = 0; r <= maxRadius; r++) {
        if (best >= 0 && r * r > best) break;   // every further ring is farther than the best
        for (int dy = -r; dy <= r; dy++)
            for (int dx = -r; dx <= r; dx++) {
                if (std::max(std::abs(dx), std::abs(dy)) != r) continue;   // ring r only
                const int x = c.x + dx, y = c.y + dy;
                if (x < 0 || y < 0 || x >= grid.width() || y >= grid.height()) continue;
                if (grid.blocked(x, y)) continue;
                const int d = dx * dx + dy * dy;
                if (best < 0 || d < best) { best = d; out = {x, y}; }
            }
    }
    return best >= 0;
}

std::vector<Cell> Router::smooth(const Grid &grid, const std::vector<Cell> &path) {
    if (path.size() < 3) return path;

    // cost along the grid path up to each cell (same step rule as findPath)
    std::vector<double> along(path.size(), 0.0);
    for (size_t k = 1; k < path.size(); k++) {
        const bool diagonal = path[k].x != path[k - 1].x && path[k].y != path[k - 1].y;
        along[k] = along[k - 1] + (diagonal ? SQRT2 : 1.0) * grid.cost(path[k].x, path[k].y);
    }

    // greedy: from each kept vertex, go straight as far as the line stays clear and no dearer
    std::vector<Cell> out{ path.front() };
    size_t i = 0;
    while (i + 1 < path.size()) {
        size_t j = i + 1;
        for (size_t k = i + 2; k < path.size(); k++) {
            double c;
            if (lineCost(grid, path[i], path[k], c) && c <= along[k] - along[i] + 1e-9) j = k;
            else break;
        }
        out.push_back(path[j]);
        i = j;
    }
    return out;
}