#include "TileCacheDatabase.h"

#include <algorithm>
#include <atomic>
#include <limits>

#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

#include "CacheTile.h"
#include "MapProviderRegistry.h"
#include "QGCLoggingCategory.h"
#include "QGCSqlHelper.h"

QGC_LOGGING_CATEGORY(TileCacheDatabaseLog, "MapTiles.TileCacheDatabase")

static std::atomic<quint64> s_connectionCounter{0};

bool TileCacheDatabase::_execBound(QSqlQuery& query, const QString& sql, const QVariantList& values)
{
    if (!query.prepare(sql)) {
        return false;
    }
    for (const QVariant& value : values) {
        // An id is a rowid, which a signed number holds
        const bool unsignedId = (value.typeId() == QMetaType::ULongLong) &&
                                (value.toULongLong() <= quint64(std::numeric_limits<qint64>::max()));
        query.addBindValue(unsignedId ? QVariant(value.toLongLong()) : value);
    }
    return query.exec();
}

bool TileCacheDatabase::_execOrWarn(QSqlQuery& query, const QString& sql, const QVariantList& values, const char* what)
{
    if (_execBound(query, sql, values)) {
        return true;
    }
    qCWarning(TileCacheDatabaseLog).nospace().noquote()
        << "Map Cache SQL error (" << what << "): " << query.lastError().text();
    return false;
}

void TileCacheDatabase::_checkpointWriteAheadLog(QSqlQuery& query)
{
    if (!query.exec(QStringLiteral("PRAGMA wal_checkpoint(PASSIVE)"))) {
        qCWarning(TileCacheDatabaseLog) << "Failed to checkpoint the write-ahead log:" << query.lastError().text();
    }
    query.finish();
}

bool TileCacheDatabase::_insertTileCacheSet(QSqlQuery& query, const TileSetRecord& set, QLatin1StringView schema)
{
    return _execBound(query,
                      QStringLiteral("INSERT INTO %1.TileSets("
                                     "name, typeStr, topleftLat, topleftLon, bottomRightLat, bottomRightLon, minZoom, "
                                     "maxZoom, type, numTiles, defaultSet, date"
                                     ") VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)")
                          .arg(schema),
                      {set.name, set.mapName, set.topleftLat, set.topleftLon, set.bottomRightLat, set.bottomRightLon,
                       set.minZoom, set.maxZoom, set.mapId, set.numTiles, set.defaultSet, set.date});
}

TileCacheDatabase::TileCacheDatabase(const QString& databasePath)
    : _databasePath(databasePath)
    , _connectionName(QStringLiteral("TileCache_%1").arg(s_connectionCounter.fetch_add(1)))
    , _legacyMapIds(MapProviderRegistry::legacyMapIds())
{}

TileCacheDatabase::~TileCacheDatabase()
{
    disconnectDB();
}

QSqlDatabase TileCacheDatabase::_database() const
{
    return QSqlDatabase::database(_connectionName);
}

QSqlDatabase TileCacheDatabase::database() const
{
    return _database();
}

bool TileCacheDatabase::_ensureConnected() const
{
    if (!_connected || !_valid) {
        qCWarning(TileCacheDatabaseLog) << "Database not connected";
        return false;
    }
    return true;
}

bool TileCacheDatabase::init()
{
    _failed = false;
    if (!_databasePath.isEmpty()) {
        qCDebug(TileCacheDatabaseLog) << "Mapping cache directory:" << _databasePath;
        if (connectDB()) {
            if (!_checkSchemaVersion()) {
                _valid = false;
                _failed = true;
                disconnectDB();
                return false;
            }
            _valid = _createDB(_database());
            disconnectDB();
            if (!_valid) {
                _failed = true;
                _removeDatabaseFiles(_databasePath);
            }
        } else {
            _failed = true;
        }
    } else {
        qCCritical(TileCacheDatabaseLog) << "Could not find suitable cache directory.";
        _failed = true;
    }

    return !_failed;
}

void TileCacheDatabase::_removeDatabaseFiles(const QString& path)
{
    for (const char* suffix : {"", "-wal", "-shm"}) {
        (void) QFile::remove(path + QLatin1String(suffix));
    }
}

