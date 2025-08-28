/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCTileCacheDatabase.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QThread>
#include <QtCore/QUuid>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

#include "QGCLoggingCategory.h"
#include "QGCTile.h"

#ifdef QT_DEBUG
#define DBG_DB_THREAD() Q_ASSERT(QThread::currentThreadId() == _ownerThreadId)
#else
#define DBG_DB_THREAD() do {} while(0);
#endif

using namespace Qt::StringLiterals;

QGC_LOGGING_CATEGORY(QGCTileCacheDatabaseLog, "QtLocationPlugin.QGCTileCacheDatabase")

/*===========================================================================*/

TransactionGuard::TransactionGuard(QGCTileCacheDatabase &db)
    : _db(db)
{
    qCDebug(QGCTileCacheDatabaseLog) << this << _db._databasePath;

    _active = _db.beginTransaction();
}

TransactionGuard::~TransactionGuard()
{
    if (_active && !_committed) {
        (void) _db.rollbackTransaction();
    }

    qCDebug(QGCTileCacheDatabaseLog) << this << _db._databasePath;
}

void TransactionGuard::commit()
{
    if (_active && _db.commitTransaction()) {
        _committed = true;
        _active = false;
    }
}

void TransactionGuard::rollback()
{
    if (_active && _db.rollbackTransaction()) {
        _committed = true;
        _active = false;
    }
}

/*===========================================================================*/

QGCTileCacheDatabase::QGCTileCacheDatabase(const QString &connectionName, const QString &databasePath)
    : _connectionName(connectionName)
    , _databasePath(databasePath)
{
    qCDebug(QGCTileCacheDatabaseLog) << this << "Creating database connection:" << connectionName;
}

QGCTileCacheDatabase::~QGCTileCacheDatabase()
{
    close();

    qCDebug(QGCTileCacheDatabaseLog) << this << "Destroyed database connection:" << _connectionName;
}

/*---------------------------------------------------------------------------*/
// Database Helpers

QSqlDatabase QGCTileCacheDatabase::_getDatabase() const
{
    DBG_DB_THREAD();

    if (!QCoreApplication::instance()) {
        qCWarning(QGCTileCacheDatabaseLog) << "No QCoreApplication instance";
        return QSqlDatabase();
    }

    if (!QSqlDatabase::contains(_connectionName)) {
        qCWarning(QGCTileCacheDatabaseLog) << "No Connection of name:" << _connectionName;
        return QSqlDatabase();
    }

    QSqlDatabase db = QSqlDatabase::database(_connectionName);
    if (!db.isValid()) {
        qCWarning(QGCTileCacheDatabaseLog) << "Database is invalid:";
    }

    return db;
}

bool QGCTileCacheDatabase::_createDatabase()
{
    QMutexLocker lock(&_dbMutex);

    if (!QCoreApplication::instance()) {
        qCWarning(QGCTileCacheDatabaseLog) << "No QCoreApplication instance";
        _lastError = u"No QCoreApplication instance"_s;
        return false;
    }

    if (QSqlDatabase::contains(_connectionName)) {
        qCWarning(QGCTileCacheDatabaseLog) << "Database already exists";
        return true;
    }

    const QFileInfo dbInfo(_databasePath);
    const QDir dbDir = dbInfo.absoluteDir();

    if (!dbDir.exists() && !dbDir.mkpath(u"."_s)) {
        _lastError = u"Cannot create database directory: %1"_s.arg(dbDir.absolutePath());
        qCCritical(QGCTileCacheDatabaseLog) << _lastError;
        return false;
    }

    if (dbInfo.exists() && !dbInfo.isWritable()) {
        _lastError = u"Database file is not writable: %1"_s.arg(_databasePath);
        qCCritical(QGCTileCacheDatabaseLog) << _lastError;
        return false;
    }

    if (!QSqlDatabase::isDriverAvailable(u"QSQLITE"_s)) {
        _lastError = u"SQLite Unavailable"_s;
        qCCritical(QGCTileCacheDatabaseLog) << "SQLite Unavailable";
        return false;
    }

    qCDebug(QGCTileCacheDatabaseLog) << "Creating database at:" << _databasePath;

    QSqlDatabase db = QSqlDatabase::addDatabase(u"QSQLITE"_s, _connectionName);
    if (!db.isValid()) {
        _lastError = u"Database is not valid"_s;
        qCCritical(QGCTileCacheDatabaseLog) << "Database is not valid";
        return false;
    }

    db.setDatabaseName(_databasePath);
    db.setConnectOptions(u"QSQLITE_BUSY_TIMEOUT=%1"_s.arg(kBusyTimeout));

    return true;
}

bool QGCTileCacheDatabase::_enableOptimizations() const
{
    QSqlDatabase db = _getDatabase();
    if (!db.isValid()) {
        return false;
    }

    QSqlQuery query(db);

    // Enable Write-Ahead Logging for better concurrency
    (void) query.exec(u"PRAGMA journal_mode=WAL"_s);

    // Faster writes with acceptable safety
    (void) query.exec(u"PRAGMA synchronous=NORMAL"_s);

    // Larger cache for better performance (~10 MiB). Negative = kibibytes
    (void) query.exec(u"PRAGMA cache_size=%1"_s.arg(-kCacheSize));

    // Keep temp data in memory
    (void) query.exec(u"PRAGMA temp_store=MEMORY"_s);

    // Enable foreign keys
    (void) query.exec(u"PRAGMA foreign_keys=ON"_s);

    // Auto-analyze for query optimization
    (void) query.exec(u"PRAGMA optimize"_s);

    // (void) query.exec("PRAGMA mmap_size=268435456");     // 256MB memory-mapped I/O
    // (void) query.exec("PRAGMA page_size=4096");          // Optimize for SSDs
    // (void) query.exec("PRAGMA auto_vacuum=INCREMENTAL"); // Auto-vacuum to prevent fragmentation
    // compile_options
    // query_only

    return true;
}

bool QGCTileCacheDatabase::_createIndexes() const
{
    const QSqlDatabase db = _getDatabase();
    QSqlQuery query(db);

    // Essential indexes for performance
    const QStringList indexes = {
        u"CREATE INDEX IF NOT EXISTS idx_tiles_type_date ON Tiles(type, date DESC)"_s,
        u"CREATE INDEX IF NOT EXISTS idx_tiles_access ON Tiles(lastAccess DESC, accessCount DESC)"_s,
        u"CREATE INDEX IF NOT EXISTS idx_settiles_setid ON SetTiles(setID)"_s,
        u"CREATE INDEX IF NOT EXISTS idx_settiles_tileid ON SetTiles(tileID)"_s,
        u"CREATE INDEX IF NOT EXISTS idx_download_pending ON TilesDownload(setID, state) WHERE state = 0"_s,
        u"CREATE INDEX IF NOT EXISTS idx_tilesets_default ON TileSets(defaultSet) WHERE defaultSet = 1"_s
    };

    for (const QString &sql : indexes) {
        if (!query.exec(sql)) {
            qCWarning(QGCTileCacheDatabaseLog) << "Failed to create index:" << query.lastError().text();
        }
    }

    return true;
}

bool QGCTileCacheDatabase::_executeQuery(const QString &query) const
{
    if (!isOpen()) {
        return false;
    }

    const QSqlDatabase db = _getDatabase();
    QSqlQuery q(db);
    return q.exec(query);
}

bool QGCTileCacheDatabase::_validateDatabase() const
{
    if (!isOpen()) {
        return false;
    }

    const QSqlDatabase db = _getDatabase();
    QSqlQuery query(db);

    // Check that all required tables exist
    const QStringList requiredTables = {
        u"Tiles"_s,
        u"TileSets"_s,
        u"SetTiles"_s,
        u"TilesDownload"_s
    };

    for (const QString &table : requiredTables) {
        (void) query.prepare(u"SELECT name FROM sqlite_master WHERE type='table' AND name=?"_s);
        query.addBindValue(table);
        if (!query.exec() || !query.next()) {
            qCWarning(QGCTileCacheDatabaseLog) << "Missing table:" << table;
            return false;
        }
    }

    return true;
}

/*---------------------------------------------------------------------------*/
// Database creation and management

bool QGCTileCacheDatabase::open()
{
    close();

    if (!_createDatabase()) {
        return false;
    }

    QSqlDatabase db = _getDatabase();
    if (!db.isValid()) {
        _lastError = u"Invalid database connection"_s;
        return false;
    }

    if (!db.open()) {
        // db.isOpenError()
        _lastError = db.lastError().text();
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to open database:" << _lastError;
        return false;
    }

    _ownerThreadId = QThread::currentThreadId();

    if (!_enableOptimizations()) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to enable optimizations";
    }

    return true;
}

