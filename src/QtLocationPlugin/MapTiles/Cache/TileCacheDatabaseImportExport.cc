#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <limits>

#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QScopeGuard>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

#include "MapProvider.h"
#include "MapProviderRegistry.h"
#include "QGCSqlHelper.h"
#include "TileCacheDatabase.h"
#include "WebMercator.h"

namespace {

/// The schema names under which the connection of the cache has the file that it imports or exports, and its own.
/// SQLite attaches and detaches a file outside a transaction only, and detaches none that a statement still reads.
constexpr QLatin1StringView kTileCacheOtherFile("otherFile");
constexpr QLatin1StringView kTileCacheOwnFile("main");

/// Tells \a progressCb which percentage of \a tileCount tiles is copied, when called with the tiles of each chunk
std::function<void(quint64)> tileCacheCopyProgress(quint64 tileCount, TileCacheProgressCallback progressCb)
{
    return [tileCount = std::max<quint64>(tileCount, 1), progressCb = std::move(progressCb), tilesDone = quint64(0),
            progress = -1](quint64 tiles) mutable {
        tilesDone = std::min(tilesDone + tiles, tileCount);
        const int percentage = static_cast<int>((tilesDone * 100) / tileCount);
        if (progressCb && (percentage != progress)) {
            progress = percentage;
            progressCb(percentage);
        }
    };
}

/// Gives \a set the corners that the metadata of an MBTiles file names as "west,south,east,north", in degrees.
/// False for anything else, which leaves \a set as it is.
bool tileCacheSetMBTilesBounds(const QString& bounds, TileSetRecord& set)
{
    const QStringList edges = bounds.split(QLatin1Char(','));
    if (edges.size() != 4) {
        return false;
    }
    std::array<double, 4> degrees;
    for (qsizetype i = 0; i < 4; i++) {
        bool isNumber = false;
        degrees[i] = edges[i].trimmed().toDouble(&isNumber);
        // The range checks below let NaN through, which toDouble() reads from "nan"
        if (!isNumber || !std::isfinite(degrees[i])) {
            return false;
        }
    }
    const auto [west, south, east, north] = degrees;
    if ((west < -180.) || (east > 180.) || (west >= east) || (south < -90.) || (north > 90.) || (south >= north)) {
        return false;
    }
    set.topleftLon = west;
    set.topleftLat = north;
    set.bottomRightLon = east;
    set.bottomRightLat = south;
    return true;
}

}  // namespace

std::optional<int> TileCacheDatabase::_importableVersion(const QString& path, QString& errorString)
{
    std::optional<int> version = std::nullopt;
    const QGCSqlHelper::ScopedConnection versionDB(path, /*readOnly=*/true, QStringLiteral("TileCacheImport"));
    if (versionDB.isValid()) {
        QSqlDatabase db = versionDB.database();
        version = QGCSqlHelper::userVersion(db);
    }
    if (!version) {
        errorString = tr("Error opening import database");
        return std::nullopt;
    }
    // Version 0 cannot be migrated: see _checkSchemaVersion(), which discards a cache of that version
    if (*version < 1) {
        errorString = tr("Import database was written by a version of QGroundControl that is too old");
        return std::nullopt;
    }
    if (*version > kSchemaVersion) {
        errorString = tr("Import database was written by a newer version of QGroundControl");
        return std::nullopt;
    }
    return version;
}

bool TileCacheDatabase::_importPathAllowed(const QString& path, const QStringList& workPaths,
                                           QString& errorString) const
{
    // A file that does not exist has no canonical path, as a cache that does not: it is none of the files below
    const QString source = QFileInfo(path).canonicalFilePath();
    if (source.isEmpty()) {
        return true;
    }
    if (source == QFileInfo(_databasePath).canonicalFilePath()) {
        errorString = tr("Import path must differ from the active database");
        return false;
    }
    for (const QString& workPath : workPaths) {
        if (source == QFileInfo(workPath).canonicalFilePath()) {
            errorString = tr("Import path must differ from the working files of the active database");
            return false;
        }
    }
    return true;
}

