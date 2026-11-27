#include <QtCore/QDateTime>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

#include "MapProvider.h"
#include "QGCSqlHelper.h"
#include "TileCacheDatabase.h"

/// Drops the tables of the cache. When one is left, the caller must not go on to _createDB(): that would mark the
/// rows of that table as rows of the current schema version.
static bool dropTileCacheTables(QSqlQuery& query)
{
    if (query.exec("DROP TABLE IF EXISTS TilesDownload") && query.exec("DROP TABLE IF EXISTS SetTiles") &&
        query.exec("DROP TABLE IF EXISTS Tiles") && query.exec("DROP TABLE IF EXISTS TileSets")) {
        return true;
    }
    qCWarning(TileCacheDatabaseLog) << "Failed to drop tables:" << query.lastError().text();
    return false;
}

bool TileCacheDatabase::_checkSchemaVersion()
{
    QSqlDatabase db = _database();
    const auto current = QGCSqlHelper::userVersion(db);
    if (!current) {
        qCWarning(TileCacheDatabaseLog) << "Failed to read schema version";
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
            qCWarning(TileCacheDatabaseLog)
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
            qCWarning(TileCacheDatabaseLog) << "Failed to migrate schema version" << version << "to" << kSchemaVersion;
            return false;
        }
        qCWarning(TileCacheDatabaseLog) << "Failed to migrate schema version" << version << "to" << kSchemaVersion
                                        << ". Resetting cache.";
    } else {
        qCWarning(TileCacheDatabaseLog) << "Unknown schema version" << version << "(expected" << kSchemaVersion
                                        << "). Resetting cache.";
    }
    _defaultSet = kInvalidTileSet;
    return dropTileCacheTables(query);
}

bool TileCacheDatabase::_migrate(QSqlDatabase db, int version)
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

bool TileCacheDatabase::_renumberMapIds(QSqlDatabase db, const QMap<int, int>& legacyMapIds)
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
            qCWarning(TileCacheDatabaseLog) << "Cannot renumber provider" << *legacyId << "to the lower id" << mapId;
            return false;
        }

        // The hash starts with the id as 10 digits. Rows are selected by hash because that uses its index: type has
        // none, and reading it reads the tile image stored before it.
        const QVariantList hashValues = {QString::asprintf("%010d", mapId), mapId,
                                         QString::asprintf("%010d", *legacyId),
                                         QString::asprintf("%010d", *legacyId + 1)};
        static const QString hashSql =
            QStringLiteral("UPDATE %1 SET hash = ? || substr(hash, 11), type = ? WHERE hash >= ? AND hash < ?");
        if (!_execBound(query, hashSql.arg(QStringLiteral("Tiles")), hashValues) ||
            !_execBound(query, hashSql.arg(QStringLiteral("TilesDownload")), hashValues) ||
            !_execBound(query, QStringLiteral("UPDATE TileSets SET type = ? WHERE type = ?"), {mapId, *legacyId})) {
            qCWarning(TileCacheDatabaseLog)
                << "Failed to renumber provider" << *legacyId << "to" << mapId << ":" << query.lastError().text();
            return false;
        }
    }

    return true;
}

bool TileCacheDatabase::_adoptCacheVariants(QSqlDatabase db, const QHash<int, QString>& cacheVariants)
{
    QSqlQuery query(db);

    // Until version 3 a provider's tiles had one key whatever source the user had configured for it, so which source
    // a tile is from is not known. They become tiles of the source configured now.
    for (auto it = cacheVariants.cbegin(); it != cacheVariants.cend(); ++it) {
        const QString suffix = MapProvider::cacheVariantSuffix(it.value());
        if (suffix.isEmpty()) {
            continue;
        }
        const QVariantList values = {suffix, QString::asprintf("%010d", it.key()),
                                     QString::asprintf("%010d", it.key() + 1)};
        static const QString sql = QStringLiteral("UPDATE %1 SET hash = hash || ? WHERE hash >= ? AND hash < ?");
        if (!_execBound(query, sql.arg(QStringLiteral("Tiles")), values) ||
            !_execBound(query, sql.arg(QStringLiteral("TilesDownload")), values)) {
            qCWarning(TileCacheDatabaseLog)
                << "Failed to add the cache variant of provider" << it.key() << ":" << query.lastError().text();
            return false;
        }
    }

    return true;
}

