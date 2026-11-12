#pragma once

#include <QtCore/QMetaType>
#include <QtCore/QSize>
#include <QtPositioning/QGeoCoordinate>

struct Viewer3DTileStatistics
{
    QGeoCoordinate coordinateMin;
    QGeoCoordinate coordinateMax;
    QSize tileCounts;
    int zoomLevel = 0;
};

Q_DECLARE_METATYPE(Viewer3DTileStatistics)
