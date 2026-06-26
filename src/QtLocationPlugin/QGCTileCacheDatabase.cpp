#include "QGCTileCacheDatabase.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QSet>
#include <QtCore/QSettings>
#include <QtCore/QUuid>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>
#include <atomic>

#include "QGCCacheTile.h"
#include "QGCLoggingCategory.h"
#include "QGCMapUrlEngine.h"
#include "QGCSqlHelper.h"
#include "QGCTile.h"
#include "QGCTileDatabaseSchema.h"
#include "QGCTileSet.h"

QGC_LOGGING_CATEGORY(QGCTileCacheDatabaseLog, "QtLocationPlugin.QGCTileCacheDatabase")

static std::atomic<quint64> s_connectionCounter{0};

QGCTileCacheDatabase::QGCTileCacheDatabase(const QString& databasePath)
    : _databasePath(databasePath),
      _connectionName(QStringLiteral("QGCTileCache_%1").arg(s_connectionCounter.fetch_add(1)))
{}

QGCTileCacheDatabase::~QGCTileCacheDatabase()
{
    disconnectDB();
}

QSqlDatabase QGCTileCacheDatabase::_database() const
{
    return QSqlDatabase::database(_connectionName);
}

QSqlDatabase QGCTileCacheDatabase::database() const
{
    return _database();
}

bool QGCTileCacheDatabase::_ensureConnected() const
{
    if (!_connected || !_valid) {
        qCWarning(QGCTileCacheDatabaseLog) << "Database not connected";
        return false;
    }
    return true;
}

bool QGCTileCacheDatabase::init(bool keepConnected)
{
    _failed = false;
    if (!_databasePath.isEmpty()) {
        qCDebug(QGCTileCacheDatabaseLog) << "Mapping cache directory:" << _databasePath;
        if (connectDB()) {
            bool didReset = false;
            if (!QGCTileDatabaseSchema::checkSchemaVersion(_database(), &didReset)) {
                _failed = true;
                disconnectDB();
                return false;
            }
            if (didReset) {
                _defaultSet = kInvalidTileSet;
            }
            _valid = QGCTileDatabaseSchema::createSchema(_database());
            if (!_valid) {
                _failed = true;
                (void) QFile::remove(_databasePath);
            }
        } else {
            _failed = true;
        }
        // keepConnected lets the import path reuse the connection init() opened
        // instead of disconnecting and immediately reconnecting.
        if (!keepConnected || _failed) {
            disconnectDB();
        }
    } else {
        qCCritical(QGCTileCacheDatabaseLog) << "Could not find suitable cache directory.";
        _failed = true;
    }

    return !_failed;
}

bool QGCTileCacheDatabase::connectDB()
{
    if (_connected) {
        disconnectDB();
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", _connectionName);
    db.setDatabaseName(_databasePath);
    // Block (up to 5s) rather than fail with SQLITE_BUSY when the WAL writer lock is
    // contended across the worker thread and other tile-cache connections.
    db.setConnectOptions(QStringLiteral("QSQLITE_BUSY_TIMEOUT=5000"));
    _valid = db.open();
    if (_valid) {
        // The tile cache is large and read-heavy — opt into a 256 MB mmap window, an
        // 8 KiB page size (better BLOB locality) and a 64 MiB page cache for bulk import.
        QGCSqlHelper::applySqlitePragmas(db, 256LL * 1024 * 1024, -65536, 8192);
        _connected = true;
    } else {
        qCCritical(QGCTileCacheDatabaseLog) << "Map Cache SQL error (open db):" << db.lastError();
        QSqlDatabase::removeDatabase(_connectionName);
    }
    return _valid;
}

void QGCTileCacheDatabase::disconnectDB()
{
    if (!_connected) {
        return;
    }
    _connected = false;

    if (!QCoreApplication::instance()) {
        return;
    }

    {
        QSqlDatabase db = QSqlDatabase::database(_connectionName, false);
        if (db.isOpen()) {
            QGCSqlHelper::runOptimize(db);
            db.close();
        }
    }
    QSqlDatabase::removeDatabase(_connectionName);
}

static QVariant _validatorOrNull(const QByteArray& value)
{
    return value.isEmpty() ? QVariant(QMetaType(QMetaType::QString)) : QVariant(QString::fromLatin1(value));
}

bool QGCTileCacheDatabase::saveTile(const QString& hash, const QString& format, const QByteArray& img,
                                    const QString& type, quint64 tileSet, const QByteArray& etag,
                                    const QByteArray& lastModified, qint64 expiresAt, bool mustRevalidate)
{
    if (!_ensureConnected()) {
        return false;
    }

    QGCSqlHelper::Transaction txn(_database());
    if (!txn.ok()) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to start transaction for saveTile";
        return false;
    }

    const qint64 nowSecs = QDateTime::currentSecsSinceEpoch();
    QSqlQuery query(_database());
    if (!QGCSqlHelper::execPrepared(
            query,
            "INSERT OR IGNORE INTO Tiles(hash, format, tile, size, typeStr, date, etag, lastModified, expiresAt, "
            "accessed, mustRevalidate) VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
            hash, format, img, img.size(), type, nowSecs, _validatorOrNull(etag), _validatorOrNull(lastModified),
            expiresAt, QDateTime::currentMSecsSinceEpoch(), mustRevalidate ? 1 : 0)) {
        qCWarning(QGCTileCacheDatabaseLog) << "Map Cache SQL error (saveTile INSERT):" << query.lastError().text();
        return false;
    }

    if (!QGCSqlHelper::execPrepared(query, "SELECT tileID FROM Tiles WHERE hash = ?", hash) || !query.next()) {
        qCWarning(QGCTileCacheDatabaseLog) << "Map Cache SQL error (tile lookup):" << query.lastError().text();
        return false;
    }
    const quint64 tileID = query.value(0).toULongLong();

    const quint64 setID = (tileSet == kInvalidTileSet) ? _getDefaultTileSet() : tileSet;
    if (setID == kInvalidTileSet) {
        qCWarning(QGCTileCacheDatabaseLog) << "Cannot save tile: no valid tile set";
        return false;
    }
    if (!QGCSqlHelper::execPrepared(query, "INSERT OR IGNORE INTO SetTiles(tileID, setID) VALUES(?, ?)", tileID,
                                    setID)) {
        qCWarning(QGCTileCacheDatabaseLog)
            << "Map Cache SQL error (add tile into SetTiles):" << query.lastError().text();
        return false;
    }

    if (!txn.commit()) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to commit saveTile transaction";
        return false;
    }

    qCDebug(QGCTileCacheDatabaseLog) << "HASH:" << hash;
    return true;
}