TileCacheResult TileCacheDatabase::importSetsReplace(const QString& path, TileCacheProgressCallback progressCb)
{
    TileCacheResult result;
    const QString backupPath = _backupPath();
    const QString copyPath = _importCopyPath();
    if (!_importPathAllowed(path, {backupPath, copyPath}, result.errorString)) {
        return result;
    }
    if (!_importableVersion(path, result.errorString)) {
        return result;
    }

    // The cache stays in place while the file is copied next to it: that is the step that fails for a file that
    // cannot be read to its end, or that the disk has no room for
    const auto removeCopy = qScopeGuard([&copyPath] { _removeDatabaseFiles(copyPath); });
    // What an import that died left of its copy
    _removeDatabaseFiles(copyPath);
    if (!QFile::copy(path, copyPath)) {
        result.errorString = tr("Failed to copy import database");
        return result;
    }
    // The copy has the permissions of the file, which can be read-only
    (void) QFile::setPermissions(copyPath, QFile::permissions(copyPath) | QFile::ReadOwner | QFile::WriteOwner);
    if (progressCb)
        progressCb(25);

    _defaultSet = kInvalidTileSet;
    disconnectDB();
    const bool hasCache = QFileInfo::exists(_databasePath);
    (void) QFile::remove(backupPath);
    const bool hasBackup = hasCache && QFile::rename(_databasePath, backupPath);
    const auto restoreCache = [this, hasBackup, &backupPath] {
        if (hasBackup) {
            (void) QFile::rename(backupPath, _databasePath);
        }
        if (init()) {
            connectDB();
        }
    };
    // An import that failed would have no cache to put back
    if (hasCache && !hasBackup) {
        result.errorString = tr("Failed to back up tile cache database");
        restoreCache();
        return result;
    }
    if (!QFile::rename(copyPath, _databasePath)) {
        result.errorString = tr("Failed to copy import database");
        restoreCache();
        return result;
    }
    // The file is not the cache yet: one that cannot be migrated is not reset, and the cache is put back
    _resetOnMigrationFailure = false;
    const bool imported = init();
    _resetOnMigrationFailure = true;
    if (!imported) {
        result.errorString = tr("Failed to initialize tile cache database after import");
        _removeDatabaseFiles(_databasePath);
        restoreCache();
    } else {
        (void) QFile::remove(backupPath);
        if (progressCb)
            progressCb(50);
        connectDB();
        if (!_valid) {
            result.errorString = tr("Failed to connect to tile cache database after import");
        }
    }
    if (progressCb)
        progressCb(100);
    result.success = result.errorString.isEmpty();
    return result;
}

bool TileCacheDatabase::_attachFile(const QString& path)
{
    QSqlQuery query(_database());
    return _execOrWarn(query, QStringLiteral("ATTACH DATABASE ? AS %1").arg(kTileCacheOtherFile), {path},
                       "attach tile cache file");
}

void TileCacheDatabase::_detachFile()
{
    QSqlQuery query(_database());
    (void) _execOrWarn(query, QStringLiteral("DETACH DATABASE %1").arg(kTileCacheOtherFile), {},
                       "detach tile cache file");
}

