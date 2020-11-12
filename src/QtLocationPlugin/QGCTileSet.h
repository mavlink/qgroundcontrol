#pragma once
#include <QString>

//-----------------------------------------------------------------------------
class QGCTileSet
{
public:
    QGCTileSet()
    {
        clear();
    }
    QGCTileSet& operator += (QGCTileSet& other)
    {
        tileX0      += other.tileX0;
        tileX1      += other.tileX1;
        tileY0      += other.tileY0;
        tileY1      += other.tileY1;
        tileCount   += other.tileCount;
        tileSize    += other.tileSize;
        return *this;
    }
    void clear()
    {
        tileX0      = 0;
        tileX1      = 0;
        tileY0      = 0;
        tileY1      = 0;
        tileCount   = 0;
        tileSize    = 0;
    }

    int         tileX0;
    int         tileX1;
    int         tileY0;
    int         tileY1;
    quint64     tileCount;
    quint64     tileSize;
};
