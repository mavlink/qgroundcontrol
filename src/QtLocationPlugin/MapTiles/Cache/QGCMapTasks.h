#pragma once

#include <functional>

#include <QtCore/QList>
#include <QtCore/QQueue>
#include <QtCore/QString>

#include "QGCMapTask.h"
#include "QGCTile.h"
#include "QGCTileCacheTypes.h"

class QGCCachedTileSet;
struct QGCCacheTile;

/// The tasks of the tile cache, for QGCMapEngine::addTask(). QGCMapTask::finished() tells the outcome of each. A task
/// with a result hands it to a handler, which runs on the thread of its context object after finished(), and not at
/// all when that object is destroyed first.
namespace QGCMapTasks {

/// Starts the worker, which opens the cache
QGCMapTask* init();

QGCMapTask* saveTile(const QGCCacheTile& tile);

/// \a fetched gets the tile: null for a tile the cache lacks and for a task that failed
QGCMapTask* fetchTile(const QString& hash, const QObject* context,
                      std::function<void(const QGCCacheTile* tile)> fetched);

/// \a fetched gets the tile sets of the cache, which are its to delete, unless the task failed
QGCMapTask* fetchTileSets(const QObject* context,
                          std::function<void(const QList<QGCCachedTileSet*>& tileSets)> fetched);

/// Saves \a tileSet with the list of its tiles to download. The task owns \a tileSet until \a saved gets it, with
/// its id and its totals: a set that cannot be saved is deleted. Call on the main thread: the cache variant of the
/// set's provider is read here, for the worker thread that computes the keys of the set's tiles.
QGCMapTask* createTileSet(QGCCachedTileSet* tileSet, const QObject* context,
                          std::function<void(QGCCachedTileSet* tileSet)> saved);

/// \a fetched gets at most \a count tiles that the set is to download, which the cache has as being downloaded from
/// then on. The tiles are its to delete. Not called for a task that failed.
QGCMapTask* tileDownloadList(quint64 setID, int count, const QObject* context,
                             std::function<void(const QQueue<QGCTile*>& tiles)> fetched);

/// \a hash "*" stands for every tile of the set
QGCMapTask* updateTileDownloadState(quint64 setID, QGCTile::TileState state, const QString& hash);

QGCMapTask* deleteTileSet(quint64 setID);
QGCMapTask* renameTileSet(quint64 setID, const QString& newName);

/// Deletes the oldest tiles that no set but the default one has, until \a amount bytes are freed
QGCMapTask* pruneCache(quint64 amount);

QGCMapTask* resetCache();

/// Emits QGCMapTask::progress()
QGCMapTask* exportSets(const QList<TileSetRecord>& sets, const QString& path);

/// Emits QGCMapTask::progress(). Call on the main thread: the cache variants are read here, which the tiles of a
/// file from before schema version 3 get.
QGCMapTask* importSets(const QString& path, bool replace);

}  // namespace QGCMapTasks