std::optional<quint64> TileCacheDatabase::_copySetTiles(QLatin1StringView from, quint64 fromSetID, QLatin1StringView to,
                                                        quint64 toSetID,
                                                        const std::function<void(quint64)>& chunkCopied)
{
    // The tiles of a chunk: those of the set whose id, in the file they come from, is above the last id of the chunk
    // before, up to the id that ends the chunk. The + keeps SQLite from its index of the set ids, with which it would
    // read every link of the set for each chunk: its index of the tile ids gives the links of the chunk alone.
    const QString chunkEndSql = QStringLiteral(
                                    "SELECT tileID FROM %1.SetTiles WHERE +setID = ? AND tileID > ? "
                                    "ORDER BY tileID LIMIT 1 OFFSET ?")
                                    .arg(from);
    const QString chunkSql = QStringLiteral(
                                 "FROM %1.SetTiles S JOIN %1.Tiles T ON T.tileID = S.tileID "
                                 "WHERE +S.setID = ? AND S.tileID > ? AND S.tileID <= ?")
                                 .arg(from);
    const QString copySql = QStringLiteral(
                                "INSERT OR IGNORE INTO %1.Tiles(hash, format, tile, size, type, date) "
                                "SELECT T.hash, T.format, T.tile, T.size, T.type, T.date %2")
                                .arg(to, chunkSql);
    // By their keys: a tile that was not copied, as the file has one under its key, is linked all the same
    const QString linkSql = QStringLiteral(
                                "INSERT OR IGNORE INTO %1.SetTiles(tileID, setID) "
                                "SELECT (SELECT D.tileID FROM %1.Tiles D WHERE D.hash = T.hash), ? %2")
                                .arg(to, chunkSql);

    QSqlQuery query(_database());
    quint64 tilesLeft = 0;
    if (_execBound(query, QStringLiteral("SELECT COUNT(*) FROM %1.SetTiles WHERE setID = ?").arg(from), {fromSetID}) &&
        query.next()) {
        tilesLeft = query.value(0).toULongLong();
    }

    quint64 linked = 0;
    qint64 lastTileID = 0;
    for (bool lastChunk = false; !lastChunk;) {
        // A transaction for each chunk: the write-ahead log holds a transaction whole, so that the disk would have to
        // hold a set twice while it is copied in one
        QGCSqlHelper::Transaction txn(_database());
        if (!txn.ok() ||
            !_execOrWarn(query, chunkEndSql, {fromSetID, lastTileID, kCopyChunkTiles - 1}, "find tiles to copy")) {
            return std::nullopt;
        }
        lastChunk = !query.next();
        const qint64 chunkEnd = lastChunk ? std::numeric_limits<qint64>::max() : query.value(0).toLongLong();

        if (!_execOrWarn(query, copySql, {fromSetID, lastTileID, chunkEnd}, "copy tiles of set") ||
            !_execOrWarn(query, linkSql, {toSetID, fromSetID, lastTileID, chunkEnd}, "link copied tiles to set")) {
            return std::nullopt;
        }
        linked += static_cast<quint64>(std::max(query.numRowsAffected(), 0));
        lastTileID = chunkEnd;
        query.finish();
        if (!txn.commit()) {
            qCWarning(TileCacheDatabaseLog) << "Failed to commit copied tiles of set";
            return std::nullopt;
        }

        const quint64 tiles = lastChunk ? tilesLeft : std::min<quint64>(tilesLeft, kCopyChunkTiles);
        tilesLeft -= tiles;
        if (chunkCopied) {
            chunkCopied(tiles);
        }
    }

    return linked;
}

