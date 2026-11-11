#pragma once

#include <memory>
#include <optional>

#include <QtCore/QByteArray>
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
    static constexpr int kSchemaVersion = 2;

    explicit QGCTileCacheDatabase(const QString& databasePath);
    ~QGCTileCacheDatabase();

    bool init();
    bool connectDB();
    void disconnectDB();

    /// Replaces the provider ids a version-1 cache is renumbered from (UrlFactory::legacyMapIds()). Call before init().
    /// For unit tests: on the platforms they run on, version 1 had the permanent ids already.
    void setLegacyMapIds(const QMap<int, int>& legacyMapIds) { _legacyMapIds = legacyMapIds; }

    bool isValid() const { return _valid; }

    bool hasFailed() const { return _failed; }

    // Tiles
    bool saveTile(const QString& hash, const QString& format, const QByteArray& img, const QString& type,
                  quint64 tileSet);
    std::unique_ptr<QGCCacheTile> getTile(const QString& hash);
    std::optional<quint64> findTile(const QString& hash);

    // Tile Sets
    QList<TileSetRecord> getTileSets();
    std::optional<quint64> createTileSet(const QString& name, const QString& mapTypeStr, double topleftLat,
                                         double topleftLon, double bottomRightLat, double bottomRightLon, int minZoom,
                                         int maxZoom, const QString& type, quint32 numTiles);
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
    void deleteBingNoTileTiles();

    // Stats
    TotalsResult computeTotals();
    SetTotalsResult computeSetTotals(quint64 setID, bool isDefault, quint32 totalTileCount, const QString& type);

    // Import/Export
    DatabaseResult importSetsReplace(const QString& path, ProgressCallback progressCb);
    DatabaseResult importSetsMerge(const QString& path, ProgressCallback progressCb);
    DatabaseResult exportSets(const QList<TileSetRecord>& sets, const QString& path, ProgressCallback progressCb);

    // Exposed for unit tests only
    QSqlDatabase database() const;

    static constexpr const char* kBingNoTileDoneKey = "_deleteBingNoTileTilesDone";

private:
    bool _ensureConnected() const;
    QSqlDatabase _database() const;
    bool _checkSchemaVersion();
    bool _migrate(QSqlDatabase db, int version);
    static bool _renumberMapIds(QSqlDatabase db, const QMap<int, int>& legacyMapIds);
    bool _createDB(QSqlDatabase db, bool createDefault = true);
    quint64 _getDefaultTileSet();
    bool _deleteTilesByIDs(const QList<quint64>& ids);
    QString _deduplicateSetName(const QString& name);
    quint64 _copyTilesForSet(QSqlDatabase srcDB, quint64 srcSetID, quint64 dstSetID, quint64& currentCount,
                             quint64 tileCount, int& lastProgress, ProgressCallback progressCb,
                             quint64* tilesIteratedOut, bool useTransaction = true);

    QString _databasePath;
    QString _connectionName;
    QMap<int, int> _legacyMapIds;
    quint64 _defaultSet = kInvalidTileSet;
    bool _connected = false;
    bool _valid = false;
    bool _failed = false;
    static constexpr int kPruneBatchSize = 128;
    static constexpr const char* kUniqueTilesSubquery =
        "SELECT A.tileID FROM SetTiles A JOIN SetTiles B ON A.tileID = B.tileID "
        "WHERE B.setID = ? GROUP BY A.tileID HAVING COUNT(A.tileID) = 1";
};