bool QGCTileCacheDatabase::saveTileBatch(const QList<const QGCCacheTile*>& tiles)
{
    if (tiles.isEmpty()) {
        return true;
    }

    if (!_ensureConnected()) {
        return false;
    }

    QGCSqlHelper::Transaction txn(_database());
    if (!txn.ok()) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to start transaction for saveTileBatch";
        return false;
    }

    QSqlQuery insertQuery(_database());
    if (!insertQuery.prepare("INSERT OR IGNORE INTO Tiles(hash, format, tile, size, typeStr, date, etag, lastModified, "
                             "expiresAt, accessed, mustRevalidate) VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)")) {
        qCWarning(QGCTileCacheDatabaseLog)
            << "Map Cache SQL error (prepare saveTileBatch insert):" << insertQuery.lastError().text();
        return false;
    }

    QSqlQuery lookupQuery(_database());
    if (!lookupQuery.prepare("SELECT tileID FROM Tiles WHERE hash = ?")) {
        qCWarning(QGCTileCacheDatabaseLog)
            << "Map Cache SQL error (prepare saveTileBatch lookup):" << lookupQuery.lastError().text();
        return false;
    }

    QSqlQuery linkQuery(_database());
    if (!linkQuery.prepare("INSERT OR IGNORE INTO SetTiles(tileID, setID) VALUES(?, ?)")) {
        qCWarning(QGCTileCacheDatabaseLog)
            << "Map Cache SQL error (prepare saveTileBatch link):" << linkQuery.lastError().text();
        return false;
    }

    const quint64 defaultSetID = _getDefaultTileSet();
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();

    for (const QGCCacheTile* tile : tiles) {
        if (!tile) {
            continue;
        }

        insertQuery.addBindValue(tile->hash);
        insertQuery.addBindValue(tile->format);
        insertQuery.addBindValue(tile->img);
        insertQuery.addBindValue(tile->img.size());
        insertQuery.addBindValue(tile->type);
        insertQuery.addBindValue(now);
        insertQuery.addBindValue(_validatorOrNull(tile->etag));
        insertQuery.addBindValue(_validatorOrNull(tile->lastModified));
        insertQuery.addBindValue(tile->expiresAt);
        insertQuery.addBindValue(nowMs);
        insertQuery.addBindValue(tile->mustRevalidate ? 1 : 0);
        if (!insertQuery.exec()) {
            qCWarning(QGCTileCacheDatabaseLog)
                << "Map Cache SQL error (saveTileBatch INSERT):" << insertQuery.lastError().text();
            return false;
        }

        quint64 tileID = 0;
        if (insertQuery.numRowsAffected() > 0) {
            tileID = insertQuery.lastInsertId().toULongLong();
        } else {
            lookupQuery.addBindValue(tile->hash);
            if (!lookupQuery.exec() || !lookupQuery.next()) {
                qCWarning(QGCTileCacheDatabaseLog)
                    << "Map Cache SQL error (saveTileBatch lookup):" << lookupQuery.lastError().text();
                return false;
            }
            tileID = lookupQuery.value(0).toULongLong();
            lookupQuery.finish();
        }

        const quint64 setID = (tile->tileSet == kInvalidTileSet) ? defaultSetID : tile->tileSet;
        if (setID == kInvalidTileSet) {
            qCWarning(QGCTileCacheDatabaseLog) << "Cannot save tile: no valid tile set";
            return false;
        }

        linkQuery.addBindValue(tileID);
        linkQuery.addBindValue(setID);
        if (!linkQuery.exec()) {
            qCWarning(QGCTileCacheDatabaseLog)
                << "Map Cache SQL error (saveTileBatch link):" << linkQuery.lastError().text();
            return false;
        }
    }

    if (!txn.commit()) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to commit saveTileBatch transaction";
        return false;
    }

    return true;
}

bool QGCTileCacheDatabase::refreshTileValidators(const QString& hash, const QByteArray& etag,
                                                 const QByteArray& lastModified, qint64 expiresAt)
{
    if (!_ensureConnected()) {
        return false;
    }

    QSqlQuery query(_database());
    if (!QGCSqlHelper::execPrepared(
            query, "UPDATE Tiles SET etag = ?, lastModified = ?, expiresAt = ?, date = ? WHERE hash = ?",
            _validatorOrNull(etag), _validatorOrNull(lastModified), expiresAt, QDateTime::currentSecsSinceEpoch(),
            hash)) {
        qCWarning(QGCTileCacheDatabaseLog)
            << "Map Cache SQL error (refreshTileValidators):" << query.lastError().text();
        return false;
    }

    return true;
}

std::unique_ptr<QGCCacheTile> QGCTileCacheDatabase::getTile(const QString& hash)
{
    if (!_ensureConnected()) {
        return nullptr;
    }

    QSqlQuery query(_database());
    if (QGCSqlHelper::execPrepared(
            query,
            "SELECT tile, format, typeStr, etag, lastModified, expiresAt, mustRevalidate FROM Tiles WHERE hash = ?",
            hash) &&
        query.next()) {
        const QByteArray tileData = query.value(0).toByteArray();
        const QString format = query.value(1).toString();
        const QString type = query.value(2).toString();
        auto tile = std::make_unique<QGCCacheTile>(hash, tileData, format, type);
        tile->etag = query.value(3).toString().toLatin1();
        tile->lastModified = query.value(4).toString().toLatin1();
        tile->expiresAt = query.value(5).toLongLong();
        tile->mustRevalidate = (query.value(6).toInt() != 0);
        qCDebug(QGCTileCacheDatabaseLog) << "(Found in DB) HASH:" << hash;
        return tile;
    }

    qCDebug(QGCTileCacheDatabaseLog) << "(NOT in DB) HASH:" << hash;
    return nullptr;
}

std::optional<quint64> QGCTileCacheDatabase::findTile(const QString& hash)
{
    if (!_ensureConnected()) {
        return std::nullopt;
    }

    QSqlQuery query(_database());
    if (QGCSqlHelper::execPrepared(query, "SELECT tileID FROM Tiles WHERE hash = ?", hash) && query.next()) {
        return query.value(0).toULongLong();
    }

    return std::nullopt;
}

bool QGCTileCacheDatabase::bumpTileAccessed(const QString& hash)
{
    if (!_ensureConnected()) {
        return false;
    }

    QSqlQuery query(_database());
    if (!QGCSqlHelper::execPrepared(query, "UPDATE Tiles SET accessed = ? WHERE hash = ?",
                                    QDateTime::currentMSecsSinceEpoch(), hash)) {
        qCWarning(QGCTileCacheDatabaseLog) << "Map Cache SQL error (bumpTileAccessed):" << query.lastError().text();
        return false;
    }

    return true;
}

