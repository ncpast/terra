#ifndef GEOGRID_H
#define GEOGRID_H

#include <algorithm>
#include <cmath>
#include "grid.h"

// A Grid placed on the earth: row 0 is the north edge, column 0 is the west edge.
// Uses a simple local projection, which is accurate enough for areas up to about 30 km.
class GeoGrid {
public:
    GeoGrid(double south, double west, double north, double east, double cellMetres)
        : m_north(north), m_west(west), m_south(south), m_east(east), m_cell(cellMetres),
        m_mPerDegLat(111320.0),
        m_mPerDegLon(111320.0 * std::cos((south + north) / 2.0 * M_PI / 180.0)),
        m_w(std::max(1, int(std::ceil((east - west) * m_mPerDegLon / cellMetres)))),
        m_h(std::max(1, int(std::ceil((north - south) * m_mPerDegLat / cellMetres)))),
        m_grid(m_w, m_h)
    {}

    Grid &grid() { return m_grid; }
    const Grid &grid() const { return m_grid; }
    int width() const { return m_w; }
    int height() const { return m_h; }
    double cellMetres() const { return m_cell; }

    Cell toCell(double lat, double lon) const {
        int x = int(std::floor((lon - m_west) * m_mPerDegLon / m_cell));
        int y = int(std::floor((m_north - lat) * m_mPerDegLat / m_cell));
        return { std::clamp(x, 0, m_w - 1), std::clamp(y, 0, m_h - 1) };
    }

    void toLatLon(Cell c, double &lat, double &lon) const { // centre of the cell
        lat = m_north - (c.y + 0.5) * m_cell / m_mPerDegLat;
        lon = m_west + (c.x + 0.5) * m_cell / m_mPerDegLon;
    }

    // true if the point falls in one of the grid's cells. The last row/column reach a little
    // past south/east (the size is rounded up to whole cells), so this is not a bounds check.
    bool contains(double lat, double lon) const {
        const double x = cellX(lon), y = cellY(lat);
        return x >= 0 && y >= 0 && x < m_w && y < m_h;
    }

    // does the box overlap the grid at all
    bool intersects(double south, double west, double north, double east) const {
        return south <= m_north && north >= m_south && west <= m_east && east >= m_west;
    }

    // fractional cell coordinates, NOT clamped (may lie outside the grid)
    double cellX(double lon) const { return (lon - m_west) * m_mPerDegLon / m_cell; }
    double cellY(double lat) const { return (m_north - lat) * m_mPerDegLat / m_cell; }

private:
    double m_north, m_west, m_south, m_east, m_cell;
    double m_mPerDegLat, m_mPerDegLon;
    int m_w, m_h;
    Grid m_grid;    // declared last: it needs m_w and m_h to be set first
};

#endif