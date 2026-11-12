#include "QGCTileCacheDatabase.h"

#include <atomic>

#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QScopeGuard>
#include <QtCore/QSettings>
#include <QtCore/QUuid>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

#include "QGCCacheTile.h"
#include "QGCLoggingCategory.h"
#include "QGCMapUrlEngine.h"
#include "QGCSqlHelper.h"
#include "QGCTile.h"
#include "QGCTileSet.h"

QGC_LOGGING_CATEGORY(QGCTileCacheDatabaseLog, "MapTiles.QGCTileCacheDatabase")

static std::atomic<quint64> s_connectionCounter{0};

static bool execBound(QSqlQuery& query, const QString& sql, const QVariantList& values)
{
    if (!query.prepare(sql)) {
        return false;
    }
    for (const QVariant& value : values) {
        query.addBindValue(value);
    }
    return query.exec();
}

/// The file itself gets smaller only when its write-ahead log is checkpointed, which SQLite otherwise leaves until
/// about 1000 pages have changed or the cache is closed. Passive: a connection that reads the cache is not waited
/// for, and a later checkpoint does what this one could not.
static void checkpointWriteAheadLog(QSqlQuery& query)
{
    if (!query.exec(QStringLiteral("PRAGMA wal_checkpoint(PASSIVE)"))) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to checkpoint the write-ahead log:" << query.lastError().text();
    }
    query.finish();
}

/// Drops the tables of the cache. When one is left, the caller must not go on to _createDB(): that would mark the
/// rows of that table as rows of the current schema version.
static bool dropTileCacheTables(QSqlQuery& query)
{
    if (query.exec("DROP TABLE IF EXISTS TilesDownload") && query.exec("DROP TABLE IF EXISTS SetTiles") &&
        query.exec("DROP TABLE IF EXISTS Tiles") && query.exec("DROP TABLE IF EXISTS TileSets")) {
        return true;
    }
    qCWarning(QGCTileCacheDatabaseLog) << "Failed to drop tables:" << query.lastError().text();
    return false;
}

/// Removes a database file and the files SQLite keeps next to it in WAL mode
static void removeDatabaseFiles(const QString& path)
{
    for (const char* suffix : {"", "-wal", "-shm"}) {
        (void) QFile::remove(path + QLatin1String(suffix));
    }
}

/// Adds \a set to the tile sets of the database of \a query, whose lastInsertId() is then the id of the set. On
/// failure \a prepared tells which step failed: preparing the statement (false) or running it (true).
static bool insertTileCacheSet(QSqlQuery& query, const TileSetRecord& set, bool& prepared)
{
    prepared = query.prepare(
        "INSERT INTO TileSets("
        "name, typeStr, topleftLat, topleftLon, bottomRightLat, bottomRightLon, minZoom, "
        "maxZoom, type, numTiles, defaultSet, date"
        ") VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    if (!prepared) {
        return false;
    }
    query.addBindValue(set.name);
    query.addBindValue(set.mapTypeStr);
    query.addBindValue(set.topleftLat);
    query.addBindValue(set.topleftLon);
    query.addBindValue(set.bottomRightLat);
    query.addBindValue(set.bottomRightLon);
    query.addBindValue(set.minZoom);
    query.addBindValue(set.maxZoom);
    query.addBindValue(set.type);
    query.addBindValue(set.numTiles);
    query.addBindValue(set.defaultSet);
    query.addBindValue(set.date);
    return query.exec();
}

/// What copyTileToTileCacheSet() did. Where a statement failed, QSqlQuery::lastError() of its query says why.
enum class TileCacheCopy
{
    Linked,             ///< The set has the tile, and had not before
    NotLinked,          ///< The set had the tile, or the statement that links it could not be prepared
    LinkFailed,         ///< The statement that links the tile failed
    NoTile,             ///< The tile was neither added nor found
    InsertNotPrepared,  ///< The statement that adds the tile could not be prepared
};