TileCacheResult TileCacheDatabase::importSetsMerge(const QString& path, TileCacheProgressCallback progressCb)
{
    TileCacheResult result;
    const QString migratedPath = _importCopyPath();
    if (!_importPathAllowed(path, {migratedPath}, result.errorString)) {
        return result;
    }
    if (!_ensureConnected()) {
        result.errorString = tr("Database not connected");
        return result;
    }

    const std::optional<int> importVersion = _importableVersion(path, result.errorString);
    if (!importVersion) {
        return result;
    }

    // An older file is migrated in a copy next to the cache: the file to import stays as it is
    QString importPath = path;
    const auto removeMigrated = qScopeGuard([&migratedPath] { _removeDatabaseFiles(migratedPath); });
    if (*importVersion < kSchemaVersion) {
        // An import that died can have left the write-ahead log of its copy, which SQLite would apply to this one
        _removeDatabaseFiles(migratedPath);
        if (!QFile::copy(path, migratedPath)) {
            result.errorString = tr("Failed to copy import database");
            return result;
        }
        // The copy has the permissions of the file, which can be read-only
        (void) QFile::setPermissions(migratedPath, QFile::ReadOwner | QFile::WriteOwner);

        TileCacheDatabase migrated(migratedPath);
        migrated.setLegacyMapIds(_legacyMapIds);
        migrated.setCacheVariants(_cacheVariants);
        migrated._resetOnMigrationFailure = false;
        if (!migrated.init()) {
            result.errorString = tr("Failed to migrate import database");
            return result;
        }
        importPath = migratedPath;
    }

    if (!_attachFile(importPath)) {
        result.errorString = tr("Error opening import database");
        return result;
    }
    const auto detach = qScopeGuard([this] { _detachFile(); });

    quint64 tileCount = 0;
    {
        QSqlQuery query(_database());
        if (query.exec(QStringLiteral("SELECT COUNT(*) FROM %1.SetTiles").arg(kTileCacheOtherFile)) && query.next()) {
            tileCount = query.value(0).toULongLong();
        }
    }
    const std::function<void(quint64)> chunkCopied = tileCacheCopyProgress(tileCount, std::move(progressCb));

    bool tilesImported = false;
    const QList<TileSetRecord> importedSets = _tileSets(kTileCacheOtherFile);
    for (TileSetRecord set : importedSets) {
        const quint64 importedSetID = set.setID;

        // The tiles of the default set of the file go to that of the cache
        QSqlQuery query(_database());
        quint64 setID = _getDefaultTileSet();
        if (!set.defaultSet) {
            set.name = _deduplicateSetName(set.name);
            set.date = static_cast<quint64>(QDateTime::currentSecsSinceEpoch());
            if (!_insertTileCacheSet(query, set)) {
                result.errorString = tr("Error adding imported tile set to database");
                break;
            }
            setID = query.lastInsertId().toULongLong();
        }

        const std::optional<quint64> tilesLinked =
            _copySetTiles(kTileCacheOtherFile, importedSetID, kTileCacheOwnFile, setID, chunkCopied);
        if (!tilesLinked) {
            // The chunks that were copied go with the set: it is in the cache whole, or not at all. Those of the
            // default set stay, which are tiles of the cache as any other.
            if (!set.defaultSet) {
                (void) deleteTileSet(setID);
            }
            result.errorString = tr("Error copying imported tiles to database");
            break;
        }

        if (*tilesLinked > 0) {
            (void) _execOrWarn(query,
                               QStringLiteral("UPDATE TileSets SET numTiles = "
                                              "(SELECT COUNT(*) FROM SetTiles WHERE setID = ?) WHERE setID = ?"),
                               {setID, setID}, "set tile count of imported set");
            tilesImported = true;
        } else if (!set.defaultSet) {
            qCDebug(TileCacheDatabaseLog) << "No unique tiles in" << set.name << "Removing it.";
            (void) deleteTileSet(setID);
        }
    }

    if (!tilesImported && result.errorString.isEmpty()) {
        result.errorString = tr("No unique tiles in imported database");
    }
    result.success = result.errorString.isEmpty();
    return result;
}