void QGCTileCacheDatabase::close()
{
    QMutexLocker lock(&_dbMutex);

    QSqlDatabase db = QSqlDatabase::database(_connectionName, false);
    if (db.isValid() && db.isOpen()) {
        db.close();
    }

    if (QSqlDatabase::contains(_connectionName)) {
        QSqlDatabase::removeDatabase(_connectionName);
    }

    QMutexLocker cacheLock(&_cacheMutex);
    _defaultSetCache = UINT64_MAX;
}

bool QGCTileCacheDatabase::isOpen() const
{
    const QSqlDatabase db = _getDatabase();
    return (db.isValid() && db.isOpen());
}

bool QGCTileCacheDatabase::createSchema(bool createDefault)
{
    if (!isOpen()) {
        _lastError = u"Database not open"_s;
        return false;
    }

    TransactionGuard transaction(*this);
    if (!transaction.isActive()) {
        return false;
    }

    const QSqlDatabase db = _getDatabase();
    QSqlQuery query(db);

    // Create Tiles table
    if (!query.exec(
        u"CREATE TABLE IF NOT EXISTS Tiles ("
        "tileID INTEGER PRIMARY KEY AUTOINCREMENT, "
        "hash TEXT NOT NULL UNIQUE, "
        "format TEXT NOT NULL, "
        "tile BLOB, "
        "size INTEGER NOT NULL DEFAULT 0, "
        "type INTEGER DEFAULT -1, "
        "date INTEGER DEFAULT (strftime('%s', 'now')), "
        "accessCount INTEGER DEFAULT 0, "
        "lastAccess INTEGER DEFAULT (strftime('%s', 'now'))"
        ")"_s))
    {
        _lastError = query.lastError().text();
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to create Tiles table:" << _lastError;
        return false;
    }

    // Create TileSets table with constraints
    if (!query.exec(
        u"CREATE TABLE IF NOT EXISTS TileSets ("
        "setID INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT NOT NULL UNIQUE COLLATE NOCASE, "
        "typeStr TEXT, "
        "topleftLat REAL DEFAULT 0.0 CHECK(topleftLat >= -90 AND topleftLat <= 90), "
        "topleftLon REAL DEFAULT 0.0 CHECK(topleftLon >= -180 AND topleftLon <= 180), "
        "bottomRightLat REAL DEFAULT 0.0 CHECK(bottomRightLat >= -90 AND bottomRightLat <= 90), "
        "bottomRightLon REAL DEFAULT 0.0 CHECK(bottomRightLon >= -180 AND bottomRightLon <= 180), "
        "minZoom INTEGER DEFAULT 3 CHECK(minZoom >= 0 AND minZoom <= 30), "
        "maxZoom INTEGER DEFAULT 3 CHECK(maxZoom >= 0 AND maxZoom <= 30), "
        "type INTEGER DEFAULT -1, "
        "numTiles INTEGER DEFAULT 0 CHECK(numTiles >= 0), "
        "defaultSet INTEGER DEFAULT 0, "
        "date INTEGER DEFAULT (strftime('%s', 'now')), "
        "CHECK(maxZoom >= minZoom)"
        ")"_s))
    {
        _lastError = query.lastError().text();
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to create TileSets table:" << _lastError;
        return false;
    }

    // Create SetTiles table with foreign keys
    if (!query.exec(
        u"CREATE TABLE IF NOT EXISTS SetTiles ("
        "setID INTEGER NOT NULL, "
        "tileID INTEGER NOT NULL, "
        "PRIMARY KEY (setID, tileID), "
        "FOREIGN KEY (setID) REFERENCES TileSets(setID) ON DELETE CASCADE, "
        "FOREIGN KEY (tileID) REFERENCES Tiles(tileID) ON DELETE CASCADE"
        ") WITHOUT ROWID"_s))
    {
        _lastError = query.lastError().text();
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to create SetTiles table:" << _lastError;
        return false;
    }

    // Migrate existing NULL type values to -1 for backward compatibility
    if (!query.exec(u"UPDATE Tiles SET type = -1 WHERE type IS NULL"_s)) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to migrate NULL type values in Tiles:" << query.lastError().text();
        // Non-fatal, continue
    }

    if (!query.exec(u"UPDATE TileSets SET type = -1 WHERE type IS NULL"_s)) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to migrate NULL type values in TileSets:" << query.lastError().text();
        // Non-fatal, continue
    }

    // Create TilesDownload table with better state tracking
    if (!query.exec(
        u"CREATE TABLE IF NOT EXISTS TilesDownload ("
        "setID INTEGER NOT NULL, "
        "hash TEXT NOT NULL, "
        "type INTEGER DEFAULT -1, "
        "x INTEGER, "
        "y INTEGER, "
        "z INTEGER, "
        "state INTEGER DEFAULT 0, "
        "retryCount INTEGER DEFAULT 0, "
        "lastAttempt INTEGER DEFAULT 0, "
        "PRIMARY KEY (setID, hash), "
        "FOREIGN KEY (setID) REFERENCES TileSets(setID) ON DELETE CASCADE"
        ") WITHOUT ROWID"_s))
    {
        _lastError = query.lastError().text();
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to create TilesDownload table:" << _lastError;
        return false;
    }

    // Create indexes
    if (!_createIndexes()) {
        return false;
    }

    // Create default tile set if requested
    if (createDefault) {
        if (query.exec(u"SELECT setID FROM TileSets WHERE defaultSet = 1"_s)) {
            if (!query.next()) {
                (void) query.prepare(u"INSERT INTO TileSets(name, defaultSet, date) VALUES(?, 1, ?)"_s);
                query.addBindValue(u"Default Tile Set"_s);
                query.addBindValue(QDateTime::currentSecsSinceEpoch());
                if (!query.exec()) {
                    _lastError = query.lastError().text();
                    qCWarning(QGCTileCacheDatabaseLog) << "Failed to create default tile set:" << _lastError;
                    return false;
                }
            }
        }
    }

    transaction.commit();
    return true;
}

bool QGCTileCacheDatabase::reset()
{
    if (!isOpen()) {
        _lastError = u"Database not open"_s;
        return false;
    }

    const QSqlDatabase db = _getDatabase();
    QSqlQuery query(db);

    (void) query.exec(u"DROP TABLE IF EXISTS TilesDownload"_s);
    (void) query.exec(u"DROP TABLE IF EXISTS SetTiles"_s);
    (void) query.exec(u"DROP TABLE IF EXISTS TileSets"_s);
    (void) query.exec(u"DROP TABLE IF EXISTS Tiles"_s);

    // Vacuum to reclaim space
    (void) query.exec(u"VACUUM"_s);
    QMutexLocker cacheLock(&_cacheMutex);
    _defaultSetCache = UINT64_MAX;
    cacheLock.unlock();

    return createSchema(true);
}

bool QGCTileCacheDatabase::vacuum()
{
    if (!isOpen()) {
        _lastError = u"Database not open"_s;
        return false;
    }

    const QSqlDatabase db = _getDatabase();
    QSqlQuery query(db);
    return query.exec(u"VACUUM"_s);
}

bool QGCTileCacheDatabase::analyze()
{
    if (!isOpen()) {
        _lastError = u"Database not open"_s;
        return false;
    }

    const QSqlDatabase db = _getDatabase();
    QSqlQuery query(db);
    return query.exec(u"ANALYZE"_s);
}

/*---------------------------------------------------------------------------*/
// Tile Operations

bool QGCTileCacheDatabase::saveTile(const TileInfo &tile, quint64 setID)
{
    DBG_DB_THREAD();

    if (!isOpen()) {
        _lastError = u"Database not open"_s;
        return false;
    }

    QMutexLocker lock(&_metricsMutex);
    _metrics.totalWrites++;
    lock.unlock();

    const QSqlDatabase db = _getDatabase();
    QSqlQuery query(db);

    if (!query.prepare(u"INSERT OR IGNORE INTO Tiles(hash, format, tile, size, type, date) "
                       "VALUES(?, ?, ?, ?, ?, ?)"_s)) {
        _lastError = query.lastError().text();
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to prepare saveTile query:" << _lastError;
        return false;
    }

    query.addBindValue(tile.hash);
    query.addBindValue(tile.format);
    query.addBindValue(tile.tile);
    query.addBindValue((tile.size > 0) ? tile.size : tile.tile.size());
    query.addBindValue(tile.type);
    query.addBindValue((tile.date > 0) ? tile.date : QDateTime::currentSecsSinceEpoch());

    if (!query.exec()) {
        // Tile might already exist - not necessarily an error
        _lastError = query.lastError().text();
        return false;
    }

    bool ok = false;
    const quint64 tileID = query.lastInsertId().toULongLong(&ok);
    if (!ok) {
        qCWarning(QGCTileCacheDatabaseLog) << "Invalid tileID for hash:" << tile.hash;
    }
    if (tileID == 0) {
        // Tile already exists, get its ID
        const quint64 existingID = getTileID(tile.hash);
        if (existingID > 0) {
            return addTileToSet(existingID, setID);
        }
        return false;
    }

    return addTileToSet(tileID, setID);
}

