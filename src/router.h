#pragma once

#include "grid.h"

struct PathResult {
    bool found = false;
    double cost = 0;
    std::vector<Cell> path;
};

class Router {
public:
    static PathResult findPath(const Grid &grid, Cell start, Cell goal);

    // Straight segment between two cell centres: false if it touches a blocked cell
    // (or squeezes between two diagonal ones), otherwise cost = sum of length x cell cost.
    static bool lineCost(const Grid &grid, Cell a, Cell b, double &cost);

    // Removes grid zigzags: replaces runs of cells by straight segments when the segment
    // is clear and costs no more than the grid path it replaces. Returns the vertices kept.
    static std::vector<Cell> smooth(const Grid &grid, const std::vector<Cell> &path);

    // Closest (Euclidean) unblocked cell within maxRadius cells of c. False if there is none.
    static bool nearestFree(const Grid &grid, Cell c, int maxRadius, Cell &out);
};