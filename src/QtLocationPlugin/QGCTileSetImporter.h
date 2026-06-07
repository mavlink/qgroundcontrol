#pragma once

#include <QtCore/QString>

#include <optional>

class QGCTileCacheDatabase;

/// Imports local offline tile archives (MBTiles, PMTiles) into the QGC SQLite
/// tile cache as a new offline tile set. Tiles are written through the existing
/// QGCTileCacheDatabase::saveTile path so they dedupe against already-cached
/// tiles and are protected from prune like any other offline set.
///
/// Y-axis note: MBTiles store rows TMS-style (origin bottom-left); QGC hashes
/// tiles XYZ-style (origin top-left), so the importer flips the row before
/// hashing: y_xyz = (2^zoom - 1) - tile_row.
namespace QGCTileSetImporter {

struct Result {
    bool success = false;
    QString errorString;
    quint64 setID = 0;
    quint32 tileCount = 0;
};

/// Default provider type imported tiles are hashed/tagged against. The generic
/// custom-URL provider is used because imported archives have no live source.
inline constexpr const char *kDefaultProviderType = "CustomURL Custom";

/// Imports \a archivePath (.mbtiles or .pmtiles, dispatched by extension) into
/// \a db. \a setName is the offline-set name (deduplicated if it collides).
/// \a providerType is the cache provider type used for tile hashing.
Result import(QGCTileCacheDatabase &db, const QString &archivePath, const QString &setName,
              const QString &providerType = QString::fromLatin1(kDefaultProviderType));

/// Imports a standard MBTiles SQLite file (tiles(zoom_level, tile_column, tile_row, tile_data)).
Result importMBTiles(QGCTileCacheDatabase &db, const QString &archivePath, const QString &setName,
                     const QString &providerType);

/// Imports a PMTiles v3 raster archive (https://github.com/protomaps/PMTiles).
/// Tiles flow through the same saveTile path as MBTiles. PMTiles uses XYZ tile
/// addressing, so no Y-flip is applied. Gzip and uncompressed directories/tiles
/// are supported; Brotli/Zstd payloads yield a clear "unsupported codec" error.
Result importPMTiles(QGCTileCacheDatabase &db, const QString &archivePath, const QString &setName,
                     const QString &providerType);

/// Maps a PMTiles (z, x, y) coordinate to its v3 Hilbert-curve tile id.
/// Exposed for testing against the published spec vectors.
quint64 zxyToTileId(quint8 z, quint32 x, quint32 y);

} // namespace QGCTileSetImporter
