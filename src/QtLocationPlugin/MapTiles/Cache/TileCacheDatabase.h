#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <optional>

#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QLoggingCategory>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>

#include "DownloadTile.h"
#include "TileCacheTypes.h"

struct CacheTile;
class QSqlDatabase;
class QSqlQuery;

Q_DECLARE_LOGGING_CATEGORY(TileCacheDatabaseLog)

class TileCacheDatabase
{
    Q_DECLARE_TR_FUNCTIONS(TileCacheDatabase)

public:
    static constexpr quint64 kInvalidTileSet = UINT64_MAX;
    /// 2: provider ids are the permanent ones (version 1 had the position in the provider list of the build)
    /// 3: the key of a tile ends with the cache variant of its provider (MapProvider::getTileHash())
    static constexpr int kSchemaVersion = 3;

    explicit TileCacheDatabase(const QString& databasePath);
    ~TileCacheDatabase();
    Q_DISABLE_COPY_MOVE(TileCacheDatabase)

    bool init();
    bool connectDB();
    void disconnectDB();

    /// Replaces the provider ids a version-1 cache is renumbered from (MapProviderRegistry::legacyMapIds()). Call
    /// before init(). For unit tests: on the platforms they run on, version 1 had the permanent ids already.
    void setLegacyMapIds(const QMap<int, int>& legacyMapIds) { _legacyMapIds = legacyMapIds; }

    /// The cache variants by provider id (MapProviderRegistry::cacheVariants()) that the tiles of a file from before
    /// schema version 3 get: of the cache when set before init(), of a file imported afterwards.
    void setCacheVariants(const QHash<int, QString>& cacheVariants) { _cacheVariants = cacheVariants; }

    bool isValid() const { return _valid; }

    bool hasFailed() const { return _failed; }

    // Tiles
    bool saveTile(const QString& hash, const QString& format, const QByteArray& img, int mapId, quint64 tileSet);
    std::unique_ptr<CacheTile> getTile(const QString& hash) const;
    /// Notes that the tile \a hash is in use, for pruneCache(). To kUseDateGranularity: a tile is written once in
    /// that time at most, however often it is read, and a tile of an offline set, which is not pruned, never.
    void markTileUsed(const QString& hash);
    std::optional<quint64> findTile(const QString& hash) const;

    // Tile Sets
    QList<TileSetRecord> getTileSets() const;
    /// Saves \a set, of which its name, map, corners, zooms and tile count count, with the list of its tiles to
    /// download, and returns its id. \a cacheVariant is that of the provider of the set, which the caller got on the
    /// main thread: this runs on the worker thread, where the provider must not read it from the settings.
    std::optional<quint64> createTileSet(const TileSetRecord& set, const QString& cacheVariant);
    bool deleteTileSet(quint64 id);
    /// Deletes the tiles that only the default set has: those that no offline set of the user is to keep
    bool deleteCachedTiles();
    bool renameTileSet(quint64 setID, const QString& newName);
    std::optional<quint64> findTileSetID(const QString& name) const;
    bool resetDatabase();

    // Downloads
    /// Marks at most \a count tiles of the set that are pending as downloading, and returns them. None when the
    /// database fails: an empty list means that nothing is left to download.
    std::optional<QList<DownloadTile>> getTileDownloadList(quint64 setID, int count);
    bool updateTileDownloadState(quint64 setID, DownloadTile::TileState state, const QString& hash);
    /// updateTileDownloadState() for each of \a states, all or none of them
    bool updateTileDownloadStates(quint64 setID, const QList<DownloadTileState>& states);
    bool updateAllTileDownloadStates(quint64 setID, DownloadTile::TileState state);

    // Cache
    /// Deletes tiles that only the default set has until \a amount bytes are freed: those first that were saved or
    /// used (markTileUsed()) longest ago
    bool pruneCache(quint64 amount);
    /// Gives back to the file system at most \a maxPages of the pages that deleted rows left unused, and returns how
    /// many are left. Only a file with incremental auto-vacuum can: any other file keeps its unused pages for new
    /// rows, and 0 is returned.
    qint64 releaseFreePages(int maxPages);

    // Stats
    TileCacheTotals computeTotals();
    TileSetTotals computeSetTotals(quint64 setID, bool isDefault, quint32 totalTileCount, int mapId);

    // Import/Export
    TileCacheResult importSetsReplace(const QString& path, TileCacheProgressCallback progressCb);
    TileCacheResult importSetsMerge(const QString& path, TileCacheProgressCallback progressCb);
    TileCacheResult exportSets(const QList<TileSetRecord>& sets, const QString& path,
                               TileCacheProgressCallback progressCb);
    /// True for an MBTiles file at \a path: the tiles of a map, in the file format of other programs
    static bool isMBTiles(const QString& path);
    /// Adds the pictures of the MBTiles file at \a path to the cache as tiles of the map of \a set, and makes them a
    /// tile set with its name, the zoom levels of the tiles and the area of the file. A tile that the cache has of the
    /// same place, from a file imported before, is replaced. With \a replace, the cache has nothing but that set
    /// afterwards: its other sets and its tiles of every map are deleted. A file without a picture is not imported,
    /// and leaves the cache as it was. The tiles are committed kCopyChunkTiles at a time: an import that fails after
    /// it began leaves no set, and neither what \a replace deleted nor the tiles it replaced come back.
    TileCacheResult importMBTiles(const QString& path, const TileSetRecord& set, bool replace,
                                  TileCacheProgressCallback progressCb);

