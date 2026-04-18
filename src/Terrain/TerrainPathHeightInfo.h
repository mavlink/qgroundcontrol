#pragma once

#include <QtCore/QList>
#include <QtCore/QMetaType>

struct TerrainPathHeightInfo {
    double          distanceBetween;        ///< Distance between each height value
    double          finalDistanceBetween;   ///< Distance between final two height values
    QList<double>   heights;                ///< Terrain heights along path
};

Q_DECLARE_METATYPE(TerrainPathHeightInfo)
Q_DECLARE_METATYPE(QList<TerrainPathHeightInfo>)
