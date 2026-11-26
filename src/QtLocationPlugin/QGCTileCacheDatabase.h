#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QString>

#include <memory>
#include <optional>

#include "QGCTileCacheTypes.h"
#include "QGCTile.h"

struct QGCCacheTile;
class QSqlDatabase;

class QGCTileCacheDatabase
{
public:
    static constexpr quint64 kInvalidTileSet = UINT64_MAX;
    static constexpr int kSchemaVersion = 1;

    explicit QGCTileCacheDatabase(const QString &databasePath);
    ~QGCTileCacheDatabase();

    bool init();
    bool connectDB();
    void disconnectDB();

    bool isValid() const { return _valid; }
    bool hasFailed() const { return _failed; }

    // Tiles
    bool saveTile(const QString &hash, const QString &format, const QByteArray &img, const QString &type, quint64 tileSet);
    std::unique_ptr<QGCCacheTile> getTile(const QString &hash);
    std::optional<quint64> findTile(const QString &hash);

    // Tile Sets
    QList<TileSetRecord> getTileSets();
    std::optional<quint64> createTileSet(const QString &name, const QString &mapTypeStr,
                                         double topleftLat, double topleftLon,
                                         double bottomRightLat, double bottomRightLon,
                                         int minZoom, int maxZoom, const QString &type, quint32 numTiles);
    bool deleteTileSet(quint64 id);
    bool renameTileSet(quint64 setID, const QString &newName);
    std::optional<quint64> findTileSetID(const QString &name);
    bool resetDatabase();

    // Downloads
    QList<QGCTile> getTileDownloadList(quint64 setID, int count);
    bool updateTileDownloadState(quint64 setID, int state, const QString &hash);
    bool updateAllTileDownloadStates(quint64 setID, int state);

    // Cache
    bool pruneCache(quint64 amount);
    void deleteBingNoTileTiles();

    // Stats
    TotalsResult computeTotals();
    SetTotalsResult computeSetTotals(quint64 setID, bool isDefault, quint32 totalTileCount, const QString &type);

    // Import/Export
    /// Replace the cache with a tile set file. A file that is not one, or is of another schema version, is refused,
    /// and so is the import that fails at any step: the cache is then as it was. While the cache is replaced, it is
    /// kept in a backup beside it. A backup that an import left, by a crash or a failed restore, is never removed:
    /// while there is one, the import is refused. init() puts it back if no cache is in its place
    /// @param movable Whether @a path may be moved into place: a file of the caller's own, on the disk of the database.
    ///        It is copied if it cannot be moved
    DatabaseResult importSetsReplace(const QString& path, ProgressCallback progressCb, bool movable = false);
    /// Add the tile sets of a tile set file to the cache. A file that is not one is refused
    DatabaseResult importSetsMerge(const QString &path, ProgressCallback progressCb);
    DatabaseResult exportSets(const QList<TileSetRecord> &sets, const QString &path, ProgressCallback progressCb);

    /// SQLite URI that opens @a absolutePath read-only, as it is ("immutable"). Its authority stays empty, as SQLite
    /// requires: a UNC path (//server/share/x.db) gives file:////server/share/x.db, a drive path file:///C:/x.db
    static QString immutableFileUri(const QString& absolutePath);

    // Exposed for unit tests only
    QSqlDatabase database() const;

    static constexpr const char *kBingNoTileDoneKey = "_deleteBingNoTileTilesDone";

private:
    bool _ensureConnected() const;
    QSqlDatabase _database() const;
    bool _checkSchemaVersion();
    /// Why a file cannot be imported, empty if it can
    /// @param requireCurrentSchema Whether the file must have the schema version of the cache, which a replace import
    ///        needs: init() would discard the tiles of another
    static QString _importRefusal(const QString& path, bool requireCurrentSchema);
    /// Put the cache that importSetsReplace() set aside back in place, and open it
    /// @return What became of it, for the message of the failed import
    QString _restoreAfterFailedImport(const QString& backupPath, bool hasBackup);
    bool _createDB(QSqlDatabase db, bool createDefault = true);
    quint64 _getDefaultTileSet();
    bool _deleteTilesByIDs(const QList<quint64> &ids);
    QString _deduplicateSetName(const QString &name);
    quint64 _copyTilesForSet(QSqlDatabase srcDB, quint64 srcSetID, quint64 dstSetID,
                              quint64 &currentCount, quint64 tileCount,
                              int &lastProgress, ProgressCallback progressCb,
                              quint64 *tilesIteratedOut, bool useTransaction = true);

    QString _databasePath;
    QString _connectionName;
    quint64 _defaultSet = kInvalidTileSet;
    bool _connected = false;
    bool _valid = false;
    bool _failed = false;
    static constexpr int kPruneBatchSize = 128;
    static constexpr const char *kUniqueTilesSubquery =
        "SELECT A.tileID FROM SetTiles A JOIN SetTiles B ON A.tileID = B.tileID "
        "WHERE B.setID = ? GROUP BY A.tileID HAVING COUNT(A.tileID) = 1";
};
