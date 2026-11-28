#pragma once

#include <functional>

#include <QtCore/QList>
#include <QtCore/QString>

#include "DownloadTile.h"
#include "TileCacheTypes.h"

class CachedTileSet;
class CacheTask;
class QObject;

/// The tasks of the tile cache, for TileCache::addTask(), that are about its offline tile sets. They end and hand
/// over their results as those of CacheTasks do.
namespace TileSetTasks {

/// \a fetched gets the tile sets of the cache, which are its to delete, unless the task failed
CacheTask* fetchTileSets(const QObject* context, std::function<void(const QList<CachedTileSet*>& tileSets)> fetched);

/// Saves \a tileSet with the list of its tiles to download. The task owns \a tileSet until \a saved gets it, with
/// its id and its totals: a set that cannot be saved is deleted. Call on the main thread: the cache variant of the
/// set's provider is read here, for the worker thread that computes the keys of the set's tiles.
CacheTask* createTileSet(CachedTileSet* tileSet, const QObject* context,
                         std::function<void(CachedTileSet* tileSet)> saved);

/// \a fetched gets at most \a count tiles that the set is to download, which the cache has as being downloaded from
/// then on. Not called for a task that failed.
CacheTask* tileDownloadList(quint64 setID, int count, const QObject* context,
                            std::function<void(const QList<DownloadTile>& tiles)> fetched);

/// Has the cache list again, as tiles to download, those of the set that failed or that a download was given
CacheTask* resetTileDownloads(quint64 setID);

/// Stores what became of the tiles \a states of the set, all or none of them
CacheTask* updateTileDownloadStates(quint64 setID, const QList<DownloadTileState>& states);

CacheTask* deleteTileSet(quint64 setID);
/// Deletes the tiles of the cache that no offline set has
CacheTask* deleteCachedTiles();
CacheTask* renameTileSet(quint64 setID, const QString& newName);

/// Emits CacheTask::progress()
CacheTask* exportSets(const QList<TileSetRecord>& sets, const QString& path);

/// Emits CacheTask::progress(). Call on the main thread: the cache variants are read here, which the tiles of a
/// file from before schema version 3 get. The tiles of an MBTiles file become a tile set, named after the file, of
/// the map of the imported tiles (MapProviderTable::kImportedMapId), and the maps are then told of them
/// (MapEngine::tilesImported()). With \a replace, the file takes the place of all that the cache has.
CacheTask* importSets(const QString& path, bool replace);

}  // namespace TileSetTasks
