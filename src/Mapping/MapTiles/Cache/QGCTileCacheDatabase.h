#pragma once

#include <memory>
#include <optional>

#include <QtCore/QByteArray>
#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>

#include "QGCTile.h"
#include "QGCTileCacheTypes.h"

struct QGCCacheTile;
class QSqlDatabase;

class QGCTileCacheDatabase
{
public:
    static constexpr quint64 kInvalidTileSet = UINT64_MAX;
    /// 2: provider ids are the permanent ones (version 1 had the position in the provider list of the build)
    /// 3: the key of a tile ends with the cache variant of its provider (UrlFactory::getTileHash())
    static constexpr int kSchemaVersion = 3;

    explicit QGCTileCacheDatabase(const QString& databasePath);
    ~QGCTileCacheDatabase();

    bool init();
    bool connectDB();
    void disconnectDB();

    /// Replaces the provider ids a version-1 cache is renumbered from (UrlFactory::legacyMapIds()). Call before init().
    /// For unit tests: on the platforms they run on, version 1 had the permanent ids already.
    void setLegacyMapIds(const QMap<int, int>& legacyMapIds) { _legacyMapIds = legacyMapIds; }

    /// The cache variants by provider id (UrlFactory::cacheVariants()) that the tiles of a file from before schema
    /// version 3 get: of the cache when set before init(), of a file imported afterwards.
    void setCacheVariants(const QHash<int, QString>& cacheVariants) { _cacheVariants = cacheVariants; }

    bool isValid() const { return _valid; }

    bool hasFailed() const { return _failed; }

    // Tiles
    bool saveTile(const QString& hash, const QString& format, const QByteArray& img, const QString& type,
                  quint64 tileSet);
    std::unique_ptr<QGCCacheTile> getTile(const QString& hash);
    std::optional<quint64> findTile(const QString& hash);

    // Tile Sets
    QList<TileSetRecord> getTileSets();
    /// \a cacheVariant is that of the provider \a type, which the caller got on the main thread: this runs on the
    /// worker thread, where the provider must not read it from the settings.
    std::optional<quint64> createTileSet(const QString& name, const QString& mapTypeStr, double topleftLat,
                                         double topleftLon, double bottomRightLat, double bottomRightLon, int minZoom,
                                         int maxZoom, const QString& type, quint32 numTiles,
                                         const QString& cacheVariant);
    bool deleteTileSet(quint64 id);
    bool renameTileSet(quint64 setID, const QString& newName);
    std::optional<quint64> findTileSetID(const QString& name);
    bool resetDatabase();

    // Downloads
    QList<QGCTile> getTileDownloadList(quint64 setID, int count);
    bool updateTileDownloadState(quint64 setID, int state, const QString& hash);
    bool updateAllTileDownloadStates(quint64 setID, int state);

    // Cache
    bool pruneCache(quint64 amount);
    /// Gives back to the file system at most \a maxPages of the pages that deleted rows left unused, and returns how
    /// many are left. Only a file with incremental auto-vacuum can: any other file keeps its unused pages for new
    /// rows, and 0 is returned.
    qint64 releaseFreePages(int maxPages);

    // Stats
    TotalsResult computeTotals();
    SetTotalsResult computeSetTotals(quint64 setID, bool isDefault, quint32 totalTileCount, const QString& type);

    // Import/Export
    DatabaseResult importSetsReplace(const QString& path, ProgressCallback progressCb);
    DatabaseResult importSetsMerge(const QString& path, ProgressCallback progressCb);
    DatabaseResult exportSets(const QList<TileSetRecord>& sets, const QString& path, ProgressCallback progressCb);

    // Exposed for unit tests only
    QSqlDatabase database() const;

private:
    bool _ensureConnected() const;
    QSqlDatabase _database() const;
    bool _checkSchemaVersion();
    bool _migrate(QSqlDatabase db, int version);
    static bool _renumberMapIds(QSqlDatabase db, const QMap<int, int>& legacyMapIds);
    static bool _adoptCacheVariants(QSqlDatabase db, const QHash<int, QString>& cacheVariants);
    /// The schema version of the file at \a path when it can be imported. Otherwise \a errorString says why not.
    static std::optional<int> _importableVersion(const QString& path, QString& errorString);
    bool _createDB(QSqlDatabase db, bool createDefault = true);
    quint64 _getDefaultTileSet();
    bool _deleteTilesByIDs(const QList<quint64>& ids);
    QString _deduplicateSetName(const QString& name);
    quint64 _copyTilesForSet(QSqlDatabase srcDB, quint64 srcSetID, quint64 dstSetID, quint64& currentCount,
                             quint64 tileCount, int& lastProgress, ProgressCallback progressCb,
                             quint64* tilesIteratedOut);

    QString _databasePath;
    QString _connectionName;
    QMap<int, int> _legacyMapIds;
    QHash<int, QString> _cacheVariants;
    quint64 _defaultSet = kInvalidTileSet;
    bool _connected = false;
    bool _valid = false;
    bool _failed = false;
    /// Off for a file that is being imported: when it cannot be migrated, init() fails and leaves it as it is
    bool _resetOnMigrationFailure = true;
    static constexpr int kPruneBatchSize = 128;
    static constexpr const char* kUniqueTilesSubquery =
        "SELECT A.tileID FROM SetTiles A JOIN SetTiles B ON A.tileID = B.tileID "
        "WHERE B.setID = ? GROUP BY A.tileID HAVING COUNT(A.tileID) = 1";
};