QList<TileSetRecord> QGCTileCacheDatabase::getTileSets()
{
    QList<TileSetRecord> records;
    if (!_ensureConnected()) {
        return records;
    }

    QSqlQuery query(_database());
    query.setForwardOnly(true);
    if (!query.exec("SELECT setID, name, typeStr, topleftLat, topleftLon, bottomRightLat, bottomRightLon, "
                    "minZoom, maxZoom, numTiles, defaultSet, date "
                    "FROM TileSets ORDER BY defaultSet DESC, name ASC")) {
        return records;
    }

    while (query.next()) {
        TileSetRecord rec;
        rec.setID = query.value(0).toULongLong();
        rec.name = query.value(1).toString();
        rec.mapTypeStr = query.value(2).toString();
        rec.topleftLat = query.value(3).toDouble();
        rec.topleftLon = query.value(4).toDouble();
        rec.bottomRightLat = query.value(5).toDouble();
        rec.bottomRightLon = query.value(6).toDouble();
        rec.minZoom = query.value(7).toInt();
        rec.maxZoom = query.value(8).toInt();
        rec.numTiles = query.value(9).toUInt();
        rec.defaultSet = (query.value(10).toInt() != 0);
        rec.date = query.value(11).toULongLong();
        records.append(rec);
    }

    return records;
}

std::optional<quint64> QGCTileCacheDatabase::createImportedTileSet(const QString& name, const QString& type,
                                                                   int minZoom, int maxZoom)
{
    if (!_ensureConnected()) {
        return std::nullopt;
    }

    const QString uniqueName = _deduplicateSetName(name);

    QSqlQuery query(_database());
    if (!QGCSqlHelper::execPrepared(
            query, "INSERT INTO TileSets(name, typeStr, minZoom, maxZoom, numTiles, date) VALUES(?, ?, ?, ?, ?, ?)",
            uniqueName, type, minZoom, maxZoom, 0, QDateTime::currentSecsSinceEpoch())) {
        qCWarning(QGCTileCacheDatabaseLog)
            << "Map Cache SQL error (createImportedTileSet):" << query.lastError().text();
        return std::nullopt;
    }

    return query.lastInsertId().toULongLong();
}

std::optional<quint64> QGCTileCacheDatabase::createTileSet(const QString& name, const QString& mapTypeStr,
                                                           double topleftLat, double topleftLon, double bottomRightLat,
                                                           double bottomRightLon, int minZoom, int maxZoom,
                                                           const QString& type, quint32 numTiles)
{
    if (!_ensureConnected()) {
        return std::nullopt;
    }

    QGCSqlHelper::Transaction txn(_database());
    if (!txn.ok()) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to start transaction for createTileSet";
        return std::nullopt;
    }

    QSqlQuery query(_database());
    if (!QGCSqlHelper::execPrepared(query,
                                    "INSERT INTO TileSets("
                                    "name, typeStr, topleftLat, topleftLon, bottomRightLat, bottomRightLon, minZoom, "
                                    "maxZoom, numTiles, date"
                                    ") VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
                                    name, mapTypeStr, topleftLat, topleftLon, bottomRightLat, bottomRightLon, minZoom,
                                    maxZoom, numTiles, QDateTime::currentSecsSinceEpoch())) {
        qCWarning(QGCTileCacheDatabaseLog)
            << "Map Cache SQL error (add tileSet into TileSets):" << query.lastError().text();
        return std::nullopt;
    }

    const quint64 setID = query.lastInsertId().toULongLong();

    // Process tiles in streaming batches to avoid holding all coordinates in memory
    constexpr int kHashBatchSize = 500;

    struct TileCoord
    {
        int x, y;
        QString hash;
    };

    QSqlQuery insertSetTile(_database());
    if (!insertSetTile.prepare(QStringLiteral("INSERT OR IGNORE INTO SetTiles(tileID, setID) VALUES(?, ?)"))) {
        return std::nullopt;
    }
    QSqlQuery insertDownload(_database());
    if (!insertDownload.prepare(QStringLiteral(
            "INSERT OR IGNORE INTO TilesDownload(setID, hash, typeStr, x, y, z, state) VALUES(?, ?, ?, ?, ?, ?, ?)"))) {
        return std::nullopt;
    }

    auto processBatch = [&](const QList<TileCoord>& tiles, int z) -> bool {
        QHash<QString, quint64> existingTiles;
        QSqlQuery lookup(_database());
        lookup.setForwardOnly(true);
        if (!lookup.prepare(QStringLiteral("SELECT hash, tileID FROM Tiles WHERE hash IN (%1)")
                                .arg(QGCSqlHelper::placeholders(tiles.size())))) {
            qCWarning(QGCTileCacheDatabaseLog)
                << "Map Cache SQL error (prepare existing-tile lookup):" << lookup.lastError().text();
            return false;
        }
        for (const auto& tc : tiles) {
            lookup.addBindValue(tc.hash);
        }
        if (!lookup.exec()) {
            qCWarning(QGCTileCacheDatabaseLog)
                << "Map Cache SQL error (exec existing-tile lookup):" << lookup.lastError().text();
            return false;
        }
        while (lookup.next()) {
            existingTiles.insert(lookup.value(0).toString(), lookup.value(1).toULongLong());
        }

        for (const auto& tc : tiles) {
            auto it = existingTiles.find(tc.hash);
            if (it != existingTiles.end()) {
                insertSetTile.bindValue(0, it.value());
                insertSetTile.bindValue(1, setID);
                if (!insertSetTile.exec()) {
                    qCWarning(QGCTileCacheDatabaseLog)
                        << "Map Cache SQL error (add tile into SetTiles):" << insertSetTile.lastError().text();
                    return false;
                }
            } else {
                insertDownload.bindValue(0, setID);
                insertDownload.bindValue(1, tc.hash);
                insertDownload.bindValue(2, type);
                insertDownload.bindValue(3, tc.x);
                insertDownload.bindValue(4, tc.y);
                insertDownload.bindValue(5, z);
                insertDownload.bindValue(6, static_cast<int>(QGCTile::StatePending));
                if (!insertDownload.exec()) {
                    qCWarning(QGCTileCacheDatabaseLog)
                        << "Map Cache SQL error (add tile into TilesDownload):" << insertDownload.lastError().text();
                    return false;
                }
            }
        }
        return true;
    };

    for (int z = minZoom; z <= maxZoom; z++) {
        const QGCTileSet set =
            UrlFactory::getTileCount(z, topleftLon, topleftLat, bottomRightLon, bottomRightLat, type);

        QList<TileCoord> batch;
        batch.reserve(kHashBatchSize);

        for (int x = set.tileX0; x <= set.tileX1; x++) {
            for (int y = set.tileY0; y <= set.tileY1; y++) {
                batch.append({x, y, UrlFactory::getTileHash(type, x, y, z)});

                if (batch.size() >= kHashBatchSize) {
                    if (!processBatch(batch, z))
                        return std::nullopt;
                    batch.clear();
                }
            }
        }

        if (!batch.isEmpty()) {
            if (!processBatch(batch, z))
                return std::nullopt;
        }
    }

    if (!txn.commit()) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to commit createTileSet transaction";
        return std::nullopt;
    }

    return setID;
}

