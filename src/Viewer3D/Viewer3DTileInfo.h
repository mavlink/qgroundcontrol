#pragma once

#include <QtCore/QByteArray>

struct Viewer3DTileInfo
{
    QByteArray data;
    int x = 0;
    int y = 0;
    int zoomLevel = 0;
    int mapId = 0;
};
