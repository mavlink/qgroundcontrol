#pragma once

#include <QtCore/QtTypes>

/// The tiles of a region at one zoom level: its first and last tile column and row, with the count of its tiles and
/// their estimated size. A sum of regions has the count and the size of all of them, and the columns and rows of the
/// first.
struct TileRegion
{
    TileRegion& operator+=(const TileRegion& other)
    {
        tileCount += other.tileCount;
        tileSize += other.tileSize;
        return *this;
    }

    void clear()
    {
        tileX0 = 0;
        tileX1 = 0;
        tileY0 = 0;
        tileY1 = 0;
        tileCount = 0;
        tileSize = 0;
    }

    int tileX0 = 0;
    int tileX1 = 0;
    int tileY0 = 0;
    int tileY1 = 0;
    quint64 tileCount = 0;
    quint64 tileSize = 0;
};