bool QGCTileCacheDatabase::deleteTileSet(quint64 id)
{
    if (!_ensureConnected()) {
        return false;
    }

    QGCSqlHelper::Transaction txn(_database());
    if (!txn.ok()) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to start transaction for deleteTileSet";
        return false;
    }

    QSqlQuery query(_database());

    // Delete download queue entries first
    if (!QGCSqlHelper::execPrepared(query, "DELETE FROM TilesDownload WHERE setID = ?", id)) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to delete download queue:" << query.lastError().text();
        return false;
    }

    // Find tiles unique to this set (not shared with other sets)
    // Must collect IDs before deleting SetTiles links
    QList<quint64> uniqueTileIDs;
    if (QGCSqlHelper::execPrepared(
            query, QStringLiteral("SELECT tileID FROM SetTiles WHERE tileID IN (%1)").arg(kUniqueTilesSubquery), id)) {
        while (query.next()) {
            uniqueTileIDs.append(query.value(0).toULongLong());
        }
    }

    // Remove set-tile links
    if (!QGCSqlHelper::execPrepared(query, "DELETE FROM SetTiles WHERE setID = ?", id)) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to delete SetTiles links:" << query.lastError().text();
        return false;
    }

    // Delete unique tiles (no longer referenced by any set)
    if (!uniqueTileIDs.isEmpty()) {
        if (query.prepare(QStringLiteral("DELETE FROM Tiles WHERE tileID IN (%1)")
                              .arg(QGCSqlHelper::placeholders(uniqueTileIDs.size())))) {
            for (const quint64 tileID : uniqueTileIDs) {
                query.addBindValue(tileID);
            }
            if (!query.exec()) {
                qCWarning(QGCTileCacheDatabaseLog) << "Failed to delete unique tiles:" << query.lastError().text();
                return false;
            }
        }
    }

    // Delete the tile set itself
    if (!QGCSqlHelper::execPrepared(query, "DELETE FROM TileSets WHERE setID = ?", id)) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to delete TileSet:" << query.lastError().text();
        return false;
    }

    if (id == _defaultSet) {
        _defaultSet = kInvalidTileSet;
    }

    return txn.commit();
}

bool QGCTileCacheDatabase::renameTileSet(quint64 setID, const QString& newName)
{
    if (!_ensureConnected()) {
        return false;
    }

    QSqlQuery query(_database());
    return QGCSqlHelper::execPrepared(query, "UPDATE TileSets SET name = ? WHERE setID = ?", newName, setID);
}

std::optional<quint64> QGCTileCacheDatabase::findTileSetID(const QString& name)
{
    if (!_ensureConnected()) {
        return std::nullopt;
    }

    QSqlQuery query(_database());
    if (QGCSqlHelper::execPrepared(query, "SELECT setID FROM TileSets WHERE name = ?", name) && query.next()) {
        return query.value(0).toULongLong();
    }

    return std::nullopt;
}

bool QGCTileCacheDatabase::resetDatabase()
{
    if (!_ensureConnected()) {
        return false;
    }

    _defaultSet = kInvalidTileSet;

    if (!QGCTileDatabaseSchema::dropSchemaTables(_database())) {
        return false;
    }
    _valid = QGCTileDatabaseSchema::createSchema(_database());
    return _valid;
}

QList<QGCTile> QGCTileCacheDatabase::getTileDownloadList(quint64 setID, int count)
{
    QList<QGCTile> tiles;
    if (!_ensureConnected()) {
        return tiles;
    }

    QGCSqlHelper::Transaction txn(_database());
    if (!txn.ok()) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to start transaction for getTileDownloadList";
        return tiles;
    }

    QSqlQuery query(_database());
    if (!QGCSqlHelper::execPrepared(
            query, "SELECT hash, typeStr, x, y, z FROM TilesDownload WHERE setID = ? AND state = ? LIMIT ?", setID,
            static_cast<int>(QGCTile::StatePending), count)) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to query tile download list:" << query.lastError().text();
        return tiles;
    }

    while (query.next()) {
        QGCTile tile;
        tile.hash = query.value(0).toString();
        tile.type = query.value(1).toString();
        tile.x = query.value(2).toInt();
        tile.y = query.value(3).toInt();
        tile.z = query.value(4).toInt();
        tiles.append(std::move(tile));
    }

    if (!tiles.isEmpty()) {
        if (!query.prepare(QStringLiteral("UPDATE TilesDownload SET state = ? WHERE setID = ? AND hash IN (%1)")
                               .arg(QGCSqlHelper::placeholders(tiles.size())))) {
            // Returning the tiles without marking them StateDownloading would re-hand
            // them out on the next call (duplicate downloads), so fail closed.
            qCWarning(QGCTileCacheDatabaseLog)
                << "Failed to prepare TilesDownload state update:" << query.lastError().text();
            tiles.clear();
            return tiles;
        }
        query.addBindValue(static_cast<int>(QGCTile::StateDownloading));
        query.addBindValue(setID);
        for (qsizetype i = 0; i < tiles.size(); i++) {
            query.addBindValue(tiles[i].hash);
        }
        if (!query.exec()) {
            qCWarning(QGCTileCacheDatabaseLog)
                << "Map Cache SQL error (batch set TilesDownload state):" << query.lastError().text();
            tiles.clear();
            return tiles;
        }
    }

    if (!txn.commit()) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to commit getTileDownloadList transaction";
        tiles.clear();
    }

    return tiles;
}

bool QGCTileCacheDatabase::updateTileDownloadState(quint64 setID, int state, const QString& hash)
{
    if (!_ensureConnected()) {
        return false;
    }

    QSqlQuery query(_database());
    const bool ok =
        (state == QGCTile::StateComplete)
            ? QGCSqlHelper::execPrepared(query, "DELETE FROM TilesDownload WHERE setID = ? AND hash = ?", setID, hash)
            : QGCSqlHelper::execPrepared(query, "UPDATE TilesDownload SET state = ? WHERE setID = ? AND hash = ?",
                                         state, setID, hash);
    if (!ok) {
        qCWarning(QGCTileCacheDatabaseLog) << "Error:" << query.lastError().text();
        return false;
    }

    return true;
}

bool QGCTileCacheDatabase::updateAllTileDownloadStates(quint64 setID, int state, int fromState)
{
    if (!_ensureConnected()) {
        return false;
    }

    QSqlQuery query(_database());
    const bool ok =
        (fromState >= 0)
            ? QGCSqlHelper::execPrepared(query, "UPDATE TilesDownload SET state = ? WHERE setID = ? AND state = ?",
                                         state, setID, fromState)
            : QGCSqlHelper::execPrepared(query, "UPDATE TilesDownload SET state = ? WHERE setID = ?", state, setID);
    if (!ok) {
        qCWarning(QGCTileCacheDatabaseLog) << "Error:" << query.lastError().text();
        return false;
    }

    return true;
}