/// Copies a tile to the database of \a query and links it to the set \a setID there. A tile that database has
/// under \a hash stays as it is, and is the one linked.
static TileCacheCopy copyTileToTileCacheSet(QSqlQuery& query, quint64 setID, const QString& hash, const QString& format,
                                            const QByteArray& img, int type, quint64 date)
{
    if (!query.prepare("INSERT INTO Tiles(hash, format, tile, size, type, date) VALUES(?, ?, ?, ?, ?, ?)")) {
        return TileCacheCopy::InsertNotPrepared;
    }
    query.addBindValue(hash);
    query.addBindValue(format);
    query.addBindValue(img);
    query.addBindValue(img.size());
    query.addBindValue(type);
    query.addBindValue(date);
    quint64 tileID = 0;
    if (query.exec()) {
        tileID = query.lastInsertId().toULongLong();
    } else if (query.prepare("SELECT tileID FROM Tiles WHERE hash = ?")) {
        query.addBindValue(hash);
        if (query.exec() && query.next()) {
            tileID = query.value(0).toULongLong();
        }
    }
    if (tileID == 0) {
        return TileCacheCopy::NoTile;
    }

    if (!query.prepare("INSERT OR IGNORE INTO SetTiles(tileID, setID) VALUES(?, ?)")) {
        return TileCacheCopy::NotLinked;
    }
    query.addBindValue(tileID);
    query.addBindValue(setID);
    if (!query.exec()) {
        return TileCacheCopy::LinkFailed;
    }
    return (query.numRowsAffected() > 0) ? TileCacheCopy::Linked : TileCacheCopy::NotLinked;
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

bool QGCTileCacheDatabase::_checkSchemaVersion()
{
    QSqlDatabase db = _database();
    const auto current = QGCSqlHelper::userVersion(db);
    if (!current) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to read schema version";
        return false;
    }

    const int version = *current;
    if (version == kSchemaVersion) {
        return true;
    }

    QSqlQuery query(db);

    if (version == 0) {
        // Either a fresh database or a legacy database created before versioning.
        // Check for existing data — if Tiles table exists with rows, it's legacy.
        // Legacy DBs stored map type as text; migration is not supported so the cache is rebuilt.
        if (query.exec("SELECT COUNT(*) FROM Tiles") && query.next() && query.value(0).toInt() > 0) {
            qCWarning(QGCTileCacheDatabaseLog)
                << "Legacy database detected (no schema version). Discarding cached tiles and rebuilding.";
            _defaultSet = kInvalidTileSet;
            return dropTileCacheTables(query);
        }
        return true;
    }

    if ((version > 0) && (version < kSchemaVersion)) {
        if (_migrate(db, version)) {
            return true;
        }
        if (!_resetOnMigrationFailure) {
            qCWarning(QGCTileCacheDatabaseLog)
                << "Failed to migrate schema version" << version << "to" << kSchemaVersion;
            return false;
        }
        qCWarning(QGCTileCacheDatabaseLog)
            << "Failed to migrate schema version" << version << "to" << kSchemaVersion << ". Resetting cache.";
    } else {
        qCWarning(QGCTileCacheDatabaseLog)
            << "Unknown schema version" << version << "(expected" << kSchemaVersion << "). Resetting cache.";
    }
    _defaultSet = kInvalidTileSet;
    return dropTileCacheTables(query);
}

bool QGCTileCacheDatabase::_migrate(QSqlDatabase db, int version)
{
    QGCSqlHelper::Transaction txn(db);
    if (!txn.ok()) {
        return false;
    }

    if ((version < 2) && !_renumberMapIds(db, _legacyMapIds)) {
        return false;
    }
    if ((version < 3) && !_adoptCacheVariants(db, _cacheVariants)) {
        return false;
    }

    // Within the transaction, so that a migration that fails leaves the version it started from
    if (!QGCSqlHelper::setUserVersion(db, kSchemaVersion)) {
        return false;
    }

    return txn.commit();
}

bool QGCTileCacheDatabase::_renumberMapIds(QSqlDatabase db, const QMap<int, int>& legacyMapIds)
{
    QSqlQuery query(db);

    // From the highest id down: an id only grows, so a renumbered row is never renumbered again as the row of a lower
    // id, and its key is never one a row that is still to be renumbered has, which UNIQUE(hash) would reject
    const QList<int> legacyIds = legacyMapIds.keys();
    for (auto legacyId = legacyIds.crbegin(); legacyId != legacyIds.crend(); ++legacyId) {
        const int mapId = legacyMapIds.value(*legacyId);
        if (mapId == *legacyId) {
            continue;
        }
        if (mapId < *legacyId) {
            qCWarning(QGCTileCacheDatabaseLog) << "Cannot renumber provider" << *legacyId << "to the lower id" << mapId;
            return false;
        }

        // The hash starts with the id as 10 digits. Rows are selected by hash because that uses its index: type has
        // none, and reading it reads the tile image stored before it.
        const QVariantList hashValues = {QString::asprintf("%010d", mapId), mapId,
                                         QString::asprintf("%010d", *legacyId),
                                         QString::asprintf("%010d", *legacyId + 1)};
        static const QString hashSql =
            QStringLiteral("UPDATE %1 SET hash = ? || substr(hash, 11), type = ? WHERE hash >= ? AND hash < ?");
        if (!execBound(query, hashSql.arg(QStringLiteral("Tiles")), hashValues) ||
            !execBound(query, hashSql.arg(QStringLiteral("TilesDownload")), hashValues) ||
            !execBound(query, QStringLiteral("UPDATE TileSets SET type = ? WHERE type = ?"), {mapId, *legacyId})) {
            qCWarning(QGCTileCacheDatabaseLog)
                << "Failed to renumber provider" << *legacyId << "to" << mapId << ":" << query.lastError().text();
            return false;
        }
    }

    return true;
}

