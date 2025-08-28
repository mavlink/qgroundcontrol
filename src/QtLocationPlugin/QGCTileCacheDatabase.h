/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <atomic>
#include <functional>

#include <QtCore/QByteArray>
#include <QtCore/QDateTime>
#include <QtCore/QLoggingCategory>
#include <QtCore/QMutex>
#include <QtCore/QObject>
#include <QtCore/QString>

Q_DECLARE_LOGGING_CATEGORY(QGCTileCacheDatabaseLog)

class QSqlDatabase;

/*===========================================================================*/

struct TileInfo
{
    quint64 tileID = 0;
    QString hash;
    QString format;
    QByteArray tile;
    qint64 size = 0;
    int type = -1;
    qint64 date = 0;
};

struct TileSetInfo
{
    quint64 setID = 0;
    QString name;
    QString typeStr;
    double topleftLat = 0.0;
    double topleftLon = 0.0;
    double bottomRightLat = 0.0;
    double bottomRightLon = 0.0;
    int minZoom = 3;
    int maxZoom = 3;
    int type = -1;
    quint32 numTiles = 0;
    bool defaultSet = false;
    qint64 date = 0;
};

struct TileDownloadInfo
{
    QString hash;
    int type = 0;
    int x = 0;
    int y = 0;
    int z = 0;
    int state = 0;
};

struct DatabaseTotals
{
    quint32 totalCount = 0;
    quint64 totalSize = 0;
    quint32 defaultCount = 0;
    quint64 defaultSize = 0;
};

struct DatabaseStatistics
{
    quint32 savedCount = 0;
    quint64 savedSize = 0;
    quint32 uniqueCount = 0;
    quint64 uniqueSize = 0;
};

/*===========================================================================*/

// RAII Transaction Guard
class TransactionGuard
{
public:
    explicit TransactionGuard(class QGCTileCacheDatabase &db);
    ~TransactionGuard();

    void commit();
    void rollback();
    bool isActive() const { return _active; }

private:
    class QGCTileCacheDatabase &_db;
    bool _committed = false;
    bool _active = false;
};

/*===========================================================================*/

class QGCTileCacheDatabase
{
    friend class TransactionGuard;

public:
    explicit QGCTileCacheDatabase(const QString &connectionName, const QString &databasePath);
    ~QGCTileCacheDatabase();

    /// Database creation and management
    bool open();
    void close();
    bool isOpen() const;
    bool createSchema(bool createDefault = true);
    bool reset();
    bool vacuum();
    bool analyze();
    QString lastError() const { return _lastError; };

    /// Tile operations
    bool saveTile(const TileInfo &tile, quint64 setID);
    bool saveTileBatch(const QList<TileInfo> &tiles, quint64 setID);
    bool getTile(const QString &hash, TileInfo &tile);
    quint64 getTileID(const QString &hash) const;
    bool deleteTile(quint64 tileID);
    bool deleteTilesMatchingBytes(const QByteArray &bytes);
    bool updateTileAccess(const QString &hash) const;

    /// Tile set operations
    bool createTileSet(const TileSetInfo &info, quint64 &setID);
    bool getTileSets(QList<TileSetInfo> &sets);
    bool getTileSet(quint64 setID, TileSetInfo &info);
    bool updateTileSet(quint64 setID, const TileSetInfo &info);
    bool deleteTileSet(quint64 setID);
    bool renameTileSet(quint64 setID, const QString &newName);
    bool findTileSetID(const QString &name, quint64 &setID);
    quint64 getDefaultTileSet() const;

    /// Set tile associations
    bool addTileToSet(quint64 tileID, quint64 setID);
    bool removeTileFromSet(quint64 tileID, quint64 setID);
    bool getTilesForSet(quint64 setID, QList<quint64> &tileIDs);
    bool isTileInSet(quint64 tileID, quint64 setID) const;

    /// Download queue operations
    bool addToDownloadQueue(quint64 setID, const TileDownloadInfo &info);
    bool addBatchToDownloadQueue(quint64 setID, const QList<TileDownloadInfo> &tiles);
    bool getDownloadList(quint64 setID, int limit, QList<TileDownloadInfo> &tiles);
    bool updateDownloadState(quint64 setID, const QString &hash, int state);
    bool clearDownloadQueue(quint64 setID);
    int getDownloadQueueCount(quint64 setID) const;

    /// Statistics and maintenance
    bool getTotals(DatabaseTotals &totals);
    bool getSetStatistics(quint64 setID, DatabaseStatistics &stats);
    bool getSetStatisticsOptimized(quint64 setID, quint32 &savedCount, quint64 &savedSize, quint32 &uniqueCount, quint64 &uniqueSize);
    bool pruneCache(quint64 setID, qint64 amount, qint64 &pruned);
    bool pruneLRU(qint64 amount, qint64 &pruned);
    qint64 getDatabaseSize() const;

    /// Import/Export
    bool exportDatabase(const QString &targetPath, const QList<quint64> &setIDs, std::function<void(int)> progressCallback = nullptr);
    bool importDatabase(const QString &sourcePath, bool replace, std::function<void(int)> progressCallback = nullptr);

    /// Transaction support
    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();
    bool inTransaction() const;

    // Performance metrics
    struct PerformanceMetrics {
        quint64 totalReads = 0;
        quint64 totalWrites = 0;
        quint64 cacheHits = 0;
        quint64 cacheMisses = 0;
        qint64 avgQueryTimeMs = 0;
        qint64 maxQueryTimeMs = 0;
    };
    PerformanceMetrics getMetrics() const;
    void resetMetrics();

private:
    /// Database Helpers
    QSqlDatabase _getDatabase() const;
    bool _createDatabase();
    bool _enableOptimizations() const;
    bool _createIndexes() const;
    bool _executeQuery(const QString &query) const;
    bool _validateDatabase() const;

    /// Import helpers
    bool _importDatabaseReplace(const QString &sourcePath, std::function<void(int)> progressCallback);
    bool _importDatabaseMerge(const QString &sourcePath, std::function<void(int)> progressCallback);

    /// Progress callback helper
    static void _safeProgressCallback(std::function<void(int)> callback, int value);

    /// Thread safety
    mutable QMutex _dbMutex;
    mutable QMutex _cacheMutex;
    mutable QMutex _metricsMutex;

    /// Database info
    QString _connectionName;
    QString _databasePath;
    QString _lastError;
    mutable std::atomic<int> _transactionCount{0};
    Qt::HANDLE _ownerThreadId = nullptr;

    /// Cache
    mutable quint64 _defaultSetCache = UINT64_MAX;

    /// Performance metrics
    mutable PerformanceMetrics _metrics;

    /// Constants
    static constexpr int kTileSize = 1024 * 50;  ///< Assume average tile size of 50KB
    static constexpr int kBatchSize = 500;
    static constexpr int kMaxRetries = 3;
    static constexpr int kBusyTimeout = 1000 * 5;
    static constexpr int kCacheSize = 1024 * 10;
};