bool QGCTileCacheDatabase::pruneCache(quint64 amount)
{
    if (!_ensureConnected()) {
        return false;
    }

    quint64 remaining = amount;
    while (remaining > 0) {
        QSqlQuery query(_database());
        query.setForwardOnly(true);
        if (!query.prepare(
                QStringLiteral(
                    "SELECT tileID, size, hash FROM Tiles WHERE tileID IN (%1) ORDER BY accessed ASC LIMIT ?")
                    .arg(kUniqueTilesSubquery))) {
            qCWarning(QGCTileCacheDatabaseLog) << "Failed to prepare prune query:" << query.lastError().text();
            return false;
        }
        query.addBindValue(_getDefaultTileSet());
        query.addBindValue(kPruneBatchSize);
        if (!query.exec()) {
            return false;
        }

        QList<quint64> tileIDs;
        while (query.next() && (remaining > 0)) {
            tileIDs << query.value(0).toULongLong();
            const quint64 sz = query.value(1).toULongLong();
            remaining = (sz >= remaining) ? 0 : remaining - sz;
            qCDebug(QGCTileCacheDatabaseLog) << "HASH:" << query.value(2).toString();
        }

        if (tileIDs.isEmpty()) {
            break;
        }

        QGCSqlHelper::Transaction txn(_database());
        if (!txn.ok()) {
            return false;
        }

        if (!_deleteTilesByIDs(tileIDs)) {
            return false;
        }

        if (!txn.commit()) {
            return false;
        }
    }

    // Reclaim pages freed by the deletes (auto_vacuum=INCREMENTAL DBs only).
    QSqlDatabase db = _database();
    if (!QGCSqlHelper::incrementalVacuum(db)) {
        qCWarning(QGCTileCacheDatabaseLog) << "Incremental vacuum failed after prune";
    }

    return true;
}

void QGCTileCacheDatabase::deleteBingNoTileTiles()
{
    if (!_ensureConnected()) {
        return;
    }

    QSettings settings;
    if (settings.value(QLatin1String(kBingNoTileDoneKey), false).toBool()) {
        return;
    }

    QFile file(QStringLiteral(":/res/BingNoTileBytes.dat"));
    if (!file.open(QFile::ReadOnly)) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to Open File" << file.fileName() << ":" << file.errorString();
        return;
    }

    const QByteArray noTileBytes = file.readAll();
    file.close();

    QSqlQuery query(_database());
    query.setForwardOnly(true);
    if (!query.prepare("SELECT tileID, hash FROM Tiles WHERE LENGTH(tile) = ? AND tile = ?")) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to prepare Bing no-tile query";
        return;
    }
    query.addBindValue(noTileBytes.length());
    query.addBindValue(noTileBytes);
    if (!query.exec()) {
        qCWarning(QGCTileCacheDatabaseLog) << "query failed";
        return;
    }

    QList<quint64> idsToDelete;
    while (query.next()) {
        idsToDelete.append(query.value(0).toULongLong());
        qCDebug(QGCTileCacheDatabaseLog) << "HASH:" << query.value(1).toString();
    }

    if (idsToDelete.isEmpty()) {
        settings.setValue(QLatin1String(kBingNoTileDoneKey), true);
        return;
    }

    QGCSqlHelper::Transaction txn(_database());
    if (!txn.ok()) {
        return;
    }

    bool allSucceeded = true;
    for (qsizetype offset = 0; offset < idsToDelete.size(); offset += kPruneBatchSize) {
        const qsizetype batchEnd = qMin(offset + static_cast<qsizetype>(kPruneBatchSize), idsToDelete.size());
        const QList<quint64> batch = idsToDelete.mid(offset, batchEnd - offset);
        if (!_deleteTilesByIDs(batch)) {
            allSucceeded = false;
            break;
        }
    }

    if (allSucceeded && txn.commit()) {
        settings.setValue(QLatin1String(kBingNoTileDoneKey), true);
    }
}

TotalsResult QGCTileCacheDatabase::computeTotals()
{
    TotalsResult result;
    if (!_ensureConnected()) {
        return result;
    }

    QSqlQuery query(_database());

    if (query.exec("SELECT COUNT(size), SUM(size) FROM Tiles") && query.next()) {
        result.totalCount = query.value(0).toUInt();
        result.totalSize = query.value(1).toULongLong();
    }

    if (!query.prepare(QStringLiteral("SELECT COUNT(size), SUM(size) FROM Tiles WHERE tileID IN (%1)")
                           .arg(kUniqueTilesSubquery))) {
        return result;
    }
    query.addBindValue(_getDefaultTileSet());
    if (query.exec() && query.next()) {
        result.defaultCount = query.value(0).toUInt();
        result.defaultSize = query.value(1).toULongLong();
    }

    return result;
}

SetTotalsResult QGCTileCacheDatabase::computeSetTotals(quint64 setID, bool isDefault, quint32 totalTileCount,
                                                       const QString& type)
{
    SetTotalsResult result;

    if (isDefault) {
        TotalsResult totals = computeTotals();
        result.savedTileCount = totals.totalCount;
        result.savedTileSize = totals.totalSize;
        result.totalTileSize = totals.totalSize;
        result.uniqueTileCount = totals.defaultCount;
        result.uniqueTileSize = totals.defaultSize;
        return result;
    }

    if (!_ensureConnected()) {
        return result;
    }

    QSqlQuery subquery(_database());
    if (!subquery.prepare("SELECT COUNT(size), SUM(size) FROM Tiles A INNER JOIN SetTiles B ON A.tileID = B.tileID "
                          "WHERE B.setID = ?")) {
        return result;
    }
    subquery.addBindValue(setID);
    if (!subquery.exec() || !subquery.next()) {
        return result;
    }

    result.savedTileCount = subquery.value(0).toUInt();
    result.savedTileSize = subquery.value(1).toULongLong();

    quint64 avg = UrlFactory::averageSizeForType(type);
    if (avg == 0) {
        avg = 4096;
    }
    if (totalTileCount <= result.savedTileCount) {
        result.totalTileSize = result.savedTileSize;
    } else {
        if ((result.savedTileCount > 10) && result.savedTileSize) {
            avg = result.savedTileSize / result.savedTileCount;
        }
        result.totalTileSize = avg * totalTileCount;
    }

    quint32 dbUniqueCount = 0;
    quint64 dbUniqueSize = 0;
    if (subquery.prepare(QStringLiteral("SELECT COUNT(size), SUM(size) FROM Tiles WHERE tileID IN (%1)")
                             .arg(kUniqueTilesSubquery))) {
        subquery.addBindValue(setID);
        if (subquery.exec() && subquery.next()) {
            dbUniqueCount = subquery.value(0).toUInt();
            dbUniqueSize = subquery.value(1).toULongLong();
        }
    } else {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to prepare unique tiles query:" << subquery.lastError().text();
    }

    if (dbUniqueCount > 0) {
        result.uniqueTileCount = dbUniqueCount;
        result.uniqueTileSize = dbUniqueSize;
    } else {
        const quint32 estimatedCount =
            (totalTileCount > result.savedTileCount) ? (totalTileCount - result.savedTileCount) : 0;
        result.uniqueTileCount = estimatedCount;
        result.uniqueTileSize = estimatedCount * avg;
    }

    return result;
}