bool TileCacheDatabase::connectDB()
{
    if (_connected) {
        disconnectDB();
    }

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", _connectionName);
        db.setDatabaseName(_databasePath);
        _valid = db.open();
        if (_valid) {
            // The mode in which a file gives back the space of deleted tiles: see releaseFreePages(). A new file gets
            // it from this, which has to precede its first table and its WAL mode. An existing file without the mode
            // stays as it is until resetDatabase() rebuilds it.
            (void) QSqlQuery(db).exec(QStringLiteral("PRAGMA auto_vacuum = INCREMENTAL"));
            QGCSqlHelper::applySqlitePragmas(db);
            _connected = true;
        } else {
            qCCritical(TileCacheDatabaseLog) << "Map Cache SQL error (open db):" << db.lastError();
        }
    }
    if (!_valid) {
        // Once no handle of the connection is left: Qt warns of a connection that is removed while it is in use
        QSqlDatabase::removeDatabase(_connectionName);
    }
    return _valid;
}

void TileCacheDatabase::disconnectDB()
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
            db.close();
        }
    }
    QSqlDatabase::removeDatabase(_connectionName);
}

bool TileCacheDatabase::saveTile(const QString& hash, const QString& format, const QByteArray& img, int mapId,
                                 quint64 tileSet)
{
    if (!_ensureConnected()) {
        return false;
    }

    QGCSqlHelper::Transaction txn(_database());
    if (!txn.ok()) {
        qCWarning(TileCacheDatabaseLog) << "Failed to start transaction for saveTile";
        return false;
    }

    QSqlQuery query(_database());
    if (!_execOrWarn(query, QLatin1StringView(kSaveTileSql),
                     {hash, format, img, img.size(), mapId, QDateTime::currentSecsSinceEpoch()}, "save tile")) {
        return false;
    }

    if (!_execOrWarn(query, QStringLiteral("SELECT tileID FROM Tiles WHERE hash = ?"), {hash}, "find saved tile")) {
        return false;
    }
    if (!query.next()) {
        qCWarning(TileCacheDatabaseLog) << "Saved tile not found:" << hash;
        return false;
    }
    const quint64 tileID = query.value(0).toULongLong();

    const quint64 setID = (tileSet == kInvalidTileSet) ? _getDefaultTileSet() : tileSet;
    if (setID == kInvalidTileSet) {
        qCWarning(TileCacheDatabaseLog) << "Cannot save tile: no valid tile set";
        return false;
    }
    if (!_execOrWarn(query, QStringLiteral("INSERT OR IGNORE INTO SetTiles(tileID, setID) VALUES(?, ?)"),
                     {tileID, setID}, "add tile to set")) {
        return false;
    }

    if (!txn.commit()) {
        qCWarning(TileCacheDatabaseLog) << "Failed to commit saveTile transaction";
        return false;
    }

    qCDebug(TileCacheDatabaseLog) << "HASH:" << hash;
    return true;
}

std::unique_ptr<CacheTile> TileCacheDatabase::getTile(const QString& hash) const
{
    if (!_ensureConnected()) {
        return nullptr;
    }

    QSqlQuery query(_database());
    if (_execBound(query, QStringLiteral("SELECT tile, format, type FROM Tiles WHERE hash = ?"), {hash}) &&
        query.next()) {
        qCDebug(TileCacheDatabaseLog) << "(Found in DB) HASH:" << hash;
        return std::make_unique<CacheTile>(hash, query.value(0).toByteArray(), query.value(1).toString(),
                                           query.value(2).toInt());
    }

    qCDebug(TileCacheDatabaseLog) << "(NOT in DB) HASH:" << hash;
    return nullptr;
}

void TileCacheDatabase::markTileUsed(const QString& hash)
{
    if (!_ensureConnected()) {
        return;
    }

    // The date of a tile is that of its saving until the tile is used. Nothing but pruneCache() reads it, and only
    // of the tiles that no set but the default one has: the others are left alone, as SQLite writes the picture of
    // a tile again with its date.
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    const qint64 granularity = std::chrono::duration_cast<std::chrono::seconds>(kUseDateGranularity).count();
    QSqlQuery query(_database());
    if (!_execBound(query,
                    QStringLiteral("UPDATE Tiles SET date = ? WHERE hash = ? AND date < ? AND NOT EXISTS "
                                   "(SELECT 1 FROM SetTiles S WHERE S.tileID = Tiles.tileID AND S.setID != ?)"),
                    {now, hash, now - granularity, _getDefaultTileSet()})) {
        // A cache that cannot be written still serves its tiles
        qCDebug(TileCacheDatabaseLog) << "Tile not marked as used:" << hash << query.lastError().text();
    }
}

std::optional<quint64> TileCacheDatabase::findTile(const QString& hash) const
{
    if (!_ensureConnected()) {
        return std::nullopt;
    }

    QSqlQuery query(_database());
    if (_execBound(query, QStringLiteral("SELECT tileID FROM Tiles WHERE hash = ?"), {hash}) && query.next()) {
        return query.value(0).toULongLong();
    }

    return std::nullopt;
}

