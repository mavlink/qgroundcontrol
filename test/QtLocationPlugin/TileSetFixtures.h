#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QTemporaryDir>

#include "QGCTileCacheDatabase.h"

/// @file
/// @brief Tile set files that the tests of the tile cache and of its import make

namespace TileSetFixtures {

/// Image of the tile that writeTileSetFile() saves
inline QByteArray tileImage()
{
    return QByteArray(30, 'S');
}

/// Writes a tile set file as the Offline Maps page exports it: the default tile set of a new cache that holds one
/// tile, @a hash
/// @return true if the file was written
inline bool writeTileSetFile(const QString& path, const QString& hash)
{
    QTemporaryDir sourceDir;
    QGCTileCacheDatabase source(sourceDir.filePath(QStringLiteral("source.db")));
    if (!source.init() || !source.connectDB() ||
        !source.saveTile(hash, QStringLiteral("png"), tileImage(), QStringLiteral("Bing Road"),
                         QGCTileCacheDatabase::kInvalidTileSet)) {
        return false;
    }
    const QList<TileSetRecord> sets = source.getTileSets();
    if (sets.isEmpty()) {
        return false;
    }
    TileSetRecord defaultSet = sets.constFirst();
    defaultSet.numTiles = 1;
    return source.exportSets({defaultSet}, path, nullptr).success;
}

}  // namespace TileSetFixtures