bool QGCTileCacheDatabase::_adoptCacheVariants(QSqlDatabase db, const QHash<int, QString>& cacheVariants)
{
    QSqlQuery query(db);

    // Until version 3 a provider's tiles had one key whatever source the user had configured for it, so which source
    // a tile is from is not known. They become tiles of the source configured now.
    for (auto it = cacheVariants.cbegin(); it != cacheVariants.cend(); ++it) {
        const QString suffix = UrlFactory::cacheVariantSuffix(it.value());
        if (suffix.isEmpty()) {
            continue;
        }
        const QVariantList values = {suffix, QString::asprintf("%010d", it.key()),
                                     QString::asprintf("%010d", it.key() + 1)};
        static const QString sql = QStringLiteral("UPDATE %1 SET hash = hash || ? WHERE hash >= ? AND hash < ?");
        if (!execBound(query, sql.arg(QStringLiteral("Tiles")), values) ||
            !execBound(query, sql.arg(QStringLiteral("TilesDownload")), values)) {
            qCWarning(QGCTileCacheDatabaseLog)
                << "Failed to add the cache variant of provider" << it.key() << ":" << query.lastError().text();
            return false;
        }
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
    if (!query.prepare("INSERT INTO Tiles(hash, format, tile, size, type, date) VALUES(?, ?, ?, ?, ?, ?) "
                       "ON CONFLICT(hash) DO UPDATE SET format = excluded.format, tile = excluded.tile, "
                       "size = excluded.size, date = excluded.date")) {
        qCWarning(QGCTileCacheDatabaseLog) << "Map Cache SQL error (prepare saveTile):" << query.lastError().text();
        return false;
    }
    query.addBindValue(hash);
    query.addBindValue(format);
    query.addBindValue(img);
    query.addBindValue(img.size());
    query.addBindValue(UrlFactory::getMapIdFromProviderType(type));
    query.addBindValue(QDateTime::currentSecsSinceEpoch());
    if (!query.exec()) {
        qCWarning(QGCTileCacheDatabaseLog) << "Map Cache SQL error (saveTile INSERT):" << query.lastError().text();
        return false;
    }

    if (!query.prepare("SELECT tileID FROM Tiles WHERE hash = ?")) {
        qCWarning(QGCTileCacheDatabaseLog) << "Map Cache SQL error (prepare tile lookup):" << query.lastError().text();
        return false;
    }
    query.addBindValue(hash);
    if (!query.exec() || !query.next()) {
        qCWarning(QGCTileCacheDatabaseLog) << "Map Cache SQL error (tile lookup):" << query.lastError().text();
        return false;
    }
    const quint64 tileID = query.value(0).toULongLong();

    const quint64 setID = (tileSet == kInvalidTileSet) ? _getDefaultTileSet() : tileSet;
    if (setID == kInvalidTileSet) {
        qCWarning(QGCTileCacheDatabaseLog) << "Cannot save tile: no valid tile set";
        return false;
    }
    if (!query.prepare("INSERT OR IGNORE INTO SetTiles(tileID, setID) VALUES(?, ?)")) {
        qCWarning(QGCTileCacheDatabaseLog) << "Map Cache SQL error (prepare SetTiles):" << query.lastError().text();
        return false;
    }
    query.addBindValue(tileID);
    query.addBindValue(setID);
    if (!query.exec()) {
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

std::unique_ptr<QGCCacheTile> QGCTileCacheDatabase::getTile(const QString& hash)
{
    if (!_ensureConnected()) {
        return nullptr;
    }

    QSqlQuery query(_database());
    if (!query.prepare("SELECT tile, format, type FROM Tiles WHERE hash = ?")) {
        return nullptr;
    }
    query.addBindValue(hash);
    if (query.exec() && query.next()) {
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
    if (!query.prepare("SELECT tileID FROM Tiles WHERE hash = ?")) {
        return std::nullopt;
    }
    query.addBindValue(hash);
    if (query.exec() && query.next()) {
        return query.value(0).toULongLong();
    }

    return std::nullopt;
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
                    "minZoom, maxZoom, type, numTiles, defaultSet, date "
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
        rec.type = query.value(9).toInt();
        rec.numTiles = query.value(10).toUInt();
        rec.defaultSet = (query.value(11).toInt() != 0);
        rec.date = query.value(12).toULongLong();
        records.append(rec);
    }

    return records;
}

std::optional<quint64> QGCTileCacheDatabase::createTileSet(const QString& name, const QString& mapTypeStr,
                                                           double topleftLat, double topleftLon, double bottomRightLat,
                                                           double bottomRightLon, int minZoom, int maxZoom,
                                                           const QString& type, quint32 numTiles,
                                                           const QString& cacheVariant)
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
    const int mapTypeId = UrlFactory::getMapIdFromProviderType(type);
    const TileSetRecord record{.name = name,
                               .mapTypeStr = mapTypeStr,
                               .topleftLat = topleftLat,
                               .topleftLon = topleftLon,
                               .bottomRightLat = bottomRightLat,
                               .bottomRightLon = bottomRightLon,
                               .minZoom = minZoom,
                               .maxZoom = maxZoom,
                               .type = mapTypeId,
                               .numTiles = numTiles,
                               .date = static_cast<quint64>(QDateTime::currentSecsSinceEpoch())};
    bool prepared = false;
    if (!insertTileCacheSet(query, record, prepared)) {
        qCWarning(QGCTileCacheDatabaseLog) << (prepared ? "Map Cache SQL error (add tileSet into TileSets):"
                                                        : "Map Cache SQL error (prepare createTileSet):")
                                           << query.lastError().text();
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

    auto processBatch = [&](const QList<TileCoord>& tiles, int z) -> bool {
        QHash<QString, quint64> existingTiles;
        QSqlQuery lookup(_database());
        lookup.setForwardOnly(true);
        if (lookup.prepare(QStringLiteral("SELECT hash, tileID FROM Tiles WHERE hash IN (%1)")
                               .arg(QGCSqlHelper::placeholders(tiles.size())))) {
            for (const auto& tc : tiles) {
                lookup.addBindValue(tc.hash);
            }
            if (lookup.exec()) {
                while (lookup.next()) {
                    existingTiles.insert(lookup.value(0).toString(), lookup.value(1).toULongLong());
                }
            }
        }

        for (const auto& tc : tiles) {
            auto it = existingTiles.find(tc.hash);
            if (it != existingTiles.end()) {
                if (!query.prepare("INSERT OR IGNORE INTO SetTiles(tileID, setID) VALUES(?, ?)")) {
                    return false;
                }
                query.addBindValue(it.value());
                query.addBindValue(setID);
                if (!query.exec()) {
                    qCWarning(QGCTileCacheDatabaseLog)
                        << "Map Cache SQL error (add tile into SetTiles):" << query.lastError().text();
                    return false;
                }
            } else {
                if (!query.prepare("INSERT OR IGNORE INTO TilesDownload(setID, hash, type, x, y, z, state) VALUES(?, "
                                   "?, ?, ?, ?, ?, ?)")) {
                    return false;
                }
                query.addBindValue(setID);
                query.addBindValue(tc.hash);
                query.addBindValue(mapTypeId);
                query.addBindValue(tc.x);
                query.addBindValue(tc.y);
                query.addBindValue(z);
                query.addBindValue(static_cast<int>(QGCTile::StatePending));
                if (!query.exec()) {
                    qCWarning(QGCTileCacheDatabaseLog)
                        << "Map Cache SQL error (add tile into TilesDownload):" << query.lastError().text();
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
                batch.append({x, y, UrlFactory::getTileHash(type, x, y, z, cacheVariant)});

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
    if (!query.prepare("DELETE FROM TilesDownload WHERE setID = ?")) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to prepare download delete:" << query.lastError().text();
        return false;
    }
    query.addBindValue(id);
    if (!query.exec()) {
        return false;
    }

    // Find tiles unique to this set (not shared with other sets)
    // Must collect IDs before deleting SetTiles links
    QList<quint64> uniqueTileIDs;
    if (query.prepare(QStringLiteral("SELECT tileID FROM SetTiles WHERE tileID IN (%1)").arg(kUniqueTilesSubquery))) {
        query.addBindValue(id);
        if (query.exec()) {
            while (query.next()) {
                uniqueTileIDs.append(query.value(0).toULongLong());
            }
        }
    }

    // Remove set-tile links
    if (!query.prepare("DELETE FROM SetTiles WHERE setID = ?")) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to prepare SetTiles delete:" << query.lastError().text();
        return false;
    }
    query.addBindValue(id);
    if (!query.exec()) {
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
    if (!query.prepare("DELETE FROM TileSets WHERE setID = ?")) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to prepare TileSets delete:" << query.lastError().text();
        return false;
    }
    query.addBindValue(id);
    if (!query.exec()) {
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
    if (!query.prepare("UPDATE TileSets SET name = ? WHERE setID = ?")) {
        return false;
    }
    query.addBindValue(newName);
    query.addBindValue(setID);
    return query.exec();
}

std::optional<quint64> QGCTileCacheDatabase::findTileSetID(const QString& name)
{
    if (!_ensureConnected()) {
        return std::nullopt;
    }

    QSqlQuery query(_database());
    if (!query.prepare("SELECT setID FROM TileSets WHERE name = ?")) {
        return std::nullopt;
    }
    query.addBindValue(name);
    if (query.exec() && query.next()) {
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

    QGCSqlHelper::Transaction txn(_database());
    if (!txn.ok()) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to start transaction for resetDatabase";
        return false;
    }
    QSqlQuery query(_database());
    if (!dropTileCacheTables(query)) {
        return false;
    }
    if (!txn.commit()) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to commit table drops in resetDatabase";
        return false;
    }
    // Rebuilding the emptied file costs little. It releases the space of the dropped tables, and gives a file from
    // before auto-vacuum that mode, which takes a rebuild.
    if (!query.exec("PRAGMA auto_vacuum = INCREMENTAL") || !query.exec("VACUUM")) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to rebuild the reset database:" << query.lastError().text();
    }
    _valid = _createDB(_database());
    checkpointWriteAheadLog(query);
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
    if (!query.prepare("SELECT hash, type, x, y, z FROM TilesDownload WHERE setID = ? AND state = ? LIMIT ?")) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to prepare tile download list query:" << query.lastError().text();
        return tiles;
    }
    query.addBindValue(setID);
    query.addBindValue(static_cast<int>(QGCTile::StatePending));
    query.addBindValue(count);
    if (!query.exec()) {
        return tiles;
    }

    while (query.next()) {
        QGCTile tile;
        tile.hash = query.value(0).toString();
        tile.type = query.value(1).toInt();
        tile.x = query.value(2).toInt();
        tile.y = query.value(3).toInt();
        tile.z = query.value(4).toInt();
        tiles.append(std::move(tile));
    }

    if (!tiles.isEmpty()) {
        if (query.prepare(QStringLiteral("UPDATE TilesDownload SET state = ? WHERE setID = ? AND hash IN (%1)")
                              .arg(QGCSqlHelper::placeholders(tiles.size())))) {
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
    if (state == QGCTile::StateComplete) {
        if (!query.prepare("DELETE FROM TilesDownload WHERE setID = ? AND hash = ?")) {
            return false;
        }
        query.addBindValue(setID);
        query.addBindValue(hash);
    } else {
        if (!query.prepare("UPDATE TilesDownload SET state = ? WHERE setID = ? AND hash = ?")) {
            return false;
        }
        query.addBindValue(state);
        query.addBindValue(setID);
        query.addBindValue(hash);
    }

    if (!query.exec()) {
        qCWarning(QGCTileCacheDatabaseLog) << "Error:" << query.lastError().text();
        return false;
    }

    return true;
}

bool QGCTileCacheDatabase::updateAllTileDownloadStates(quint64 setID, int state)
{
    if (!_ensureConnected()) {
        return false;
    }

    QSqlQuery query(_database());
    if (!query.prepare("UPDATE TilesDownload SET state = ? WHERE setID = ?")) {
        return false;
    }
    query.addBindValue(state);
    query.addBindValue(setID);

    if (!query.exec()) {
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
                QStringLiteral("SELECT tileID, size, hash FROM Tiles WHERE tileID IN (%1) ORDER BY date ASC LIMIT ?")
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
        checkpointWriteAheadLog(query);
    }
    return pagesLeft;
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
    result.totalTileCount = totalTileCount;

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

    // A provider does not have every tile of a region. A set is complete with the tiles it saved once it has
    // tried them all, or it would look unfinished for good.
    if ((totalTileCount > result.savedTileCount) &&
        subquery.prepare("SELECT COUNT(*) FROM TilesDownload WHERE setID = ?")) {
        subquery.addBindValue(setID);
        if (subquery.exec() && subquery.next() && (subquery.value(0).toUInt() == 0)) {
            totalTileCount = result.savedTileCount;
            result.totalTileCount = totalTileCount;
        }
    }

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

std::optional<int> QGCTileCacheDatabase::_importableVersion(const QString& path, QString& errorString)
{
    std::optional<int> version;
    const QGCSqlHelper::ScopedConnection versionDB(path, /*readOnly=*/true, QStringLiteral("QGeoTileImportSession"));
    if (versionDB.isValid()) {
        QSqlDatabase db = versionDB.database();
        version = QGCSqlHelper::userVersion(db);
    }
    if (!version) {
        errorString = "Error opening import database";
        return std::nullopt;
    }
    // Version 0 cannot be migrated: see _checkSchemaVersion(), which discards a cache of that version
    if (*version < 1) {
        errorString = "Import database was written by a version of QGroundControl that is too old";
        return std::nullopt;
    }
    if (*version > kSchemaVersion) {
        errorString = "Import database was written by a newer version of QGroundControl";
        return std::nullopt;
    }
    return version;
}

DatabaseResult QGCTileCacheDatabase::importSetsReplace(const QString& path, ProgressCallback progressCb)
{
    DatabaseResult result;
    if (QFileInfo(path).canonicalFilePath() == QFileInfo(_databasePath).canonicalFilePath()) {
        result.errorString = "Import path must differ from the active database";
        return result;
    }
    if (!_importableVersion(path, result.errorString)) {
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
    const auto restoreCache = [this, hasBackup, &backupPath] {
        if (hasBackup) {
            (void) QFile::rename(backupPath, _databasePath);
        }
        if (init()) {
            connectDB();
        }
    };
    if (!QFile::copy(path, _databasePath)) {
        result.errorString = "Failed to copy import database";
        restoreCache();
        return result;
    }
    if (progressCb)
        progressCb(25);
    // The file is not the cache yet: one that cannot be migrated is not reset, and the cache is put back
    _resetOnMigrationFailure = false;
    const bool imported = init();
    _resetOnMigrationFailure = true;
    if (!imported) {
        result.errorString = QStringLiteral("Failed to initialize tile cache database after import");
        removeDatabaseFiles(_databasePath);
        restoreCache();
    } else {
        (void) QFile::remove(backupPath);
        if (progressCb)
            progressCb(50);
        connectDB();
        if (!_valid) {
            result.errorString = QStringLiteral("Failed to connect to tile cache database after import");
        }
    }
    if (progressCb)
        progressCb(100);
    result.success = result.errorString.isEmpty();
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

    const std::optional<int> importVersion = _importableVersion(path, result.errorString);
    if (!importVersion) {
        return result;
    }

    // An older file is migrated in a copy next to the cache: the file to import stays as it is
    QString importPath = path;
    const QString migratedPath = _databasePath + QStringLiteral(".import");
    const auto removeMigrated = qScopeGuard([&migratedPath] { removeDatabaseFiles(migratedPath); });
    if (*importVersion < kSchemaVersion) {
        // An import that died can have left the write-ahead log of its copy, which SQLite would apply to this one
        removeDatabaseFiles(migratedPath);
        if (!QFile::copy(path, migratedPath)) {
            result.errorString = "Failed to copy import database";
            return result;
        }
        // The copy has the permissions of the file, which can be read-only
        (void) QFile::setPermissions(migratedPath, QFile::ReadOwner | QFile::WriteOwner);

        QGCTileCacheDatabase migrated(migratedPath);
        migrated.setLegacyMapIds(_legacyMapIds);
        migrated.setCacheVariants(_cacheVariants);
        migrated._resetOnMigrationFailure = false;
        if (!migrated.init()) {
            result.errorString = "Failed to migrate import database";
            return result;
        }
        importPath = migratedPath;
    }

    QGCSqlHelper::ScopedConnection importDB(importPath, /*readOnly=*/true, QStringLiteral("QGeoTileImportSession"));
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
                TileSetRecord set;
                set.name = query.value("name").toString();
                const quint64 setID = query.value("setID").toULongLong();
                set.mapTypeStr = query.value("typeStr").toString();
                set.topleftLat = query.value("topleftLat").toDouble();
                set.topleftLon = query.value("topleftLon").toDouble();
                set.bottomRightLat = query.value("bottomRightLat").toDouble();
                set.bottomRightLon = query.value("bottomRightLon").toDouble();
                set.minZoom = query.value("minZoom").toInt();
                set.maxZoom = query.value("maxZoom").toInt();
                set.type = query.value("type").toInt();
                set.numTiles = query.value("numTiles").toUInt();
                const int defaultSet = query.value("defaultSet").toInt();
                quint64 insertSetID = _getDefaultTileSet();

                // Wrap each set creation + tile copy in a single transaction
                QGCSqlHelper::Transaction txn(_database());
                if (!txn.ok()) {
                    result.errorString = "Failed to start transaction for import set";
                    break;
                }

                if (defaultSet == 0) {
                    set.name = _deduplicateSetName(set.name);
                    set.date = static_cast<quint64>(QDateTime::currentSecsSinceEpoch());
                    QSqlQuery cQuery(_database());
                    bool prepared = false;
                    if (!insertTileCacheSet(cQuery, set, prepared)) {
                        result.errorString =
                            prepared ? "Error adding imported tile set to database" : "Error preparing tile set insert";
                        break;
                    }
                    insertSetID = cQuery.lastInsertId().toULongLong();
                }

                quint64 tilesIterated = 0;
                const quint64 tilesSaved = _copyTilesForSet(importDB.database(), setID, insertSetID, currentCount,
                                                            tileCount, lastProgress, progressCb, &tilesIterated);
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
                    qCWarning(QGCTileCacheDatabaseLog) << "Failed to commit import transaction for set:" << set.name;
                    continue;
                }

                if (tilesIterated > tilesSaved) {
                    const quint64 alreadyExisting = tilesIterated - tilesSaved;
                    tileCount = (alreadyExisting < tileCount) ? tileCount - alreadyExisting : 0;
                }

                if ((tilesSaved == 0) && (defaultSet == 0)) {
                    qCDebug(QGCTileCacheDatabaseLog) << "No unique tiles in" << set.name << "Removing it.";
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

    if (!_createDB(exportDB.database(), false)) {
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
        if (!query.prepare("SELECT T.hash, T.format, T.tile, T.type, T.date FROM Tiles T "
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
        bool prepared = false;
        if (!insertTileCacheSet(exportQuery, set, prepared)) {
            result.errorString =
                prepared ? "Error adding tile set to exported database" : "Error preparing tile set insert for export";
            break;
        }

        const quint64 exportSetID = exportQuery.lastInsertId().toULongLong();

        quint64 skippedTiles = 0;
        while (query.next()) {
            const QString hash = query.value(0).toString();
            const QString format = query.value(1).toString();
            const QByteArray img = query.value(2).toByteArray();
            const int tileType = query.value(3).toInt();
            const quint64 tileDate = query.value(4).toULongLong();

            const TileCacheCopy copied =
                copyTileToTileCacheSet(exportQuery, exportSetID, hash, format, img, tileType, tileDate);
            if (copied == TileCacheCopy::InsertNotPrepared) {
                qCWarning(QGCTileCacheDatabaseLog)
                    << "Failed to prepare tile INSERT for export:" << exportQuery.lastError().text();
                skippedTiles++;
                continue;
            }
            if (copied == TileCacheCopy::NoTile) {
                skippedTiles++;
            } else if (copied == TileCacheCopy::LinkFailed) {
                qCWarning(QGCTileCacheDatabaseLog)
                    << "Failed to link tile to set in export:" << exportQuery.lastError().text();
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

bool QGCTileCacheDatabase::_createDB(QSqlDatabase db, bool createDefault)
{
    // applySqlitePragmas (in connectDB / ScopedConnection ctor) already
    // enabled foreign_keys; nothing to redo here.
    QSqlQuery query(db);

    if (!query.exec("CREATE TABLE IF NOT EXISTS Tiles ("
                    "tileID INTEGER PRIMARY KEY NOT NULL, "
                    "hash TEXT NOT NULL UNIQUE, "
                    "format TEXT NOT NULL, "
                    "tile BLOB NULL, "
                    "size INTEGER, "
                    "type INTEGER, "
                    "date INTEGER DEFAULT 0)")) {
        qCWarning(QGCTileCacheDatabaseLog) << "Map Cache SQL error (create Tiles db):" << query.lastError().text();
        return false;
    }

    if (!query.exec("CREATE TABLE IF NOT EXISTS TileSets ("
                    "setID INTEGER PRIMARY KEY NOT NULL, "
                    "name TEXT NOT NULL UNIQUE, "
                    "typeStr TEXT, "
                    "topleftLat REAL DEFAULT 0.0, "
                    "topleftLon REAL DEFAULT 0.0, "
                    "bottomRightLat REAL DEFAULT 0.0, "
                    "bottomRightLon REAL DEFAULT 0.0, "
                    "minZoom INTEGER DEFAULT 3, "
                    "maxZoom INTEGER DEFAULT 3, "
                    "type INTEGER DEFAULT -1, "
                    "numTiles INTEGER DEFAULT 0, "
                    "defaultSet INTEGER DEFAULT 0, "
                    "date INTEGER DEFAULT 0)")) {
        qCWarning(QGCTileCacheDatabaseLog) << "Map Cache SQL error (create TileSets db):" << query.lastError().text();
        return false;
    }

    if (!query.exec("CREATE TABLE IF NOT EXISTS SetTiles ("
                    "setID INTEGER NOT NULL REFERENCES TileSets(setID) ON DELETE CASCADE, "
                    "tileID INTEGER NOT NULL REFERENCES Tiles(tileID) ON DELETE CASCADE)")) {
        qCWarning(QGCTileCacheDatabaseLog) << "Map Cache SQL error (create SetTiles db):" << query.lastError().text();
        return false;
    }

    if (!query.exec("CREATE TABLE IF NOT EXISTS TilesDownload ("
                    "setID INTEGER NOT NULL REFERENCES TileSets(setID) ON DELETE CASCADE, "
                    "hash TEXT NOT NULL, "
                    "type INTEGER, "
                    "x INTEGER, "
                    "y INTEGER, "
                    "z INTEGER, "
                    "state INTEGER DEFAULT 0)")) {
        qCWarning(QGCTileCacheDatabaseLog)
            << "Map Cache SQL error (create TilesDownload db):" << query.lastError().text();
        return false;
    }

    static const char* indexStatements[] = {
        "CREATE UNIQUE INDEX IF NOT EXISTS idx_settiles_unique ON SetTiles(tileID, setID)",
        "CREATE INDEX IF NOT EXISTS idx_settiles_setid ON SetTiles(setID)",
        "CREATE INDEX IF NOT EXISTS idx_settiles_tileid ON SetTiles(tileID)",
        "CREATE UNIQUE INDEX IF NOT EXISTS idx_tilesdownload_setid_hash ON TilesDownload(setID, hash)",
        "CREATE INDEX IF NOT EXISTS idx_tilesdownload_setid_state ON TilesDownload(setID, state)",
        "CREATE INDEX IF NOT EXISTS idx_tiles_date ON Tiles(date)",
    };
    for (const char* sql : indexStatements) {
        if (!query.exec(QLatin1String(sql))) {
            qCWarning(QGCTileCacheDatabaseLog) << "Failed to create index:" << sql << query.lastError().text();
        }
    }

    if (!QGCSqlHelper::setUserVersion(db, kSchemaVersion)) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to set schema version";
    }

    if (!createDefault) {
        return true;
    }

    if (!query.prepare("SELECT name FROM TileSets WHERE name = ?")) {
        qCWarning(QGCTileCacheDatabaseLog) << "Map Cache SQL error (prepare default set check):" << db.lastError();
        return false;
    }
    query.addBindValue(QStringLiteral("Default Tile Set"));
    if (!query.exec()) {
        qCWarning(QGCTileCacheDatabaseLog) << "Map Cache SQL error (Looking for default tile set):" << db.lastError();
        return true;
    }
    if (query.next()) {
        return true;
    }

    if (!query.prepare("INSERT INTO TileSets(name, defaultSet, date) VALUES(?, ?, ?)")) {
        qCWarning(QGCTileCacheDatabaseLog) << "Map Cache SQL error (prepare default tile set):" << db.lastError();
        return false;
    }
    query.addBindValue(QStringLiteral("Default Tile Set"));
    query.addBindValue(1);
    query.addBindValue(QDateTime::currentSecsSinceEpoch());
    if (!query.exec()) {
        qCWarning(QGCTileCacheDatabaseLog) << "Map Cache SQL error (Creating default tile set):" << db.lastError();
        return false;
    }

    return true;
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
    QString escaped = name;
    escaped.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
    escaped.replace(QLatin1Char('%'), QStringLiteral("\\%"));
    escaped.replace(QLatin1Char('_'), QStringLiteral("\\_"));
    if (query.prepare(QStringLiteral("SELECT name FROM TileSets WHERE name LIKE ? || ' %' ESCAPE '\\'"))) {
        query.addBindValue(escaped);
        if (query.exec()) {
            while (query.next()) {
                existing.insert(query.value(0).toString());
            }
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
                                               ProgressCallback progressCb, quint64* tilesIteratedOut)
{
    QSqlQuery subQuery(srcDB);
    subQuery.setForwardOnly(true);
    if (!subQuery.prepare("SELECT T.hash, T.format, T.tile, T.type, T.date FROM Tiles T "
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

    QSqlQuery cQuery(_database());
    while (subQuery.next()) {
        tilesFound++;
        const QString hash = subQuery.value(0).toString();
        const QString format = subQuery.value(1).toString();
        const QByteArray img = subQuery.value(2).toByteArray();
        const int tileType = subQuery.value(3).toInt();
        const quint64 tileDate = subQuery.value(4).toULongLong();

        if (copyTileToTileCacheSet(cQuery, dstSetID, hash, format, img, tileType, tileDate) == TileCacheCopy::Linked) {
            tilesLinked++;
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

    if (tilesIteratedOut)
        *tilesIteratedOut = tilesFound;
    return tilesLinked;
}