bool QGCTileCacheDatabase::saveTileBatch(const QList<TileInfo> &tiles, quint64 setID)
{
    if (!isOpen() || tiles.isEmpty()) {
        return false;
    }

    TransactionGuard transaction(*this);
    if (!transaction.isActive()) {
        return false;
    }

    const QSqlDatabase db = _getDatabase();
    QSqlQuery insertQuery(db);
    QSqlQuery findQuery(db);
    QSqlQuery setQuery(db);

    if (!insertQuery.prepare(u"INSERT OR IGNORE INTO Tiles(hash, format, tile, size, type, date) "
                              "VALUES(?, ?, ?, ?, ?, ?)"_s)) {
        _lastError = insertQuery.lastError().text();
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to prepare insertQuery:" << _lastError;
        return false;
    }

    if (!findQuery.prepare(u"SELECT tileID FROM Tiles WHERE hash = ?"_s)) {
        _lastError = findQuery.lastError().text();
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to prepare findQuery:" << _lastError;
        return false;
    }

    if (!setQuery.prepare(u"INSERT OR IGNORE INTO SetTiles(setID, tileID) VALUES(?, ?)"_s)) {
        _lastError = setQuery.lastError().text();
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to prepare setQuery:" << _lastError;
        return false;
    }

    int processed = 0;
    int failed = 0;

    for (const TileInfo &tile : tiles) {
        insertQuery.addBindValue(tile.hash);
        insertQuery.addBindValue(tile.format);
        insertQuery.addBindValue(tile.tile);
        insertQuery.addBindValue((tile.size > 0) ? tile.size : tile.tile.size());
        insertQuery.addBindValue(tile.type);
        insertQuery.addBindValue((tile.date > 0) ? tile.date : QDateTime::currentSecsSinceEpoch());

        if (!insertQuery.exec()) {
            qCWarning(QGCTileCacheDatabaseLog) << "Failed to insert tile in batch:" << tile.hash << "-" << insertQuery.lastError().text();
            failed++;
            continue;  // Continue with next tile instead of aborting entire batch
        }

        bool ok = false;
        quint64 tileID = insertQuery.lastInsertId().toULongLong(&ok);
        if (!ok) {
            qCWarning(QGCTileCacheDatabaseLog) << "Invalid tileID for hash:" << tile.hash;
        }
        if (tileID == 0) {
            // Get existing tile ID
            findQuery.addBindValue(tile.hash);
            if (findQuery.exec() && findQuery.next()) {
                tileID = findQuery.value(0).toULongLong(&ok);
                if (!ok) {
                    qCWarning(QGCTileCacheDatabaseLog) << "Invalid tileID for hash:" << tile.hash;
                }
            }
        }

        if (tileID > 0) {
            setQuery.addBindValue(setID);
            setQuery.addBindValue(tileID);
            if (!setQuery.exec()) {
                qCWarning(QGCTileCacheDatabaseLog) << "Failed to add tile to set:" << tile.hash;
                failed++;
            }
        }

        processed++;
    }

    transaction.commit();

    // Return success if we processed at least some tiles
    if (processed > 0 && failed < tiles.count()) {
        return true;
    }

    // All tiles failed
    if (failed == tiles.count()) {
        _lastError = u"All tiles in batch failed to save"_s;
        return false;
    }

    return true;
}

bool QGCTileCacheDatabase::getTile(const QString &hash, TileInfo &tile)
{
    DBG_DB_THREAD();

    if (!isOpen()) {
        _lastError = u"Database not open"_s;
        return false;
    }

    QMutexLocker lock(&_metricsMutex);
    _metrics.totalReads++;
    lock.unlock();

    const QSqlDatabase db = _getDatabase();
    QSqlQuery query(db);

    if (!query.prepare(u"SELECT * FROM Tiles WHERE hash = ?"_s)) {
        _lastError = query.lastError().text();
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to prepare getTile query:" << _lastError;
        return false;
    }

    query.addBindValue(hash);

    if (!query.exec()) {
        _lastError = query.lastError().text();
        return false;
    }

    if (!query.next()) {
        return false;
    }

    bool ok = false;

    tile.tileID = query.value(u"tileID"_s).toULongLong(&ok);
    if (!ok) {
        qCWarning(QGCTileCacheDatabaseLog) << "Invalid tileID for hash:" << hash;
        return false;
    }

    tile.hash = query.value(u"hash"_s).toString();
    tile.format = query.value(u"format"_s).toString();
    tile.tile = query.value(u"tile"_s).toByteArray();

    tile.size = query.value(u"size"_s).toLongLong(&ok);
    if (!ok || (tile.size < 0)) {
        qCWarning(QGCTileCacheDatabaseLog) << "Invalid size for hash:" << hash;
        tile.size = tile.tile.size();
    }

    tile.type = query.value(u"type"_s).toInt(&ok);
    if (!ok) {
        qCWarning(QGCTileCacheDatabaseLog) << "Invalid type for hash:" << hash;
        tile.type = -1;
    }

    tile.date = query.value(u"date"_s).toLongLong(&ok);
    if (!ok) {
        qCWarning(QGCTileCacheDatabaseLog) << "Invalid date for hash:" << hash;
        tile.date = 0;
    }

    // Update access statistics (this modifies database, not object state, so const_cast is acceptable)
    // However, we make updateTileAccess const to avoid the cast
    (void) updateTileAccess(hash);

    return true;
}

quint64 QGCTileCacheDatabase::getTileID(const QString &hash) const
{
    if (!isOpen()) {
        return 0;
    }

    const QSqlDatabase db = _getDatabase();
    QSqlQuery query(db);

    if (!query.prepare(u"SELECT tileID FROM Tiles WHERE hash = ?"_s)) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to prepare getTileID query:" << query.lastError().text();
        return 0;
    }

    query.addBindValue(hash);

    if (query.exec() && query.next()) {
        bool ok = false;
        const quint64 id = query.value(0).toULongLong(&ok);
        if (ok) {
            return id;
        }
        qCWarning(QGCTileCacheDatabaseLog) << "Invalid tileID for hash:" << hash;
    }

    return 0;
}

bool QGCTileCacheDatabase::deleteTile(quint64 tileID)
{
    if (!isOpen()) {
        _lastError = u"Database not open"_s;
        return false;
    }

    const QSqlDatabase db = _getDatabase();
    QSqlQuery query(db);

    if (!query.prepare(u"DELETE FROM Tiles WHERE tileID = ?"_s)) {
        _lastError = query.lastError().text();
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to prepare deleteTile query:" << _lastError;
        return false;
    }

    query.addBindValue(tileID);

    if (!query.exec()) {
        _lastError = query.lastError().text();
        return false;
    }

    return true;
}

bool QGCTileCacheDatabase::deleteTilesMatchingBytes(const QByteArray &bytes)
{
    if (!isOpen()) {
        _lastError = u"Database not open"_s;
        return false;
    }

    TransactionGuard transaction(*this);
    if (!transaction.isActive()) {
        return false;
    }

    const QSqlDatabase db = _getDatabase();
    QSqlQuery query(db);

    // First, find matching tiles
    if (!query.prepare(u"SELECT tileID, tile FROM Tiles WHERE LENGTH(tile) = ?"_s)) {
        _lastError = query.lastError().text();
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to prepare deleteTilesMatchingBytes query:" << _lastError;
        return false;
    }

    query.addBindValue(bytes.length());

    if (!query.exec()) {
        _lastError = query.lastError().text();
        return false;
    }

    QList<quint64> idsToDelete;
    while (query.next()) {
        if (query.value(1).toByteArray() == bytes) {
            bool ok = false;
            const quint64 id = query.value(0).toULongLong(&ok);
            if (ok) {
                idsToDelete.append(id);
            } else {
                qCWarning(QGCTileCacheDatabaseLog) << "Invalid tileID";
            }
        }
    }

    for (const quint64 tileId : idsToDelete) {
        if (!query.prepare(u"DELETE FROM Tiles WHERE tileID = ?"_s)) {
            _lastError = query.lastError().text();
            qCWarning(QGCTileCacheDatabaseLog) << "Failed to prepare delete query:" << _lastError;
            return false;
        }
        query.addBindValue(tileId);
        if (!query.exec()) {
            _lastError = query.lastError().text();
            return false;
        }
    }

    transaction.commit();
    return true;
}

bool QGCTileCacheDatabase::updateTileAccess(const QString &hash) const
{
    if (!isOpen()) {
        return false;
    }

    const QSqlDatabase db = _getDatabase();
    QSqlQuery query(db);

    if (!query.prepare(u"UPDATE Tiles SET accessCount = accessCount + 1, "
                       "lastAccess = strftime('%s', 'now') WHERE hash = ?"_s)) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to prepare updateTileAccess query:" << query.lastError().text();
        return false;
    }

    query.addBindValue(hash);
    return query.exec();
}

