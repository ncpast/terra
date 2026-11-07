#pragma once

#include <vector>
#include <limits>

struct Cell { int x; int y; };

class Grid {
public:
    Grid(int w, int h) : m_w(w), m_h(h), m_cost(w * h, 1.0f) {}
    int width() const { return m_w; }
    int height() const { return m_h; }
    float cost(int x, int y) const { return m_cost[y * m_w + x]; }
    void setCost(int x, int y, float c) { m_cost[y * m_w + x] = c; }
    bool blocked(int x, int y) const {
        return cost(x, y) == std::numeric_limits<float>::infinity();
    }
private:
    int m_w, m_h;
    std::vector<float> m_cost;  // multiplier per cell: 1 = normal, infinity = blocked
};