bool TileCacheDatabase::pruneCache(quint64 amount)
{
    if (!_ensureConnected()) {
        return false;
    }

    quint64 remaining = amount;
    while (remaining > 0) {
        QSqlQuery query(_database());
        query.setForwardOnly(true);
        if (!_execOrWarn(
                query,
                QStringLiteral("SELECT tileID, size, hash FROM Tiles WHERE tileID IN (%1) ORDER BY date ASC LIMIT ?")
                    .arg(kUniqueTilesSubquery),
                {_getDefaultTileSet(), kPruneBatchSize}, "select tiles to prune")) {
            return false;
        }

        QList<quint64> tileIDs;
        while (query.next() && (remaining > 0)) {
            tileIDs << query.value(0).toULongLong();
            const quint64 sz = query.value(1).toULongLong();
            remaining = (sz >= remaining) ? 0 : remaining - sz;
            qCDebug(TileCacheDatabaseLog) << "HASH:" << query.value(2).toString();
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

    return true;
}

qint64 TileCacheDatabase::releaseFreePages(int maxPages)
{
    if (!_ensureConnected()) {
        return 0;
    }

    QSqlQuery query(_database());
    const auto freePageCount = [&query]() -> qint64 {
        return (query.exec(QStringLiteral("PRAGMA freelist_count")) && query.next()) ? query.value(0).toLongLong() : 0;
    };

    // Mode 2 is incremental auto-vacuum
    if (!query.exec(QStringLiteral("PRAGMA auto_vacuum")) || !query.next() || (query.value(0).toInt() != 2)) {
        return 0;
    }
    const qint64 freePages = freePageCount();
    if (freePages <= 0) {
        return 0;
    }

    // One transaction for the batch: it took a third less time than a transaction for every page
    QGCSqlHelper::Transaction txn(_database());
    // SQLite releases one page for each step of the pragma, and Qt steps a statement without result columns once
    bool released = txn.ok() && query.prepare(QStringLiteral("PRAGMA incremental_vacuum(1)"));
    for (qint64 pages = std::min<qint64>(maxPages, freePages); released && (pages > 0); pages--) {
        released = query.exec();
    }
    // The statement is still in its first step, and a transaction cannot commit with a statement in progress
    query.finish();
    if (!released || !txn.commit()) {
        qCWarning(TileCacheDatabaseLog) << "Failed to release unused pages:" << query.lastError().text();
        return 0;
    }

    const qint64 pagesLeft = freePageCount();
    if (pagesLeft == 0) {
        _checkpointWriteAheadLog(query);
    }
    return pagesLeft;
}

TileCacheTotals TileCacheDatabase::computeTotals()
{
    TileCacheTotals result;
    if (!_ensureConnected()) {
        return result;
    }

    QSqlQuery query(_database());

    // Read from the index of the sizes: the row of a tile has its picture before its size
    if (query.exec("SELECT COUNT(size), SUM(size) FROM Tiles") && query.next()) {
        result.totalCount = query.value(0).toUInt();
        result.totalSize = query.value(1).toULongLong();
    }

    // SQLite looks a tile up by its id in the table unless it is told the index. A cache whose index could not be
    // created is read from the table.
    static const QString defaultSetSql =
        QStringLiteral("SELECT COUNT(size), SUM(size) FROM Tiles %1 WHERE tileID IN (%2)");
    const QVariantList defaultSet = {_getDefaultTileSet()};
    if ((_execBound(query, defaultSetSql.arg(QStringLiteral("INDEXED BY idx_tiles_size"), kUniqueTilesSubquery),
                    defaultSet) ||
         _execBound(query, defaultSetSql.arg(QString(), kUniqueTilesSubquery), defaultSet)) &&
        query.next()) {
        result.defaultCount = query.value(0).toUInt();
        result.defaultSize = query.value(1).toULongLong();
    }

    return result;
}

quint64 TileCacheDatabase::_getDefaultTileSet()
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

    qCWarning(TileCacheDatabaseLog) << "Default tile set not found in database";
    return kInvalidTileSet;
}

bool TileCacheDatabase::_deleteTilesByIDs(const QList<quint64>& ids)
{
    if (ids.isEmpty()) {
        return true;
    }

    QSqlQuery query(_database());
    return _execBound(
        query, QStringLiteral("DELETE FROM Tiles WHERE tileID IN (%1)").arg(QGCSqlHelper::placeholders(ids.size())),
        QVariantList(ids.cbegin(), ids.cend()));
}