/*---------------------------------------------------------------------------*/
// Tile Set Operations

bool QGCTileCacheDatabase::createTileSet(const TileSetInfo &info, quint64 &setID)
{
    if (!isOpen()) {
        _lastError = u"Database not open"_s;
        return false;
    }

    const QSqlDatabase db = _getDatabase();
    QSqlQuery query(db);

    if (!query.prepare(
        u"INSERT INTO TileSets("
        "name, typeStr, topleftLat, topleftLon, bottomRightLat, bottomRightLon, "
        "minZoom, maxZoom, type, numTiles, defaultSet, date"
        ") VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"_s)) {
        _lastError = query.lastError().text();
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to prepare createTileSet query:" << _lastError;
        return false;
    }

    query.addBindValue(info.name);
    query.addBindValue(info.typeStr);
    query.addBindValue(info.topleftLat);
    query.addBindValue(info.topleftLon);
    query.addBindValue(info.bottomRightLat);
    query.addBindValue(info.bottomRightLon);
    query.addBindValue(info.minZoom);
    query.addBindValue(info.maxZoom);
    query.addBindValue(info.type);
    query.addBindValue(info.numTiles);
    query.addBindValue(info.defaultSet ? 1 : 0);
    query.addBindValue((info.date > 0) ? info.date : QDateTime::currentSecsSinceEpoch());

    if (!query.exec()) {
        _lastError = query.lastError().text();
        return false;
    }

    bool ok = false;
    setID = query.lastInsertId().toULongLong(&ok);
    return ok;
}

bool QGCTileCacheDatabase::getTileSets(QList<TileSetInfo> &sets)
{
    if (!isOpen()) {
        _lastError = u"Database not open"_s;
        return false;
    }

    const QSqlDatabase db = _getDatabase();
    QSqlQuery query(db);

    if (!query.exec(u"SELECT * FROM TileSets ORDER BY defaultSet DESC, name ASC"_s)) {
        _lastError = query.lastError().text();
        return false;
    }

    while (query.next()) {
        bool ok = false;

        TileSetInfo info{};

        info.setID = query.value(u"setID"_s).toULongLong(&ok);
        if (!ok) {
            qCWarning(QGCTileCacheDatabaseLog) << "Invalid setID value";
            continue;
        }

        info.name = query.value(u"name"_s).toString();
        info.typeStr = query.value(u"typeStr"_s).toString();

        info.topleftLat = query.value(u"topleftLat"_s).toDouble(&ok);
        if (!ok) {
            qCWarning(QGCTileCacheDatabaseLog) << "Invalid topleftLat value";
        }

        info.topleftLon = query.value(u"topleftLon"_s).toDouble(&ok);
        if (!ok) {
            qCWarning(QGCTileCacheDatabaseLog) << "Invalid topleftLon value";
        }

        info.bottomRightLat = query.value(u"bottomRightLat"_s).toDouble(&ok);
        if (!ok) {
            qCWarning(QGCTileCacheDatabaseLog) << "Invalid bottomRightLat value";
        }

        info.bottomRightLon = query.value(u"bottomRightLon"_s).toDouble(&ok);
        if (!ok) {
            qCWarning(QGCTileCacheDatabaseLog) << "Invalid bottomRightLon value";
        }

        info.minZoom = query.value(u"minZoom"_s).toInt(&ok);
        if (!ok) {
            qCWarning(QGCTileCacheDatabaseLog) << "Invalid minZoom value";
        }

        info.maxZoom = query.value(u"maxZoom"_s).toInt(&ok);
        if (!ok) {
            qCWarning(QGCTileCacheDatabaseLog) << "Invalid maxZoom value";
        }

        info.type = query.value(u"type"_s).toInt(&ok);
        if (!ok) {
            qCWarning(QGCTileCacheDatabaseLog) << "Invalid type value";
        }

        info.numTiles = query.value(u"numTiles"_s).toUInt(&ok);
        if (!ok) {
            qCWarning(QGCTileCacheDatabaseLog) << "Invalid numTiles value";
        }

        info.defaultSet = (query.value(u"defaultSet"_s).toInt(&ok) != 0);
        if (!ok) {
            qCWarning(QGCTileCacheDatabaseLog) << "Invalid defaultSet value";
        }

        info.date = query.value(u"date"_s).toLongLong(&ok);
        if (!ok) {
            qCWarning(QGCTileCacheDatabaseLog) << "Invalid date value";
        }

        sets.append(info);
    }

    return true;
}

bool QGCTileCacheDatabase::getTileSet(quint64 setID, TileSetInfo &info)
{
    if (!isOpen()) {
        _lastError = u"Database not open"_s;
        return false;
    }

    const QSqlDatabase db = _getDatabase();
    QSqlQuery query(db);

    if (!query.prepare(u"SELECT * FROM TileSets WHERE setID = ?"_s)) {
        _lastError = query.lastError().text();
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to prepare getTileSet query:" << _lastError;
        return false;
    }

    query.addBindValue(setID);

    if (!query.exec() || !query.next()) {
        return false;
    }

    bool ok = false;

    info.setID = query.value(u"setID"_s).toULongLong(&ok);
    if (!ok) {
        qCWarning(QGCTileCacheDatabaseLog) << "Invalid setID value";
        return false;
    }

    info.name = query.value(u"name"_s).toString();
    info.typeStr = query.value(u"typeStr"_s).toString();

    info.topleftLat = query.value(u"topleftLat"_s).toDouble(&ok);
    if (!ok) {
        qCWarning(QGCTileCacheDatabaseLog) << "Invalid topleftLat value";
    }

    info.topleftLon = query.value(u"topleftLon"_s).toDouble(&ok);
    if (!ok) {
        qCWarning(QGCTileCacheDatabaseLog) << "Invalid topleftLon value";
    }

    info.bottomRightLat = query.value(u"bottomRightLat"_s).toDouble(&ok);
    if (!ok) {
        qCWarning(QGCTileCacheDatabaseLog) << "Invalid bottomRightLat value";
    }

    info.bottomRightLon = query.value(u"bottomRightLon"_s).toDouble(&ok);
    if (!ok) {
        qCWarning(QGCTileCacheDatabaseLog) << "Invalid bottomRightLon value";
    }

    info.minZoom = query.value(u"minZoom"_s).toInt(&ok);
    if (!ok) {
        qCWarning(QGCTileCacheDatabaseLog) << "Invalid minZoom value";
    }

    info.maxZoom = query.value(u"maxZoom"_s).toInt(&ok);
    if (!ok) {
        qCWarning(QGCTileCacheDatabaseLog) << "Invalid maxZoom value";
    }

    info.type = query.value(u"type"_s).toInt(&ok);
    if (!ok) {
        qCWarning(QGCTileCacheDatabaseLog) << "Invalid type value";
    }

    info.numTiles = query.value(u"numTiles"_s).toUInt(&ok);
    if (!ok) {
        qCWarning(QGCTileCacheDatabaseLog) << "Invalid numTiles value";
    }

    info.defaultSet = (query.value(u"defaultSet"_s).toInt(&ok) != 0);
    if (!ok) {
        qCWarning(QGCTileCacheDatabaseLog) << "Invalid defaultSet value";
    }

    info.date = query.value(u"date"_s).toLongLong(&ok);
    if (!ok) {
        qCWarning(QGCTileCacheDatabaseLog) << "Invalid date value";
    }

    return true;
}

bool QGCTileCacheDatabase::updateTileSet(quint64 setID, const TileSetInfo &info)
{
    if (!isOpen()) {
        _lastError = u"Database not open"_s;
        return false;
    }

    const QSqlDatabase db = _getDatabase();
    QSqlQuery query(db);

    if (!query.prepare(
        u"UPDATE TileSets SET "
        "name=?, typeStr=?, topleftLat=?, topleftLon=?, bottomRightLat=?, bottomRightLon=?, "
        "minZoom=?, maxZoom=?, type=?, numTiles=? WHERE setID=?"_s)) {
        _lastError = query.lastError().text();
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to prepare updateTileSet query:" << _lastError;
        return false;
    }

    query.addBindValue(info.name);
    query.addBindValue(info.typeStr);
    query.addBindValue(info.topleftLat);
    query.addBindValue(info.topleftLon);
    query.addBindValue(info.bottomRightLat);
    query.addBindValue(info.bottomRightLon);
    query.addBindValue(info.minZoom);
    query.addBindValue(info.maxZoom);
    query.addBindValue(info.type);
    query.addBindValue(info.numTiles);
    query.addBindValue(setID);

    if (!query.exec()) {
        _lastError = query.lastError().text();
        return false;
    }

    return true;
}

