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
};