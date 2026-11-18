#include "QGCTileCacheDatabase.h"

#include <atomic>

#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

#include "QGCCacheTile.h"
#include "QGCLoggingCategory.h"
#include "QGCSqlHelper.h"
#include "UrlFactory.h"

QGC_LOGGING_CATEGORY(QGCTileCacheDatabaseLog, "MapTiles.QGCTileCacheDatabase")

static std::atomic<quint64> s_connectionCounter{0};

bool QGCTileCacheDatabase::_execBound(QSqlQuery& query, const QString& sql, const QVariantList& values)
{
    if (!query.prepare(sql)) {
        return false;
    }
    for (const QVariant& value : values) {
        query.addBindValue(value);
    }
    return query.exec();
}

bool QGCTileCacheDatabase::_execOrWarn(QSqlQuery& query, const QString& sql, const QVariantList& values,
                                       const char* what)
{
    if (_execBound(query, sql, values)) {
        return true;
    }
    qCWarning(QGCTileCacheDatabaseLog).nospace().noquote()
        << "Map Cache SQL error (" << what << "): " << query.lastError().text();
    return false;
}

void QGCTileCacheDatabase::_checkpointWriteAheadLog(QSqlQuery& query)
{
    if (!query.exec(QStringLiteral("PRAGMA wal_checkpoint(PASSIVE)"))) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to checkpoint the write-ahead log:" << query.lastError().text();
    }
    query.finish();
}

bool QGCTileCacheDatabase::_insertTileCacheSet(QSqlQuery& query, const TileSetRecord& set)
{
    return _execBound(query,
                      QStringLiteral("INSERT INTO TileSets("
                                     "name, typeStr, topleftLat, topleftLon, bottomRightLat, bottomRightLon, minZoom, "
                                     "maxZoom, type, numTiles, defaultSet, date"
                                     ") VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"),
                      {set.name, set.mapTypeStr, set.topleftLat, set.topleftLon, set.bottomRightLat, set.bottomRightLon,
                       set.minZoom, set.maxZoom, set.type, set.numTiles, set.defaultSet, set.date});
}

QGCTileCacheDatabase::QGCTileCacheDatabase(const QString& databasePath)
    : _databasePath(databasePath)
    , _connectionName(QStringLiteral("QGCTileCache_%1").arg(s_connectionCounter.fetch_add(1)))
    , _legacyMapIds(UrlFactory::legacyMapIds())
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

bool QGCTileCacheDatabase::init()
{
    _failed = false;
    if (!_databasePath.isEmpty()) {
        qCDebug(QGCTileCacheDatabaseLog) << "Mapping cache directory:" << _databasePath;
        if (connectDB()) {
            if (!_checkSchemaVersion()) {
                _valid = false;
                _failed = true;
                disconnectDB();
                return false;
            }
            _valid = _createDB(_database());
            if (!_valid) {
                _failed = true;
                (void) QFile::remove(_databasePath);
            }
        } else {
            _failed = true;
        }
        disconnectDB();
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
    _valid = db.open();
    if (_valid) {
        // The mode in which a file gives back the space of deleted tiles: see releaseFreePages(). A new file gets
        // it from this, which has to precede its first table and its WAL mode. An existing file without the mode
        // stays as it is until resetDatabase() rebuilds it.
        (void) QSqlQuery(db).exec(QStringLiteral("PRAGMA auto_vacuum = INCREMENTAL"));
        QGCSqlHelper::applySqlitePragmas(db);
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
            db.close();
        }
    }
    QSqlDatabase::removeDatabase(_connectionName);
}