TileCacheResult TileCacheDatabase::exportSets(const QList<TileSetRecord>& sets, const QString& path,
                                              TileCacheProgressCallback progressCb)
{
    TileCacheResult result;
    if (!_ensureConnected()) {
        result.errorString = tr("Database not connected");
        return result;
    }
    if (QFileInfo(path).canonicalFilePath() == QFileInfo(_databasePath).canonicalFilePath()) {
        result.errorString = tr("Export path must differ from the active database");
        return result;
    }

    (void) QFile::remove(path);
    {
        // The file gets its tables through a connection of its own, which _createDB() takes
        const QGCSqlHelper::ScopedConnection exportDB(path, /*readOnly=*/false, QStringLiteral("TileCacheExport"));
        if (!exportDB.isValid()) {
            qCCritical(TileCacheDatabaseLog)
                << "Map Cache SQL error (create export database):" << exportDB.database().lastError();
            result.errorString = tr("Error opening export database");
            return result;
        }
        if (!_createDB(exportDB.database(), false)) {
            result.errorString = tr("Error creating export database");
            return result;
        }
    }
    if (!_attachFile(path)) {
        result.errorString = tr("Error opening export database");
        return result;
    }
    auto detach = qScopeGuard([this] { _detachFile(); });

    quint64 tileCount = 0;
    for (const TileSetRecord& set : sets) {
        QSqlQuery query(_database());
        if (_execBound(query, QStringLiteral("SELECT COUNT(*) FROM SetTiles WHERE setID = ?"), {set.setID}) &&
            query.next()) {
            tileCount += query.value(0).toULongLong();
        }
    }
    const std::function<void(quint64)> chunkCopied = tileCacheCopyProgress(tileCount, std::move(progressCb));

    for (const TileSetRecord& set : sets) {
        QSqlQuery query(_database());
        if (!_insertTileCacheSet(query, set, kTileCacheOtherFile)) {
            result.errorString = tr("Error adding tile set to exported database");
            break;
        }
        if (!_copySetTiles(kTileCacheOwnFile, set.setID, kTileCacheOtherFile, query.lastInsertId().toULongLong(),
                           chunkCopied)) {
            result.errorString = tr("Error reading tile set to export");
            break;
        }
    }

    // Also for sets without a tile, whose export has no chunk to tell of
    chunkCopied(tileCount);
    result.success = result.errorString.isEmpty();
    if (!result.success) {
        // A file with a set that is not whole would look complete: an export that failed leaves no file
        detach.dismiss();
        _detachFile();
        _removeDatabaseFiles(path);
    }
    return result;
}

bool TileCacheDatabase::isMBTiles(const QString& path)
{
    const QGCSqlHelper::ScopedConnection file(path, /*readOnly=*/true, QStringLiteral("TileCacheImport"));
    if (!file.isValid()) {
        return false;
    }
    // Of a table or of a view. A tile cache has a table of this name as well, which has no such column.
    QSqlQuery query(file.database());
    return query.exec(QStringLiteral("SELECT 1 FROM pragma_table_info('tiles') WHERE name = 'tile_data'")) &&
           query.next();
}