DatabaseResult QGCTileCacheDatabase::importSetsReplace(const QString& path, ProgressCallback progressCb)
{
    DatabaseResult result;
    if (QFileInfo(path).canonicalFilePath() == QFileInfo(_databasePath).canonicalFilePath()) {
        result.errorString = "Import path must differ from the active database";
        return result;
    }
    _defaultSet = kInvalidTileSet;
    disconnectDB();
    const QString backupPath = _databasePath + QStringLiteral(".bak");
    (void) QFile::remove(backupPath);
    const bool hasBackup = QFile::rename(_databasePath, backupPath);
    if (!hasBackup) {
        (void) QFile::remove(_databasePath);
    }
    if (!QFile::copy(path, _databasePath)) {
        if (hasBackup) {
            (void) QFile::rename(backupPath, _databasePath);
        }
        result.errorString = "Failed to copy import database";
        _valid = false;
        _failed = true;
        return result;
    }
    (void) QFile::remove(backupPath);
    if (progressCb)
        progressCb(25);
    // keepConnected=true leaves the SQLite connection open for the caller; the
    // import flow reuses it immediately. The owning worker calls disconnectDB().
    init(/*keepConnected=*/true);
    if (!_valid) {
        result.errorString = QStringLiteral("Failed to initialize tile cache database after import");
    } else if (progressCb) {
        progressCb(50);
    }
    if (progressCb)
        progressCb(100);
    result.success = _valid;
    return result;
}

DatabaseResult QGCTileCacheDatabase::importSetsMerge(const QString& path, ProgressCallback progressCb)
{
    DatabaseResult result;
    if (QFileInfo(path).canonicalFilePath() == QFileInfo(_databasePath).canonicalFilePath()) {
        result.errorString = "Import path must differ from the active database";
        return result;
    }
    if (!_ensureConnected()) {
        result.errorString = "Database not connected";
        return result;
    }

    // Bulk read of a potentially large tile DB — opt into a 64 MB mmap window.
    QGCSqlHelper::ScopedConnection importDB(path, /*readOnly=*/true, QStringLiteral("QGeoTileImportSession"),
                                            64LL * 1024 * 1024);
    if (!importDB.isValid()) {
        result.errorString = "Error opening import database";
        return result;
    }

    QSqlQuery query(importDB.database());
    quint64 tileCount = 0;
    int lastProgress = -1;
    if (query.exec("SELECT COUNT(tileID) FROM Tiles") && query.next()) {
        tileCount = query.value(0).toULongLong();
    }

    bool tilesImported = false;

    if (tileCount > 0) {
        if (query.exec("SELECT * FROM TileSets ORDER BY defaultSet DESC, name ASC")) {
            quint64 currentCount = 0;
            while (query.next()) {
                QString name = query.value("name").toString();
                const quint64 setID = query.value("setID").toULongLong();
                const QString mapType = query.value("typeStr").toString();
                const double topleftLat = query.value("topleftLat").toDouble();
                const double topleftLon = query.value("topleftLon").toDouble();
                const double bottomRightLat = query.value("bottomRightLat").toDouble();
                const double bottomRightLon = query.value("bottomRightLon").toDouble();
                const int minZoom = query.value("minZoom").toInt();
                const int maxZoom = query.value("maxZoom").toInt();
                const quint32 numTiles = query.value("numTiles").toUInt();
                const int defaultSet = query.value("defaultSet").toInt();
                quint64 insertSetID = _getDefaultTileSet();

                // Wrap each set creation + tile copy in a single transaction
                QGCSqlHelper::Transaction txn(_database());
                if (!txn.ok()) {
                    result.errorString = "Failed to start transaction for import set";
                    break;
                }

                if (defaultSet == 0) {
                    name = _deduplicateSetName(name);
                    QSqlQuery cQuery(_database());
                    if (!cQuery.prepare("INSERT INTO TileSets("
                                        "name, typeStr, topleftLat, topleftLon, bottomRightLat, bottomRightLon, "
                                        "minZoom, maxZoom, numTiles, defaultSet, date"
                                        ") VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)")) {
                        result.errorString = "Error preparing tile set insert";
                        break;
                    }
                    cQuery.addBindValue(name);
                    cQuery.addBindValue(mapType);
                    cQuery.addBindValue(topleftLat);
                    cQuery.addBindValue(topleftLon);
                    cQuery.addBindValue(bottomRightLat);
                    cQuery.addBindValue(bottomRightLon);
                    cQuery.addBindValue(minZoom);
                    cQuery.addBindValue(maxZoom);
                    cQuery.addBindValue(numTiles);
                    cQuery.addBindValue(defaultSet);
                    cQuery.addBindValue(QDateTime::currentSecsSinceEpoch());
                    if (!cQuery.exec()) {
                        result.errorString = "Error adding imported tile set to database";
                        break;
                    }
                    insertSetID = cQuery.lastInsertId().toULongLong();
                }

                quint64 tilesIterated = 0;
                const quint64 tilesSaved = _copyTilesForSet(importDB.database(), setID, insertSetID, currentCount,
                                                            tileCount, lastProgress, progressCb, &tilesIterated, false);
                if (tilesSaved > 0) {
                    tilesImported = true;
                    QSqlQuery cQuery(_database());
                    if (cQuery.prepare("SELECT COUNT(size) FROM Tiles A INNER JOIN SetTiles B ON A.tileID = B.tileID "
                                       "WHERE B.setID = ?")) {
                        cQuery.addBindValue(insertSetID);
                        if (cQuery.exec() && cQuery.next()) {
                            const quint64 count = cQuery.value(0).toULongLong();
                            if (cQuery.prepare("UPDATE TileSets SET numTiles = ? WHERE setID = ?")) {
                                cQuery.addBindValue(count);
                                cQuery.addBindValue(insertSetID);
                                (void) cQuery.exec();
                            }
                        }
                    }
                }

                if (!txn.commit()) {
                    qCWarning(QGCTileCacheDatabaseLog) << "Failed to commit import transaction for set:" << name;
                    continue;
                }

                if (tilesIterated > tilesSaved) {
                    const quint64 alreadyExisting = tilesIterated - tilesSaved;
                    tileCount = (alreadyExisting < tileCount) ? tileCount - alreadyExisting : 0;
                }

                if ((tilesSaved == 0) && (defaultSet == 0) && (insertSetID != _getDefaultTileSet())) {
                    qCDebug(QGCTileCacheDatabaseLog) << "No unique tiles in" << name << "Removing it.";
                    deleteTileSet(insertSetID);
                }
            }
        } else {
            result.errorString = "No tile set in database";
        }
    }

    if (!tilesImported && result.errorString.isEmpty()) {
        result.errorString = "No unique tiles in imported database";
    }
    result.success = result.errorString.isEmpty();
    return result;
}