bool QGCTileCacheDatabase::saveTile(const QString& hash, const QString& format, const QByteArray& img,
                                    const QString& type, quint64 tileSet)
{
    if (!_ensureConnected()) {
        return false;
    }

    QGCSqlHelper::Transaction txn(_database());
    if (!txn.ok()) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to start transaction for saveTile";
        return false;
    }

    // A tile is saved under the key of a cached one when that one could not be used: the new one takes its place
    QSqlQuery query(_database());
    if (!_execOrWarn(query,
                     QStringLiteral("INSERT INTO Tiles(hash, format, tile, size, type, date) VALUES(?, ?, ?, ?, ?, ?) "
                                    "ON CONFLICT(hash) DO UPDATE SET format = excluded.format, tile = excluded.tile, "
                                    "size = excluded.size, date = excluded.date"),
                     {hash, format, img, img.size(), UrlFactory::getMapIdFromProviderType(type),
                      QDateTime::currentSecsSinceEpoch()},
                     "save tile")) {
        return false;
    }

    if (!_execOrWarn(query, QStringLiteral("SELECT tileID FROM Tiles WHERE hash = ?"), {hash}, "find saved tile")) {
        return false;
    }
    if (!query.next()) {
        qCWarning(QGCTileCacheDatabaseLog) << "Saved tile not found:" << hash;
        return false;
    }
    const quint64 tileID = query.value(0).toULongLong();

    const quint64 setID = (tileSet == kInvalidTileSet) ? _getDefaultTileSet() : tileSet;
    if (setID == kInvalidTileSet) {
        qCWarning(QGCTileCacheDatabaseLog) << "Cannot save tile: no valid tile set";
        return false;
    }
    if (!_execOrWarn(query, QStringLiteral("INSERT OR IGNORE INTO SetTiles(tileID, setID) VALUES(?, ?)"),
                     {tileID, setID}, "add tile to set")) {
        return false;
    }

    if (!txn.commit()) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to commit saveTile transaction";
        return false;
    }

    qCDebug(QGCTileCacheDatabaseLog) << "HASH:" << hash;
    return true;
}

std::unique_ptr<QGCCacheTile> QGCTileCacheDatabase::getTile(const QString& hash)
{
    if (!_ensureConnected()) {
        return nullptr;
    }

    QSqlQuery query(_database());
    if (_execBound(query, QStringLiteral("SELECT tile, format, type FROM Tiles WHERE hash = ?"), {hash}) &&
        query.next()) {
        const QByteArray tileData = query.value(0).toByteArray();
        const QString format = query.value(1).toString();
        const QString type = UrlFactory::getProviderTypeFromMapId(query.value(2).toInt());
        qCDebug(QGCTileCacheDatabaseLog) << "(Found in DB) HASH:" << hash;
        return std::make_unique<QGCCacheTile>(hash, tileData, format, type);
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
    if (_execBound(query, QStringLiteral("SELECT tileID FROM Tiles WHERE hash = ?"), {hash}) && query.next()) {
        return query.value(0).toULongLong();
    }

    return std::nullopt;
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

    return true;
}

qint64 QGCTileCacheDatabase::releaseFreePages(int maxPages)
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
    for (qint64 pages = qMin<qint64>(maxPages, freePages); released && (pages > 0); pages--) {
        released = query.exec();
    }
    // The statement is still in its first step, and a transaction cannot commit with a statement in progress
    query.finish();
    if (!released || !txn.commit()) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to release unused pages:" << query.lastError().text();
        return 0;
    }

    const qint64 pagesLeft = freePageCount();
    if (pagesLeft == 0) {
        _checkpointWriteAheadLog(query);
    }
    return pagesLeft;
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

    if (_execBound(
            query,
            QStringLiteral("SELECT COUNT(size), SUM(size) FROM Tiles WHERE tileID IN (%1)").arg(kUniqueTilesSubquery),
            {_getDefaultTileSet()}) &&
        query.next()) {
        result.defaultCount = query.value(0).toUInt();
        result.defaultSize = query.value(1).toULongLong();
    }

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
    return _execBound(
        query, QStringLiteral("DELETE FROM Tiles WHERE tileID IN (%1)").arg(QGCSqlHelper::placeholders(ids.size())),
        QVariantList(ids.cbegin(), ids.cend()));
}