bool QGCTileCacheDatabase::deleteTileSet(quint64 setID)
{
    if (!isOpen()) {
        _lastError = u"Database not open"_s;
        return false;
    }

    TransactionGuard transaction(*this);
    if (!transaction.isActive()) {
        return false;
    }

    const QSqlDatabase db = _getDatabase();
    QSqlQuery query(db);

    // Delete tiles unique to this set
    if (!query.prepare(uR"(
        DELETE FROM Tiles
        WHERE tileID IN (
            SELECT a.tileID FROM SetTiles a
            WHERE setID = ?
                AND NOT EXISTS (
                    SELECT 1 FROM SetTiles b
                    WHERE b.tileID = a.tileID AND b.setID <> a.setID
                )
        )
    )"_s)) {
        _lastError = query.lastError().text();
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to prepare delete unique tiles query:" << _lastError;
    } else {
        query.addBindValue(setID);
        (void) query.exec();
    }

    // Delete from download queue
    if (!query.prepare(u"DELETE FROM TilesDownload WHERE setID = ?"_s)) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to prepare delete download queue query:" << query.lastError().text();
    } else {
        query.addBindValue(setID);
        (void) query.exec();
    }

    // Delete the tile set
    if (!query.prepare(u"DELETE FROM TileSets WHERE setID = ?"_s)) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to prepare delete tileset query:" << query.lastError().text();
    } else {
        query.addBindValue(setID);
        (void) query.exec();
    }

    // Delete set-tile associations
    if (!query.prepare(u"DELETE FROM SetTiles WHERE setID = ?"_s)) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to prepare delete set tiles query:" << query.lastError().text();
    } else {
        query.addBindValue(setID);
        (void) query.exec();
    }

    transaction.commit();
    return true;
}

bool QGCTileCacheDatabase::renameTileSet(quint64 setID, const QString &newName)
{
    if (!isOpen()) {
        _lastError = u"Database not open"_s;
        return false;
    }

    const QSqlDatabase db = _getDatabase();
    QSqlQuery query(db);

    if (!query.prepare(u"UPDATE TileSets SET name = ? WHERE setID = ?"_s)) {
        _lastError = query.lastError().text();
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to prepare renameTileSet query:" << _lastError;
        return false;
    }

    query.addBindValue(newName);
    query.addBindValue(setID);

    if (!query.exec()) {
        _lastError = query.lastError().text();
        return false;
    }

    return true;
}

bool QGCTileCacheDatabase::findTileSetID(const QString &name, quint64 &setID)
{
    if (!isOpen()) {
        return false;
    }

    const QSqlDatabase db = _getDatabase();
    QSqlQuery query(db);

    if (!query.prepare(u"SELECT setID FROM TileSets WHERE name = ?"_s)) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to prepare findTileSetID query:" << query.lastError().text();
        return false;
    }

    query.addBindValue(name);

    if (query.exec() && query.next()) {
        bool ok = false;
        setID = query.value(0).toULongLong(&ok);
        return ok;
    }

    return false;
}

quint64 QGCTileCacheDatabase::getDefaultTileSet() const
{
    // Check cache first with minimal lock time
    {
        QMutexLocker lock(&_cacheMutex);
        if (_defaultSetCache != UINT64_MAX) {
            return _defaultSetCache;
        }
    }

    // Cache miss - query database without holding cache mutex
    if (!isOpen()) {
        return 1;
    }

    const QSqlDatabase db = _getDatabase();
    QSqlQuery query(db);

    if (query.exec(u"SELECT setID FROM TileSets WHERE defaultSet = 1 LIMIT 1"_s) && query.next()) {
        bool ok = false;
        const quint64 setID = query.value(0).toULongLong(&ok);
        if (ok) {
            // Update cache atomically
            QMutexLocker lock(&_cacheMutex);
            // Double-check pattern: another thread might have already updated the cache
            if (_defaultSetCache == UINT64_MAX) {
                _defaultSetCache = setID;
            }
            return _defaultSetCache;
        }

        qCWarning(QGCTileCacheDatabaseLog) << "Invalid setID value";
    }

    return 1;
}

/*---------------------------------------------------------------------------*/
// Set Tile Associations

bool QGCTileCacheDatabase::addTileToSet(quint64 tileID, quint64 setID)
{
    if (!isOpen()) {
        _lastError = u"Database not open"_s;
        return false;
    }

    const QSqlDatabase db = _getDatabase();
    QSqlQuery query(db);

    if (!query.prepare(u"INSERT OR IGNORE INTO SetTiles(tileID, setID) VALUES(?, ?)"_s)) {
        _lastError = query.lastError().text();
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to prepare addTileToSet query:" << _lastError;
        return false;
    }

    query.addBindValue(tileID);
    query.addBindValue(setID);

    if (!query.exec()) {
        _lastError = query.lastError().text();
        return false;
    }

    return true;
}

bool QGCTileCacheDatabase::removeTileFromSet(quint64 tileID, quint64 setID)
{
    if (!isOpen()) {
        _lastError = u"Database not open"_s;
        return false;
    }

    const QSqlDatabase db = _getDatabase();
    QSqlQuery query(db);

    if (!query.prepare(u"DELETE FROM SetTiles WHERE tileID = ? AND setID = ?"_s)) {
        _lastError = query.lastError().text();
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to prepare removeTileFromSet query:" << _lastError;
        return false;
    }

    query.addBindValue(tileID);
    query.addBindValue(setID);

    if (!query.exec()) {
        _lastError = query.lastError().text();
        return false;
    }

    return true;
}

bool QGCTileCacheDatabase::getTilesForSet(quint64 setID, QList<quint64> &tileIDs)
{
    if (!isOpen()) {
        _lastError = u"Database not open"_s;
        return false;
    }

    const QSqlDatabase db = _getDatabase();
    QSqlQuery query(db);

    if (!query.prepare(u"SELECT tileID FROM SetTiles WHERE setID = ?"_s)) {
        _lastError = query.lastError().text();
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to prepare getTilesForSet query:" << _lastError;
        return false;
    }

    query.addBindValue(setID);

    if (!query.exec()) {
        _lastError = query.lastError().text();
        return false;
    }

    while (query.next()) {
        bool ok = false;
        const quint64 id = query.value(0).toULongLong(&ok);
        if (ok) {
            tileIDs.append(id);
        } else {
            qCWarning(QGCTileCacheDatabaseLog) << "Invalid tileID";
        }
    }

    return true;
}

bool QGCTileCacheDatabase::isTileInSet(quint64 tileID, quint64 setID) const
{
    if (!isOpen()) {
        return false;
    }

    const QSqlDatabase db = _getDatabase();
    QSqlQuery query(db);

    if (!query.prepare(u"SELECT 1 FROM SetTiles WHERE tileID = ? AND setID = ? LIMIT 1"_s)) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to prepare isTileInSet query:" << query.lastError().text();
        return false;
    }

    query.addBindValue(tileID);
    query.addBindValue(setID);

    return (query.exec() && query.next());
}

/*---------------------------------------------------------------------------*/
// Download Queue Operations

bool QGCTileCacheDatabase::addToDownloadQueue(quint64 setID, const TileDownloadInfo &info)
{
    if (!isOpen()) {
        _lastError = u"Database not open"_s;
        return false;
    }

    const QSqlDatabase db = _getDatabase();
    QSqlQuery query(db);

    if (!query.prepare(
        u"INSERT OR IGNORE INTO TilesDownload(setID, hash, type, x, y, z, state) "
        "VALUES(?, ?, ?, ?, ?, ?, ?)"_s)) {
        _lastError = query.lastError().text();
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to prepare addToDownloadQueue query:" << _lastError;
        return false;
    }

    query.addBindValue(setID);
    query.addBindValue(info.hash);
    query.addBindValue(info.type);
    query.addBindValue(info.x);
    query.addBindValue(info.y);
    query.addBindValue(info.z);
    query.addBindValue(info.state);

    if (!query.exec()) {
        _lastError = query.lastError().text();
        return false;
    }

    return true;
}

bool QGCTileCacheDatabase::addBatchToDownloadQueue(quint64 setID, const QList<TileDownloadInfo> &tiles)
{
    if (!isOpen() || tiles.isEmpty()) {
        return false;
    }

    TransactionGuard transaction(*this);
    if (!transaction.isActive()) {
        return false;
    }

    const QSqlDatabase db = _getDatabase();
    QSqlQuery query(db);

    if (!query.prepare(
        u"INSERT OR IGNORE INTO TilesDownload(setID, hash, type, x, y, z, state) "
        "VALUES(?, ?, ?, ?, ?, ?, ?)"_s)) {
        _lastError = query.lastError().text();
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to prepare addBatchToDownloadQueue query:" << _lastError;
        return false;
    }

    for (const TileDownloadInfo &info : tiles) {
        query.addBindValue(setID);
        query.addBindValue(info.hash);
        query.addBindValue(info.type);
        query.addBindValue(info.x);
        query.addBindValue(info.y);
        query.addBindValue(info.z);
        query.addBindValue(info.state);

        if (!query.exec()) {
            _lastError = query.lastError().text();
            return false;
        }
    }

    transaction.commit();
    return true;
}

