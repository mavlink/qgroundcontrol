#pragma once

#include <QtCore/QString>

#include <functional>

struct TileSetRecord {
    quint64 setID = 0;
    QString name;
    QString mapTypeStr;
    double topleftLat = 0.;
    double topleftLon = 0.;
    double bottomRightLat = 0.;
    double bottomRightLon = 0.;
    int minZoom = 3;
    int maxZoom = 3;
    int type = -1;
    quint32 numTiles = 0;
    bool defaultSet = false;
    quint64 date = 0;
};

struct TotalsResult {
    quint32 totalCount = 0;
    quint64 totalSize = 0;
    quint32 defaultCount = 0;
    quint64 defaultSize = 0;
};

struct SetTotalsResult {
    quint32 savedTileCount = 0;
    quint64 savedTileSize = 0;
    quint64 totalTileSize = 0;
    quint32 uniqueTileCount = 0;
    quint64 uniqueTileSize = 0;
};

struct DatabaseResult {
    bool success = false;
    QString errorString;
};

using ProgressCallback = std::function<void(int)>;