bool TileCacheDatabase::resetDatabase()
{
    if (!_ensureConnected()) {
        return false;
    }

    _defaultSet = kInvalidTileSet;

    QGCSqlHelper::Transaction txn(_database());
    if (!txn.ok()) {
        qCWarning(TileCacheDatabaseLog) << "Failed to start transaction for resetDatabase";
        return false;
    }
    QSqlQuery query(_database());
    if (!dropTileCacheTables(query)) {
        return false;
    }
    if (!txn.commit()) {
        qCWarning(TileCacheDatabaseLog) << "Failed to commit table drops in resetDatabase";
        return false;
    }
    // Rebuilding the emptied file costs little. It releases the space of the dropped tables, and gives a file from
    // before auto-vacuum that mode, which takes a rebuild.
    if (!query.exec("PRAGMA auto_vacuum = INCREMENTAL") || !query.exec("VACUUM")) {
        qCWarning(TileCacheDatabaseLog) << "Failed to rebuild the reset database:" << query.lastError().text();
    }
    _valid = _createDB(_database());
    _checkpointWriteAheadLog(query);
    return _valid;
}

bool TileCacheDatabase::_createDB(QSqlDatabase db, bool createDefault)
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
        qCWarning(TileCacheDatabaseLog) << "Map Cache SQL error (create Tiles db):" << query.lastError().text();
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
        qCWarning(TileCacheDatabaseLog) << "Map Cache SQL error (create TileSets db):" << query.lastError().text();
        return false;
    }

    if (!query.exec("CREATE TABLE IF NOT EXISTS SetTiles ("
                    "setID INTEGER NOT NULL REFERENCES TileSets(setID) ON DELETE CASCADE, "
                    "tileID INTEGER NOT NULL REFERENCES Tiles(tileID) ON DELETE CASCADE)")) {
        qCWarning(TileCacheDatabaseLog) << "Map Cache SQL error (create SetTiles db):" << query.lastError().text();
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
        qCWarning(TileCacheDatabaseLog) << "Map Cache SQL error (create TilesDownload db):" << query.lastError().text();
        return false;
    }

    static const char* indexStatements[] = {
        "CREATE UNIQUE INDEX IF NOT EXISTS idx_settiles_unique ON SetTiles(tileID, setID)",
        "CREATE INDEX IF NOT EXISTS idx_settiles_setid ON SetTiles(setID)",
        "CREATE UNIQUE INDEX IF NOT EXISTS idx_tilesdownload_setid_hash ON TilesDownload(setID, hash)",
        "CREATE INDEX IF NOT EXISTS idx_tilesdownload_setid_state ON TilesDownload(setID, state)",
        "CREATE INDEX IF NOT EXISTS idx_tiles_date ON Tiles(date)",
        // The sizes of the tiles without their rows, which have the pictures: for computeTotals()
        "CREATE INDEX IF NOT EXISTS idx_tiles_size ON Tiles(tileID, size)",
    };
    for (const char* sql : indexStatements) {
        if (!query.exec(QLatin1String(sql))) {
            qCWarning(TileCacheDatabaseLog) << "Failed to create index:" << sql << query.lastError().text();
        }
    }
    // Earlier versions had an index of the links by their tile as well. The unique one, which begins with the tile,
    // serves every statement that it served: it only cost each link a third index to write.
    if (!query.exec(QStringLiteral("DROP INDEX IF EXISTS idx_settiles_tileid"))) {
        qCWarning(TileCacheDatabaseLog) << "Failed to drop index:" << query.lastError().text();
    }

    if (!QGCSqlHelper::setUserVersion(db, kSchemaVersion)) {
        qCWarning(TileCacheDatabaseLog) << "Failed to set schema version";
    }

    if (!createDefault) {
        return true;
    }

    if (!query.prepare("SELECT name FROM TileSets WHERE name = ?")) {
        qCWarning(TileCacheDatabaseLog) << "Map Cache SQL error (prepare default set check):"
                                        << query.lastError().text();
        return false;
    }
    query.addBindValue(QStringLiteral("Default Tile Set"));
    if (!query.exec()) {
        qCWarning(TileCacheDatabaseLog) << "Map Cache SQL error (Looking for default tile set):"
                                        << query.lastError().text();
        return false;
    }
    if (query.next()) {
        return true;
    }

    return _execOrWarn(query, QStringLiteral("INSERT INTO TileSets(name, defaultSet, date) VALUES(?, ?, ?)"),
                       {QStringLiteral("Default Tile Set"), 1, QDateTime::currentSecsSinceEpoch()},
                       "create default tile set");
}