bool QGCTileCacheDatabase::getDownloadList(quint64 setID, int limit, QList<TileDownloadInfo> &tiles)
{
    if (!isOpen()) {
        _lastError = u"Database not open"_s;
        return false;
    }

    const QSqlDatabase db = _getDatabase();
    QSqlQuery query(db);

    if (!query.prepare(u"SELECT hash, type, x, y, z "
                       "FROM TilesDownload WHERE setID = ? AND state = 0 "
                       "ORDER BY lastAttempt ASC, rowid ASC LIMIT ?"_s)) {
        _lastError = query.lastError().text();
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to prepare getDownloadList query:" << _lastError;
        return false;
    }

    query.addBindValue(setID);
    query.addBindValue(limit);

    if (!query.exec()) {
        _lastError = query.lastError().text();
        return false;
    }

    while (query.next()) {
        bool ok = false;

        TileDownloadInfo info{};
        info.hash = query.value(u"hash"_s).toString();

        info.type = query.value(u"type"_s).toInt(&ok);
        if (!ok) {
            info.type = 0;
            qCWarning(QGCTileCacheDatabaseLog) << "Invalid type value";
        }

        info.x = query.value(u"x"_s).toInt(&ok);
        if (!ok) {
            info.x = 0;
            qCWarning(QGCTileCacheDatabaseLog) << "Invalid x value";
        }

        info.y = query.value(u"y"_s).toInt(&ok);
        if (!ok) {
            info.y = 0;
            qCWarning(QGCTileCacheDatabaseLog) << "Invalid y value";
        }

        info.z = query.value(u"z"_s).toInt(&ok);
        if (!ok) {
            info.z = 0;
            qCWarning(QGCTileCacheDatabaseLog) << "Invalid z value";
        }

        tiles.append(info);
    }

    return true;
}

bool QGCTileCacheDatabase::updateDownloadState(quint64 setID, const QString &hash, int state)
{
    if (!isOpen()) {
        _lastError = u"Database not open"_s;
        return false;
    }

    const QSqlDatabase db = _getDatabase();
    QSqlQuery query(db);

    bool prepared = false;
    if (state == static_cast<int>(QGCTile::StateComplete)) {
        prepared = query.prepare(u"DELETE FROM TilesDownload WHERE setID = ? AND hash = ?"_s);
        if (prepared) {
            query.addBindValue(setID);
            query.addBindValue(hash);
        }
    } else if (hash == "*") {
        prepared = query.prepare(u"UPDATE TilesDownload SET state = ? WHERE setID = ?"_s);
        if (prepared) {
            query.addBindValue(state);
            query.addBindValue(setID);
        }
    } else {
        prepared = query.prepare(
            u"UPDATE TilesDownload SET state = ?, retryCount = retryCount + 1, "
            "lastAttempt = strftime('%s', 'now') WHERE setID = ? AND hash = ?"_s);
        if (prepared) {
            query.addBindValue(state);
            query.addBindValue(setID);
            query.addBindValue(hash);
        }
    }

    if (!prepared) {
        _lastError = query.lastError().text();
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to prepare updateDownloadState query:" << _lastError;
        return false;
    }

    if (!query.exec()) {
        _lastError = query.lastError().text();
        return false;
    }

    return true;
}

bool QGCTileCacheDatabase::clearDownloadQueue(quint64 setID)
{
    if (!isOpen()) {
        _lastError = u"Database not open"_s;
        return false;
    }

    const QSqlDatabase db = _getDatabase();
    QSqlQuery query(db);

    if (!query.prepare(u"DELETE FROM TilesDownload WHERE setID = ?"_s)) {
        _lastError = query.lastError().text();
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to prepare clearDownloadQueue query:" << _lastError;
        return false;
    }

    query.addBindValue(setID);

    if (!query.exec()) {
        _lastError = query.lastError().text();
        return false;
    }

    return true;
}

int QGCTileCacheDatabase::getDownloadQueueCount(quint64 setID) const
{
    if (!isOpen()) {
        return 0;
    }

    const QSqlDatabase db = _getDatabase();
    QSqlQuery query(db);

    if (!query.prepare(u"SELECT COUNT(*) FROM TilesDownload WHERE setID = ? AND state = 0"_s)) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to prepare getDownloadQueueCount query:" << query.lastError().text();
        return 0;
    }

    query.addBindValue(setID);

    if (query.exec() && query.next()) {
        bool ok = false;
        const int count = query.value(0).toInt(&ok);
        if (ok) {
            return count;
        }
        qCWarning(QGCTileCacheDatabaseLog) << "Invalid count for setID:" << setID;
    }

    return 0;
}

/*---------------------------------------------------------------------------*/
// Statistics Operations

bool QGCTileCacheDatabase::getTotals(DatabaseTotals &totals)
{
    if (!isOpen()) {
        _lastError = u"Database not open"_s;
        return false;
    }

    const QSqlDatabase db = _getDatabase();
    QSqlQuery query(db);

    // Get total counts with COALESCE for NULL handling
    if (query.exec(u"SELECT COUNT(*), COALESCE(SUM(size), 0) FROM Tiles"_s) && query.next()) {
        bool ok = false;
        totals.totalCount = query.value(0).toUInt(&ok);
        if (!ok) {
            totals.totalCount = 0;
            qCWarning(QGCTileCacheDatabaseLog) << "Invalid totalCount value";
        }

        totals.totalSize = query.value(1).toULongLong(&ok);
        if (!ok) {
            totals.totalSize = 0;
            qCWarning(QGCTileCacheDatabaseLog) << "Invalid totalSize value";
        }
    }

    // Get default set unique counts
    const quint64 defaultSet = getDefaultTileSet();

    if (!query.prepare(uR"(
        SELECT COUNT(*), COALESCE(SUM(size), 0)
        FROM Tiles
        WHERE tileID IN (
            SELECT tileID FROM SetTiles a
            WHERE setID = ?
                AND NOT EXISTS (
                    SELECT 1 FROM SetTiles b
                    WHERE b.tileID = a.tileID AND b.setID <> a.setID
                )
        )
    )"_s)) {
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to prepare getTotals (default set) query:" << query.lastError().text();
        return true;  // Return true with partial data
    }

    query.addBindValue(defaultSet);

    if (query.exec() && query.next()) {
        bool ok = false;
        totals.defaultCount = query.value(0).toUInt(&ok);
        if (!ok) {
            totals.defaultCount = 0;
            qCWarning(QGCTileCacheDatabaseLog) << "Invalid defaultCount value";
        }

        totals.defaultSize = query.value(1).toULongLong(&ok);
        if (!ok) {
            totals.defaultSize = 0;
            qCWarning(QGCTileCacheDatabaseLog) << "Invalid defaultSize value";
        }
    }

    return true;
}

bool QGCTileCacheDatabase::getSetStatistics(quint64 setID, DatabaseStatistics &stats)
{
    return getSetStatisticsOptimized(setID, stats.savedCount, stats.savedSize, stats.uniqueCount, stats.uniqueSize);
}

bool QGCTileCacheDatabase::getSetStatisticsOptimized(quint64 setID, quint32 &savedCount, quint64 &savedSize, quint32 &uniqueCount, quint64 &uniqueSize)
{
    if (!isOpen()) {
        _lastError = u"Database not open"_s;
        return false;
    }

    const QSqlDatabase db = _getDatabase();
    QSqlQuery query(db);

    // Single optimized query using CTEs
    if (!query.prepare(uR"(
        SELECT
            (SELECT COUNT(*) FROM SetTiles ST JOIN Tiles T ON ST.tileID = T.tileID WHERE ST.setID = ?) AS savedCount,
            (SELECT COALESCE(SUM(size), 0) FROM SetTiles ST JOIN Tiles T ON ST.tileID = T.tileID WHERE ST.setID = ?) AS savedSize,
            (SELECT COUNT(*) FROM Tiles WHERE tileID IN (
                SELECT a.tileID FROM SetTiles a
                WHERE a.setID = ?
                    AND NOT EXISTS (
                        SELECT 1 FROM SetTiles b
                        WHERE b.tileID = a.tileID AND b.setID <> a.setID
                    )
            )) AS uniqueCount,
            (SELECT COALESCE(SUM(size), 0) FROM Tiles WHERE tileID IN (
                SELECT a.tileID FROM SetTiles a
                WHERE a.setID = ?
                    AND NOT EXISTS (
                        SELECT 1 FROM SetTiles b
                        WHERE b.tileID = a.tileID AND b.setID <> a.setID
                    )
            )) AS uniqueSize
    )"_s)) {
        _lastError = query.lastError().text();
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to prepare getSetStatisticsOptimized query:" << _lastError;
        return false;
    }

    query.addBindValue(setID);
    query.addBindValue(setID);
    query.addBindValue(setID);
    query.addBindValue(setID);

    if (!query.exec()) {
        _lastError = query.lastError().text();
        return false;
    }

    if (!query.next()) {
        return false;
    }

    bool ok = false;
    savedCount = query.value(0).toUInt(&ok);
    if (!ok) {
        savedCount = 0;
        qCWarning(QGCTileCacheDatabaseLog) << "Invalid savedCount value";
    }

    savedSize = query.value(1).toULongLong(&ok);
    if (!ok) {
        savedSize = 0;
        qCWarning(QGCTileCacheDatabaseLog) << "Invalid savedSize value";
    }

    uniqueCount = query.value(2).toUInt(&ok);
    if (!ok) {
        uniqueCount = 0;
        qCWarning(QGCTileCacheDatabaseLog) << "Invalid uniqueCount value";
    }

    uniqueSize = query.value(3).toULongLong(&ok);
    if (!ok) {
        uniqueSize = 0;
        qCWarning(QGCTileCacheDatabaseLog) << "Invalid uniqueSize value";
    }

    return true;
}