TileCacheResult TileCacheDatabase::importMBTiles(const QString& path, const TileSetRecord& set, bool replace,
                                                 TileCacheProgressCallback progressCb)
{
    TileCacheResult result;
    if (!_ensureConnected()) {
        result.errorString = tr("Database not connected");
        return result;
    }
    const SharedMapProvider provider = MapProviderRegistry::getMapProviderFromMapId(set.mapId);
    if (!provider) {
        result.errorString = tr("No map to import the tiles to");
        return result;
    }

    // Read through a connection of its own: the tiles of a file can be a view, which has no ids to copy chunks by
    const QGCSqlHelper::ScopedConnection file(path, /*readOnly=*/true, QStringLiteral("TileCacheImport"));
    if (!file.isValid()) {
        result.errorString = tr("Error opening import database");
        return result;
    }
    QSqlQuery tiles(file.database());
    tiles.setForwardOnly(true);

    QString bounds;
    if (tiles.exec(QStringLiteral("SELECT name, value FROM metadata WHERE name IN ('bounds', 'format')"))) {
        while (tiles.next()) {
            const QString value = tiles.value(1).toString();
            if (tiles.value(0).toString() == QLatin1StringView("bounds")) {
                bounds = value;
            } else if ((value == QLatin1StringView("pbf")) || (value == QLatin1StringView("mvt"))) {
                result.errorString = tr("The file has vector tiles, which cannot be shown");
                return result;
            }
        }
    }

    // A tile of the row that \a tiles is at, with the row of the tile counted from the north as the cache counts it.
    // No format for what is no tile of a map.
    struct MBTile
    {
        int zoom = 0;
        int x = 0;
        int y = 0;
        QByteArray image;
        QString format;
    };

    const QString tilesSql = QStringLiteral("SELECT zoom_level, tile_column, tile_row, tile_data FROM tiles");
    const auto readTile = [&tiles] {
        MBTile tile;
        tile.zoom = tiles.value(0).toInt();
        tile.x = tiles.value(1).toInt();
        const int row = tiles.value(2).toInt();
        const int tilesAcross = ((tile.zoom >= 0) && (tile.zoom <= QGC_MAX_MAP_ZOOM)) ? (1 << tile.zoom) : 0;
        if ((tile.x >= 0) && (tile.x < tilesAcross) && (row >= 0) && (row < tilesAcross)) {
            // MBTiles counts the rows from the south
            tile.y = tilesAcross - 1 - row;
            tile.image = tiles.value(3).toByteArray();
            // By the tile itself: the metadata need not name a format, and a file can have tiles of more than one
            tile.format = MapProvider::imageFormatFromSignature(tile.image);
        }
        return tile;
    };

    // Before the cache is written, which \a replace empties: a file of pictures has one in its first row
    bool hasTile = false;
    if (tiles.exec(tilesSql)) {
        while (!hasTile && tiles.next()) {
            hasTile = !readTile().format.isEmpty();
        }
    }
    if (!hasTile) {
        result.errorString = tr("The file has no map tiles that can be shown");
        return result;
    }

    quint64 tileCount = 0;
    if (tiles.exec(QStringLiteral("SELECT COUNT(*) FROM tiles")) && tiles.next()) {
        tileCount = tiles.value(0).toULongLong();
    }
    const std::function<void(quint64)> tileRead = tileCacheCopyProgress(tileCount, std::move(progressCb));

    const qint64 now = QDateTime::currentSecsSinceEpoch();
    TileSetRecord record = set;
    record.defaultSet = false;
    record.date = static_cast<quint64>(now);
    record.minZoom = QGC_MAX_MAP_ZOOM;
    record.maxZoom = 0;
    record.numTiles = 0;
    QSqlQuery query(_database());
    {
        // The set takes the place of what the cache has, or the cache stays as it is. The links and the downloads of
        // the sets go with them, and the default set stays, without its tiles.
        QGCSqlHelper::Transaction txn(_database());
        if (!txn.ok() || (replace && (!_execOrWarn(query, QStringLiteral("DELETE FROM TileSets WHERE defaultSet = 0"),
                                                   {}, "delete tile sets") ||
                                      !_execOrWarn(query, QStringLiteral("DELETE FROM Tiles"), {}, "delete tiles")))) {
            result.errorString = tr("Error emptying database for import");
            return result;
        }
        record.name = _deduplicateSetName(set.name);
        if (!_insertTileCacheSet(query, record) || !txn.commit()) {
            result.errorString = tr("Error adding imported tile set to database");
            return result;
        }
    }
    const qint64 setID = query.lastInsertId().toLongLong();

    // The columns and rows of the tiles of a zoom level
    struct TileBox
    {
        int x0 = std::numeric_limits<int>::max();
        int x1 = -1;
        int y0 = std::numeric_limits<int>::max();
        int y1 = -1;
    };

    std::array<TileBox, QGC_MAX_MAP_ZOOM + 1> boxes;
    // The set as it is with the tiles imported so far
    const auto saveSet = [&query, &record, &boxes, &bounds, setID] {
        if (!tileCacheSetMBTilesBounds(bounds, record)) {
            // The area of the deepest tiles, which is the closest to that of the map
            const TileBox& deepest = boxes[record.maxZoom];
            const double tilesAcross = 1 << record.maxZoom;
            record.topleftLon = WebMercator::longitudeOfX(deepest.x0 / tilesAcross);
            record.topleftLat = WebMercator::latitudeOfY(deepest.y0 / tilesAcross);
            record.bottomRightLon = WebMercator::longitudeOfX((deepest.x1 + 1) / tilesAcross);
            record.bottomRightLat = WebMercator::latitudeOfY((deepest.y1 + 1) / tilesAcross);
        }
        return _execOrWarn(query,
                           QStringLiteral("UPDATE TileSets SET topleftLat = ?, topleftLon = ?, bottomRightLat = ?, "
                                          "bottomRightLon = ?, minZoom = ?, maxZoom = ?, numTiles = ? WHERE setID = ?"),
                           {record.topleftLat, record.topleftLon, record.bottomRightLat, record.bottomRightLon,
                            record.minZoom, record.maxZoom, record.numTiles, setID},
                           "set area of imported set");
    };

    QSqlQuery saveTile(_database());
    QSqlQuery linkTile(_database());
    bool imported = saveTile.prepare(QLatin1StringView(kSaveTileSql)) &&
                    linkTile.prepare(QStringLiteral(
                        "INSERT OR IGNORE INTO SetTiles(tileID, setID) SELECT tileID, ? FROM Tiles WHERE hash = ?")) &&
                    tiles.exec(tilesSql);
    {
        // A transaction for each chunk of tiles: the write-ahead log holds a transaction whole, so that the disk
        // would have to hold the map twice while it is imported in one
        std::optional<QGCSqlHelper::Transaction> txn = std::nullopt;
        int chunkTiles = 0;
        while (imported && tiles.next()) {
            tileRead(1);
            const MBTile tile = readTile();
            if (tile.format.isEmpty()) {
                continue;
            }
            if (!txn) {
                imported = txn.emplace(_database()).ok();
            }
            const QString hash = provider->getTileHash(tile.x, tile.y, tile.zoom, QStringView());
            saveTile.bindValue(0, hash);
            saveTile.bindValue(1, tile.format);
            saveTile.bindValue(2, tile.image);
            saveTile.bindValue(3, tile.image.size());
            saveTile.bindValue(4, set.mapId);
            saveTile.bindValue(5, now);
            linkTile.bindValue(0, setID);
            linkTile.bindValue(1, hash);
            imported = imported && saveTile.exec() && linkTile.exec();
            if (!imported) {
                break;
            }
            record.numTiles += static_cast<quint32>(std::max(linkTile.numRowsAffected(), 0));
            record.minZoom = std::min(record.minZoom, tile.zoom);
            record.maxZoom = std::max(record.maxZoom, tile.zoom);
            TileBox& box = boxes[tile.zoom];
            box.x0 = std::min(box.x0, tile.x);
            box.x1 = std::max(box.x1, tile.x);
            box.y0 = std::min(box.y0, tile.y);
            box.y1 = std::max(box.y1, tile.y);

            if (++chunkTiles == kCopyChunkTiles) {
                chunkTiles = 0;
                imported = saveSet() && txn->commit();
                txn.reset();
            }
        }
        // A row that cannot be read ends the rows as the last one does
        imported = imported && !tiles.lastError().isValid();
        if (imported && txn) {
            imported = saveSet() && txn->commit();
        }
    }
    if (!imported || (record.numTiles == 0)) {
        qCWarning(TileCacheDatabaseLog) << "Map Cache SQL error (import MBTiles):" << saveTile.lastError().text()
                                        << linkTile.lastError().text() << tiles.lastError().text();
        // The chunks that were imported go with the set: it is in the cache whole, or not at all. A tile of a file
        // that was imported before stays as this file replaced it.
        (void) deleteTileSet(static_cast<quint64>(setID));
        result.errorString = tr("Error saving imported tiles to database");
        return result;
    }

    result.success = true;
    return result;
}