DatabaseResult QGCTileCacheDatabase::exportSets(const QList<TileSetRecord>& sets, const QString& path,
                                                ProgressCallback progressCb)
{
    DatabaseResult result;
    if (!_ensureConnected()) {
        result.errorString = "Database not connected";
        return result;
    }
    if (QFileInfo(path).canonicalFilePath() == QFileInfo(_databasePath).canonicalFilePath()) {
        result.errorString = "Export path must differ from the active database";
        return result;
    }

    (void) QFile::remove(path);
    QGCSqlHelper::ScopedConnection exportDB(path, /*readOnly=*/false, QStringLiteral("QGeoTileExportSession"));
    if (!exportDB.isValid()) {
        qCCritical(QGCTileCacheDatabaseLog)
            << "Map Cache SQL error (create export database):" << exportDB.database().lastError();
        result.errorString = "Error opening export database";
        return result;
    }

    if (!QGCTileDatabaseSchema::createSchema(exportDB.database(), false)) {
        result.errorString = "Error creating export database";
        return result;
    }

    quint64 tileCount = 0;
    quint64 currentCount = 0;
    int lastProgress = -1;
    for (const auto& set : sets) {
        QSqlQuery countQuery(_database());
        quint64 actualCount = 0;
        if (countQuery.prepare(
                "SELECT COUNT(*) FROM Tiles T INNER JOIN SetTiles S ON T.tileID = S.tileID WHERE S.setID = ?")) {
            countQuery.addBindValue(set.setID);
            if (countQuery.exec() && countQuery.next()) {
                actualCount = countQuery.value(0).toULongLong();
            }
        }
        tileCount += (actualCount > 0) ? actualCount : set.numTiles;
    }

    if (tileCount == 0) {
        tileCount = 1;
    }

    for (const auto& set : sets) {
        QSqlQuery query(_database());
        query.setForwardOnly(true);
        if (!query.prepare("SELECT T.hash, T.format, T.tile, T.typeStr, T.date FROM Tiles T "
                           "INNER JOIN SetTiles S ON T.tileID = S.tileID WHERE S.setID = ?")) {
            qCWarning(QGCTileCacheDatabaseLog) << "Failed to prepare tile query for export set" << set.name;
            continue;
        }
        query.addBindValue(set.setID);
        if (!query.exec()) {
            qCWarning(QGCTileCacheDatabaseLog) << "Failed to query tiles for export set" << set.name;
            continue;
        }

        QGCSqlHelper::Transaction txn(exportDB.database());
        if (!txn.ok()) {
            qCWarning(QGCTileCacheDatabaseLog) << "Failed to start transaction for export set" << set.name;
            result.errorString = "Failed to start export transaction";
            break;
        }

        QSqlQuery exportQuery(exportDB.database());
        if (!exportQuery.prepare("INSERT INTO TileSets("
                                 "name, typeStr, topleftLat, topleftLon, bottomRightLat, bottomRightLon, minZoom, "
                                 "maxZoom, numTiles, defaultSet, date"
                                 ") VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)")) {
            result.errorString = "Error preparing tile set insert for export";
            break;
        }
        exportQuery.addBindValue(set.name);
        exportQuery.addBindValue(set.mapTypeStr);
        exportQuery.addBindValue(set.topleftLat);
        exportQuery.addBindValue(set.topleftLon);
        exportQuery.addBindValue(set.bottomRightLat);
        exportQuery.addBindValue(set.bottomRightLon);
        exportQuery.addBindValue(set.minZoom);
        exportQuery.addBindValue(set.maxZoom);
        exportQuery.addBindValue(set.numTiles);
        exportQuery.addBindValue(set.defaultSet);
        exportQuery.addBindValue(set.date);
        if (!exportQuery.exec()) {
            result.errorString = "Error adding tile set to exported database";
            break;
        }

        const quint64 exportSetID = exportQuery.lastInsertId().toULongLong();

        // Prepare once per set, bind+exec per tile (re-preparing inside the loop
        // recompiled the SQL for every tile).
        QSqlQuery tileInsert(exportDB.database());
        QSqlQuery tileLookup(exportDB.database());
        QSqlQuery setTileInsert(exportDB.database());
        if (!tileInsert.prepare(
                "INSERT INTO Tiles(hash, format, tile, size, typeStr, date) VALUES(?, ?, ?, ?, ?, ?)") ||
            !tileLookup.prepare("SELECT tileID FROM Tiles WHERE hash = ?") ||
            !setTileInsert.prepare("INSERT OR IGNORE INTO SetTiles(tileID, setID) VALUES(?, ?)")) {
            qCWarning(QGCTileCacheDatabaseLog)
                << "Failed to prepare tile export statements:" << tileInsert.lastError().text();
            result.errorString = "Error preparing tile export statements";
            break;
        }

        quint64 skippedTiles = 0;
        while (query.next()) {
            const QString hash = query.value(0).toString();
            const QString format = query.value(1).toString();
            const QByteArray img = query.value(2).toByteArray();
            const QString tileType = query.value(3).toString();
            const quint64 tileDate = query.value(4).toULongLong();

            quint64 exportTileID = 0;
            tileInsert.addBindValue(hash);
            tileInsert.addBindValue(format);
            tileInsert.addBindValue(img);
            tileInsert.addBindValue(img.size());
            tileInsert.addBindValue(tileType);
            tileInsert.addBindValue(tileDate);
            if (tileInsert.exec()) {
                exportTileID = tileInsert.lastInsertId().toULongLong();
            } else {
                tileLookup.addBindValue(hash);
                if (tileLookup.exec() && tileLookup.next()) {
                    exportTileID = tileLookup.value(0).toULongLong();
                }
                tileLookup.finish();
            }

            if (exportTileID > 0) {
                setTileInsert.addBindValue(exportTileID);
                setTileInsert.addBindValue(exportSetID);
                if (!setTileInsert.exec()) {
                    qCWarning(QGCTileCacheDatabaseLog)
                        << "Failed to link tile to set in export:" << setTileInsert.lastError().text();
                }
            } else {
                skippedTiles++;
            }
            currentCount++;
            if (progressCb) {
                const int progress = qMin(
                    100,
                    static_cast<int>((static_cast<double>(currentCount) / static_cast<double>(tileCount)) * 100.0));
                if (lastProgress != progress) {
                    lastProgress = progress;
                    progressCb(progress);
                }
            }
        }
        if (skippedTiles > 0) {
            qCWarning(QGCTileCacheDatabaseLog) << "Skipped" << skippedTiles << "tiles during export of" << set.name;
        }
        if (!txn.commit()) {
            qCWarning(QGCTileCacheDatabaseLog) << "Failed to commit export transaction for" << set.name;
        }
    }

    result.success = result.errorString.isEmpty();
    return result;
}