/*---------------------------------------------------------------------------*/
// Cache Management

bool QGCTileCacheDatabase::pruneCache(quint64 setID, qint64 amount, qint64 &pruned)
{
    if (!isOpen()) {
        _lastError = u"Database not open"_s;
        return false;
    }

    TransactionGuard transaction(*this);
    if (!transaction.isActive()) {
        return false;
    }

    const QSqlDatabase db = _getDatabase();
    QSqlQuery query(db);

    // Select least recently used tiles in set
    if (!query.prepare(
        u"SELECT tileID, size FROM Tiles WHERE tileID IN ("
        "SELECT tileID FROM SetTiles WHERE setID = ? "
        "GROUP BY tileID HAVING COUNT(*) = 1) "
        "ORDER BY lastAccess ASC, accessCount ASC "
        "LIMIT 1000"_s)) {
        _lastError = query.lastError().text();
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to prepare pruneCache query:" << _lastError;
        return false;
    }

    query.addBindValue(setID);

    if (!query.exec()) {
        _lastError = query.lastError().text();
        return false;
    }

    QList<quint64> tilesToDelete;
    pruned = 0;

    while (query.next() && (amount > 0)) {
        bool ok = false;
        const quint64 tileID = query.value(0).toULongLong(&ok);
        if (!ok) {
            qCWarning(QGCTileCacheDatabaseLog) << "Invalid tileID value";
            continue;
        }

        const qint64 size = query.value(1).toLongLong(&ok);
        if (!ok) {
            qCWarning(QGCTileCacheDatabaseLog) << "Invalid size value";
            continue;
        }

        tilesToDelete.append(tileID);
        amount -= size;
        pruned += size;
    }

    // Delete selected tiles
    QSqlQuery deleteQuery(db);
    (void) deleteQuery.prepare(u"DELETE FROM Tiles WHERE tileID = ?"_s);

    for (const quint64 tileID : std::as_const(tilesToDelete)) {
        deleteQuery.addBindValue(tileID);
        if (!deleteQuery.exec()) {
            _lastError = deleteQuery.lastError().text();
            return false;
        }
    }

    transaction.commit();
    return true;
}

bool QGCTileCacheDatabase::pruneLRU(qint64 amount, qint64 &pruned)
{
    if (!isOpen()) {
        _lastError = u"Database not open"_s;
        return false;
    }

    TransactionGuard transaction(*this);
    if (!transaction.isActive()) {
        _lastError = u"Transaction not active"_s;
        return false;
    }

    const QSqlDatabase db = _getDatabase();
    QSqlQuery query(db);

    // Delete least recently used tiles across all sets
    if (!query.prepare(
        u"DELETE FROM Tiles WHERE tileID IN ("
        "SELECT tileID FROM Tiles "
        "ORDER BY lastAccess ASC, accessCount ASC "
        "LIMIT ?)"_s)) {
        _lastError = query.lastError().text();
        qCWarning(QGCTileCacheDatabaseLog) << "Failed to prepare pruneLRU query:" << _lastError;
        return false;
    }

    const int tilesToDelete = qMax(1, static_cast<int>(amount / kTileSize));
    query.addBindValue(tilesToDelete);

    if (!query.exec()) {
        _lastError = query.lastError().text();
        return false;
    }

    pruned = query.numRowsAffected() * kTileSize;

    transaction.commit();
    return true;
}

qint64 QGCTileCacheDatabase::getDatabaseSize() const
{
    const QFileInfo dbInfo(_databasePath);
    return dbInfo.size();
}

/*---------------------------------------------------------------------------*/
// Import/Export Operations

bool QGCTileCacheDatabase::exportDatabase(const QString &targetPath, const QList<quint64> &setIDs, std::function<void(int)> progressCallback)
{
    if (!isOpen()) {
        _lastError = u"Database not open"_s;
        return false;
    }

    // Delete target if it exists
    if (QFile::exists(targetPath)) {
        (void) QFile::remove(targetPath);
    }

    const QString exportConn = u"QGCTileExport_%1"_s.arg(QUuid::createUuid().toString(QUuid::Id128));
    QGCTileCacheDatabase exportDb(exportConn, targetPath);

    // Create exported database
    if (!exportDb.open()) {
        _lastError = u"Failed to create export database"_s;
        return false;
    }

    // Create schema in export database
    if (!exportDb.createSchema(false)) {
        _lastError = u"Failed to create export database schema"_s;
        exportDb.close();
        return false;
    }

    // Calculate total tiles for progress
    quint64 totalTiles = 0;
    quint64 currentProgress = 0;

    for (const quint64 setID : setIDs) {
        TileSetInfo setInfo{};
        if (!getTileSet(setID, setInfo)) {
            continue;
        }

        if (setInfo.defaultSet) {
            totalTiles += setInfo.numTiles;
            continue;
        }

        // Get unique tile count for non-default sets
        DatabaseStatistics stats{};
        if (getSetStatistics(setID, stats)) {
            totalTiles += stats.uniqueCount;
        }
    }

    if (totalTiles == 0) {
        totalTiles = 1;
    }

    // Export each tile set
    for (const quint64 setID : setIDs) {
        TileSetInfo setInfo{};
        if (!getTileSet(setID, setInfo)) {
            continue;
        }

        // Create tile set in export database
        quint64 exportSetID;
        if (!exportDb.createTileSet(setInfo, exportSetID)) {
            qCWarning(QGCTileCacheDatabaseLog) << "Failed to create tile set in export database";
            continue;
        }

        // Get all tiles for this set
        QList<quint64> tileIDs;
        if (!getTilesForSet(setID, tileIDs)) {
            continue;
        }

        // Begin transaction for bulk insert
        TransactionGuard transaction(exportDb);
        if (!transaction.isActive()) {
            continue;
        }

        for (const quint64 tileID : std::as_const(tileIDs)) {
            const QSqlDatabase db = _getDatabase();
            QSqlQuery query(db);

            if (!query.prepare(u"SELECT * FROM Tiles WHERE tileID = ?"_s)) {
                qCWarning(QGCTileCacheDatabaseLog) << "Failed to prepare exportDatabase tile query:" << query.lastError().text();
                continue;
            }

            query.addBindValue(tileID);
            if (!query.exec() || !query.next()) {
                continue;
            }

            bool ok = false;

            TileInfo tile{};
            tile.hash = query.value(u"hash"_s).toString();
            tile.format = query.value(u"format"_s).toString();
            tile.tile = query.value(u"tile"_s).toByteArray();
            tile.size = query.value(u"size"_s).toLongLong(&ok);
            if (!ok) {
                qCWarning(QGCTileCacheDatabaseLog) << "Invalid size value";
            }

            tile.type = query.value(u"type"_s).toInt(&ok);
            if (!ok) {
                qCWarning(QGCTileCacheDatabaseLog) << "Invalid type value";
                tile.type = -1;
            }

            tile.date = query.value(u"date"_s).toLongLong(&ok);
            if (!ok) {
                qCWarning(QGCTileCacheDatabaseLog) << "Invalid date value";
            }

            (void) exportDb.saveTile(tile, exportSetID);

            currentProgress++;
            // Safe progress calculation with bounds checking
            if (totalTiles > 0) {
                const double ratio = static_cast<double>(currentProgress) / static_cast<double>(totalTiles);
                const int progress = qBound(0, static_cast<int>(ratio * 100.0), 100);
                _safeProgressCallback(progressCallback, progress);
            }
        }

        transaction.commit();
    }

    exportDb.close();
    return true;
}