    // Exposed for unit tests only
    QSqlDatabase database() const;

private:
    /// Prepares \a sql, binds \a values in their order and runs the statement. An unsigned 64-bit value, as the id
    /// of a tile set is, is bound as the integer it is: Qt would bind it as text, which no number is equal to where a
    /// comparison lacks the affinity of a column. False when the statement cannot be prepared or when it fails:
    /// QSqlQuery::lastError() of \a query says why.
    [[nodiscard]] static bool _execBound(QSqlQuery& query, const QString& sql, const QVariantList& values = {});
    /// _execBound(), whose failure is a warning that tells \a what the statement was to do
    [[nodiscard]] static bool _execOrWarn(QSqlQuery& query, const QString& sql, const QVariantList& values,
                                          const char* what);
    /// The file itself gets smaller only when its write-ahead log is checkpointed, which SQLite otherwise leaves until
    /// about 1000 pages have changed or the cache is closed. Passive: a connection that reads the cache is not waited
    /// for, and a later checkpoint does what this one could not.
    static void _checkpointWriteAheadLog(QSqlQuery& query);
    /// Adds \a set to the tile sets of the schema \a schema of the connection of \a query, whose lastInsertId() is
    /// then the id of the set
    [[nodiscard]] static bool _insertTileCacheSet(QSqlQuery& query, const TileSetRecord& set,
                                                  QLatin1StringView schema = QLatin1StringView("main"));
    /// The tile sets of the schema \a schema of the connection of the cache: its own, or those of a file it attached
    QList<TileSetRecord> _tileSets(QLatin1StringView schema) const;
    /// Attaches the tile cache file at \a path to the connection of the cache, for _copySetTiles(), until
    /// _detachFile(). Not within a transaction.
    [[nodiscard]] bool _attachFile(const QString& path);
    void _detachFile();
    /// Copies the tiles of the set \a fromSetID of the schema \a from to the schema \a to, and links them to its set
    /// \a toSetID: one of the two is the cache, and the other one the file it attached. A tile that \a to has under
    /// the same key stays as it is, and is the one linked. A statement copies at most kCopyChunkTiles tiles, in a
    /// transaction of its own, and \a chunkCopied is told how many after each. Not within a transaction. Returns how
    /// many tiles the set got that it had not, or none when a statement failed: the chunks before it stay copied.
    std::optional<quint64> _copySetTiles(QLatin1StringView from, quint64 fromSetID, QLatin1StringView to,
                                         quint64 toSetID, const std::function<void(quint64)>& chunkCopied);
    bool _ensureConnected() const;
    QSqlDatabase _database() const;
    bool _checkSchemaVersion();
    bool _migrate(QSqlDatabase db, int version);
    static bool _renumberMapIds(QSqlDatabase db, const QMap<int, int>& legacyMapIds);
    static bool _adoptCacheVariants(QSqlDatabase db, const QHash<int, QString>& cacheVariants);

    /// Where importSetsReplace() keeps the cache until the file it imports has taken its place
    QString _backupPath() const { return _databasePath + QStringLiteral(".bak"); }

    /// Where an import puts its copy of the file
    QString _importCopyPath() const { return _databasePath + QStringLiteral(".import"); }

    /// False, with \a errorString, for a file that is the cache or one of \a workPaths, which the import removes
    bool _importPathAllowed(const QString& path, const QStringList& workPaths, QString& errorString) const;
    /// The schema version of the file at \a path when it can be imported. Otherwise \a errorString says why not.
    static std::optional<int> _importableVersion(const QString& path, QString& errorString);
    bool _createDB(QSqlDatabase db, bool createDefault = true);
    /// Removes the database file at \a path and the files SQLite keeps next to it in WAL mode. Only once no
    /// connection has it open: Windows does not remove an open file.
    static void _removeDatabaseFiles(const QString& path);
    quint64 _getDefaultTileSet();
    bool _deleteTilesByIDs(const QList<quint64>& ids);
    QString _deduplicateSetName(const QString& name);

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
    static constexpr std::chrono::hours kUseDateGranularity{24};
    static constexpr int kCopyChunkTiles = 2000;
    /// A tile is saved under the key of one the cache has when that one could not be used, or is of a file that was
    /// imported before: the new one takes its place
    static constexpr const char* kSaveTileSql =
        "INSERT INTO Tiles(hash, format, tile, size, type, date) VALUES(?, ?, ?, ?, ?, ?) "
        "ON CONFLICT(hash) DO UPDATE SET format = excluded.format, tile = excluded.tile, "
        "size = excluded.size, date = excluded.date";
    static constexpr const char* kUniqueTilesSubquery =
        "SELECT A.tileID FROM SetTiles A JOIN SetTiles B ON A.tileID = B.tileID "
        "WHERE B.setID = ? GROUP BY A.tileID HAVING COUNT(A.tileID) = 1";
};