quint64 QGCTileCacheDatabase::_getDefaultTileSet()
{
    if (_defaultSet != kInvalidTileSet) {
        return _defaultSet;
    }

    if (!_ensureConnected()) {
        return kInvalidTileSet;
    }

    QSqlQuery query(_database());
    if (query.exec("SELECT setID FROM TileSets WHERE defaultSet = 1") && query.next()) {
        _defaultSet = query.value(0).toULongLong();
        return _defaultSet;
    }

    qCWarning(QGCTileCacheDatabaseLog) << "Default tile set not found in database";
    return kInvalidTileSet;
}

bool QGCTileCacheDatabase::_deleteTilesByIDs(const QList<quint64>& ids)
{
    if (ids.isEmpty()) {
        return true;
    }

    QSqlQuery query(_database());
    if (!query.prepare(
            QStringLiteral("DELETE FROM Tiles WHERE tileID IN (%1)").arg(QGCSqlHelper::placeholders(ids.size())))) {
        return false;
    }
    for (const quint64 id : ids) {
        query.addBindValue(id);
    }
    return query.exec();
}

QString QGCTileCacheDatabase::_deduplicateSetName(const QString& name)
{
    if (!findTileSetID(name).has_value()) {
        return name;
    }

    QSet<QString> existing;
    existing.insert(name);
    QSqlQuery query(_database());
    const QString escaped = QGCSqlHelper::escapeLikePattern(name);
    if (QGCSqlHelper::execPrepared(
            query, QStringLiteral("SELECT name FROM TileSets WHERE name LIKE ? || ' %' ESCAPE '\\'"), escaped)) {
        while (query.next()) {
            existing.insert(query.value(0).toString());
        }
    }

    for (int i = 1; i <= 9999; i++) {
        const QString candidate = QStringLiteral("%1 %2").arg(name).arg(i, 4, 10, QChar('0'));
        if (!existing.contains(candidate)) {
            return candidate;
        }
    }

    return QStringLiteral("%1 %2").arg(name, QUuid::createUuid().toString(QUuid::WithoutBraces).left(8));
}

quint64 QGCTileCacheDatabase::_copyTilesForSet(QSqlDatabase srcDB, quint64 srcSetID, quint64 dstSetID,
                                               quint64& currentCount, quint64 tileCount, int& lastProgress,
                                               ProgressCallback progressCb, quint64* tilesIteratedOut,
                                               bool useTransaction)
{
    QSqlQuery subQuery(srcDB);
    subQuery.setForwardOnly(true);
    if (!subQuery.prepare("SELECT T.hash, T.format, T.tile, T.typeStr, T.date FROM Tiles T "
                          "INNER JOIN SetTiles S ON T.tileID = S.tileID WHERE S.setID = ?")) {
        if (tilesIteratedOut)
            *tilesIteratedOut = 0;
        return 0;
    }
    subQuery.addBindValue(srcSetID);
    if (!subQuery.exec()) {
        if (tilesIteratedOut)
            *tilesIteratedOut = 0;
        return 0;
    }

    quint64 tilesFound = 0;
    quint64 tilesLinked = 0;

    std::unique_ptr<QGCSqlHelper::Transaction> txn;
    if (useTransaction) {
        txn = std::make_unique<QGCSqlHelper::Transaction>(_database());
        if (!txn->ok()) {
            qCWarning(QGCTileCacheDatabaseLog) << "Failed to start transaction for merge import";
            if (tilesIteratedOut)
                *tilesIteratedOut = 0;
            return 0;
        }
    }

    // Prepare once, bind+exec per tile (re-preparing inside the loop recompiled
    // the SQL for every tile).
    QSqlQuery tileInsert(_database());
    QSqlQuery tileLookup(_database());
    QSqlQuery setTileInsert(_database());
    if (!tileInsert.prepare("INSERT INTO Tiles(hash, format, tile, size, typeStr, date) VALUES(?, ?, ?, ?, ?, ?)") ||
        !tileLookup.prepare("SELECT tileID FROM Tiles WHERE hash = ?") ||
        !setTileInsert.prepare("INSERT OR IGNORE INTO SetTiles(tileID, setID) VALUES(?, ?)")) {
        qCWarning(QGCTileCacheDatabaseLog)
            << "Failed to prepare merge import statements:" << tileInsert.lastError().text();
        if (tilesIteratedOut)
            *tilesIteratedOut = 0;
        return 0;
    }
    while (subQuery.next()) {
        tilesFound++;
        const QString hash = subQuery.value(0).toString();
        const QString format = subQuery.value(1).toString();
        const QByteArray img = subQuery.value(2).toByteArray();
        const QString tileType = subQuery.value(3).toString();
        const quint64 tileDate = subQuery.value(4).toULongLong();

        quint64 importTileID = 0;
        tileInsert.addBindValue(hash);
        tileInsert.addBindValue(format);
        tileInsert.addBindValue(img);
        tileInsert.addBindValue(img.size());
        tileInsert.addBindValue(tileType);
        tileInsert.addBindValue(tileDate);
        if (tileInsert.exec()) {
            importTileID = tileInsert.lastInsertId().toULongLong();
        } else {
            tileLookup.addBindValue(hash);
            if (tileLookup.exec() && tileLookup.next()) {
                importTileID = tileLookup.value(0).toULongLong();
            }
            tileLookup.finish();
        }

        if (importTileID > 0) {
            setTileInsert.addBindValue(importTileID);
            setTileInsert.addBindValue(dstSetID);
            if (setTileInsert.exec() && setTileInsert.numRowsAffected() > 0) {
                tilesLinked++;
            }
        }

        currentCount++;
        if (tileCount > 0 && progressCb) {
            const int progress = qMin(
                100, static_cast<int>((static_cast<double>(currentCount) / static_cast<double>(tileCount)) * 100.0));
            if (lastProgress != progress) {
                lastProgress = progress;
                progressCb(progress);
            }
        }
    }

    if (txn && !txn->commit()) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to commit merge import transaction";
        if (tilesIteratedOut)
            *tilesIteratedOut = tilesFound;
        return 0;
    }

    if (tilesIteratedOut)
        *tilesIteratedOut = tilesFound;
    return tilesLinked;
}
