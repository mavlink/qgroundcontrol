#pragma once

#include <functional>

#include <QtCore/QString>

struct TileSetRecord
{
    quint64 setID = 0;
    QString name;
    /// Of the map of the set, as the user sees it
    QString mapName;
    double topleftLat = 0.;
    double topleftLon = 0.;
    double bottomRightLat = 0.;
    double bottomRightLon = 0.;
    int minZoom = 3;
    int maxZoom = 3;
    /// Of the provider of the set (MapProvider::mapId()): -1 for the default set, which has tiles of all
    int mapId = -1;
    quint32 numTiles = 0;
    bool defaultSet = false;
    quint64 date = 0;
};

struct TileCacheTotals
{
    quint32 totalCount = 0;
    quint64 totalSize = 0;
    quint32 defaultCount = 0;
    quint64 defaultSize = 0;
};

struct TileSetTotals
{
    /// The tiles of the set: those it has saved once nothing is left for it to download
    quint32 totalTileCount = 0;
    quint32 savedTileCount = 0;
    quint64 savedTileSize = 0;
    quint64 totalTileSize = 0;
    quint32 uniqueTileCount = 0;
    quint64 uniqueTileSize = 0;
};

struct TileCacheResult
{
    bool success = false;
    QString errorString;
};

using TileCacheProgressCallback = std::function<void(int)>;
