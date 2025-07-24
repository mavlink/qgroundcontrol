/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QtTypes>
#include <QtCore/QMetaType>

class QGCTileSet
{
public:
    QGCTileSet() = default;
    ~QGCTileSet() = default;

    QGCTileSet &operator+=(const QGCTileSet &other)
    {
        tileX0 += other.tileX0;
        tileX1 += other.tileX1;
        tileY0 += other.tileY0;
        tileY1 += other.tileY1;
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
Q_DECLARE_METATYPE(QGCTileSet)