bool QGCTileCacheDatabase::_importDatabaseReplace(const QString &sourcePath, std::function<void(int)> progressCallback)
{
    DBG_DB_THREAD();

    // WARNING: This function closes and reopens the database.
    // The caller MUST ensure no other operations are in progress.
    // This should only be called from the worker thread.

    const QString currentPath = _databasePath;

    // Save state for restoration if import fails
    const bool wasOpen = isOpen();

    // Close database to allow file operations
    close();

    // Delete existing database
    if (QFile::exists(currentPath)) {
        if (!QFile::remove(currentPath)) {
            _lastError = u"Failed to remove existing database"_s;
            // Try to reopen original database
            if (wasOpen) {
                (void) open();
            }
            return false;
        }
    }

    // Copy import database to our path
    if (!QFile::copy(sourcePath, currentPath)) {
        _lastError = u"Failed to copy import database"_s;
        // Try to reopen original database (may fail since we deleted it)
        if (wasOpen) {
            (void) open();
        }
        return false;
    }

    _safeProgressCallback(progressCallback, 25);

    // Reopen the database
    if (!open()) {
        _lastError = u"Failed to open imported database"_s;
        return false;
    }

    _safeProgressCallback(progressCallback, 50);

    // Validate and potentially recreate schema
    if (!_validateDatabase()) {
        qCWarning(QGCTileCacheDatabaseLog) << "Imported database schema invalid, recreating";
        if (!createSchema(true)) {
            _lastError = u"Failed to create database schema"_s;
            return false;
        }
    }

    _safeProgressCallback(progressCallback, 100);
    return true;
}

bool QGCTileCacheDatabase::_importDatabaseMerge(const QString &sourcePath, std::function<void(int)> progressCallback)
{
    // Create imported database
    const QString importConn = u"QGCTileImport_%1"_s.arg(QUuid::createUuid().toString(QUuid::Id128));
    QGCTileCacheDatabase importDb(importConn, sourcePath);

    if (!importDb.open()) {
        _lastError = u"Failed to open import database"_s;
        return false;
    }

    // Calculate total tiles for progress
    quint64 totalTiles = 0;
    const QSqlDatabase importDbConn = importDb._getDatabase();
    QSqlQuery countQuery(importDbConn);
    if (countQuery.exec(u"SELECT COUNT(tileID) FROM Tiles"_s) && countQuery.next()) {
        bool ok = false;
        totalTiles = countQuery.value(0).toULongLong(&ok);
        if (!ok) {
            qCWarning(QGCTileCacheDatabaseLog) << "Invalid totalTiles value";
        }
    }

    if (totalTiles == 0) {
        _lastError = u"No tiles in import database"_s;
        importDb.close();
        return false;
    }

    // Import tile sets
    QList<TileSetInfo> tileSets;
    if (!importDb.getTileSets(tileSets)) {
        _lastError = u"Failed to read tile sets from import database"_s;
        importDb.close();
        return false;
    }

    quint64 currentProgress = 0;

    for (TileSetInfo &setInfo : tileSets) {
        const quint64 importSetID = setInfo.setID;
        quint64 targetSetID = getDefaultTileSet();

        // For non-default sets, check if name exists and make unique if needed
        if (!setInfo.defaultSet) {
            quint64 existingID;
            if (findTileSetID(setInfo.name, existingID)) {
                int counter = 1;
                const QString baseName = setInfo.name;
                do {
                    setInfo.name = u"%1 %02d"_s.arg(baseName).arg(counter++, 2, 10, QChar('0'));
                } while (findTileSetID(setInfo.name, existingID) && (counter < 100));
            }

            // Create the tile set
            if (!createTileSet(setInfo, targetSetID)) {
                qCWarning(QGCTileCacheDatabaseLog) << "Failed to create tile set during import:" << setInfo.name;
                continue;
            }
        }

        // Import tiles for this set
        QList<quint64> tileIDs;
        if (!importDb.getTilesForSet(importSetID, tileIDs)) {
            continue;
        }

        TransactionGuard transaction(*this);
        if (!transaction.isActive()) {
            continue;
        }

        quint64 tilesImported = 0;
        for (const quint64 tileID : std::as_const(tileIDs)) {
            QSqlQuery tileQuery(importDbConn);

            if (!tileQuery.prepare(u"SELECT * FROM Tiles WHERE tileID = ?"_s)) {
                qCWarning(QGCTileCacheDatabaseLog) << "Failed to prepare importDatabase tile query:" << tileQuery.lastError().text();
                continue;
            }

            tileQuery.addBindValue(tileID);

            if (!tileQuery.exec() || !tileQuery.next()) {
                continue;
            }

            bool ok = false;

            TileInfo tile{};
            tile.hash = tileQuery.value(u"hash"_s).toString();
            tile.format = tileQuery.value(u"format"_s).toString();
            tile.tile = tileQuery.value(u"tile"_s).toByteArray();
            tile.size = tileQuery.value(u"size"_s).toLongLong(&ok);
            if (!ok) {
                qCWarning(QGCTileCacheDatabaseLog) << "Invalid size value";
            }

            tile.type = tileQuery.value(u"type"_s).toInt(&ok);
            if (!ok) {
                qCWarning(QGCTileCacheDatabaseLog) << "Invalid type value";
                tile.type = -1;
            }

            tile.date = tileQuery.value(u"date"_s).toLongLong(&ok);
            if (!ok) {
                qCWarning(QGCTileCacheDatabaseLog) << "Invalid date value";
            }

            if (saveTile(tile, targetSetID)) {
                tilesImported++;
            }

            currentProgress++;
            // Safe progress calculation with bounds checking
            if (totalTiles > 0) {
                const double ratio = static_cast<double>(currentProgress) / static_cast<double>(totalTiles);
                const int progress = qBound(0, static_cast<int>(ratio * 100.0), 100);
                _safeProgressCallback(progressCallback, progress);
            }
        }

        transaction.commit();

        // Update tile count if tiles were added
        if ((tilesImported > 0) && !setInfo.defaultSet) {
            DatabaseStatistics stats{};
            if (getSetStatistics(targetSetID, stats)) {
                TileSetInfo updateInfo = setInfo;
                updateInfo.numTiles = stats.savedCount;
                (void) updateTileSet(targetSetID, updateInfo);
            }
        }

        // Remove empty imported set
        if ((tilesImported == 0) && !setInfo.defaultSet) {
            (void) deleteTileSet(targetSetID);
        }
    }

    importDb.close();
    return true;
}

bool QGCTileCacheDatabase::importDatabase(const QString &sourcePath, bool replace, std::function<void(int)> progressCallback)
{
    if (!isOpen()) {
        _lastError = u"Database not open"_s;
        return false;
    }

    bool result = false;

    if (replace) {
        result = _importDatabaseReplace(sourcePath, progressCallback);
    } else {
        result = _importDatabaseMerge(sourcePath, progressCallback);
    }

    return result;
}

/*---------------------------------------------------------------------------*/
// Transaction Management

bool QGCTileCacheDatabase::beginTransaction()
{
    DBG_DB_THREAD();

    if (!isOpen()) {
        _lastError = u"Database not open"_s;
        return false;
    }

    // Prevent nested transactions - SQLite doesn't support them
    if (_transactionCount.load() > 0) {
        _lastError = u"Nested transactions not supported"_s;
        qCWarning(QGCTileCacheDatabaseLog) << "Attempted nested transaction";
        return false;
    }

    QSqlDatabase db = _getDatabase();
    if (db.transaction()) {
        _transactionCount++;
        return true;
    }

    _lastError = db.lastError().text();
    return false;
}

bool QGCTileCacheDatabase::commitTransaction()
{
    if (!isOpen()) {
        _lastError = u"Database not open"_s;
        return false;
    }

    QSqlDatabase db = _getDatabase();
    if (db.commit()) {
        if (_transactionCount.load() > 0) {
            _transactionCount--;
        }
        return true;
    }

    _lastError = db.lastError().text();
    return false;
}

bool QGCTileCacheDatabase::rollbackTransaction()
{
    if (!isOpen()) {
        _lastError = u"Database not open"_s;
        return false;
    }

    QSqlDatabase db = _getDatabase();
    if (db.rollback()) {
        if (_transactionCount.load() > 0) {
            _transactionCount--;
        }
        return true;
    }

    _lastError = db.lastError().text();
    return false;
}

bool QGCTileCacheDatabase::inTransaction() const
{
    return (_transactionCount.load() > 0);
}

/*---------------------------------------------------------------------------*/
// Progress callback helper

void QGCTileCacheDatabase::_safeProgressCallback(std::function<void(int)> callback, int value)
{
    if (callback) {
        callback(qBound(0, value, 100));
    }
}

/*---------------------------------------------------------------------------*/
// Performance Metrics

QGCTileCacheDatabase::PerformanceMetrics QGCTileCacheDatabase::getMetrics() const
{
    QMutexLocker lock(&_metricsMutex);
    return _metrics;
}

void QGCTileCacheDatabase::resetMetrics()
{
    QMutexLocker lock(&_metricsMutex);
    _metrics = PerformanceMetrics();
}
