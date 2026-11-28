#pragma once

#include <QtCore/QString>

struct DownloadTile
{
    /// The database stores the numbers
    enum class TileState
    {
        Pending = 0,
        Downloading,
        Error,
        Complete
    };

    int x = 0;
    int y = 0;
    int z = 0;
    QString hash;
    int mapId = -1;
};

/// What became of a tile that a set was to download
struct DownloadTileState
{
    QString hash;
    DownloadTile::TileState state = DownloadTile::TileState::Pending;
};
