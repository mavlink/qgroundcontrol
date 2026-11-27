#pragma once

#include <functional>

#include <QtCore/QString>

#include "CacheTask.h"

struct CacheTile;

/// The tasks of the tile cache, for TileCache::addTask(): TileSetTasks has those of its offline tile sets.
/// CacheTask::finished() tells the outcome of each. A task with a result hands it to a handler, which runs on the
/// thread of its context object after finished(), and not at all when that object is destroyed first.
namespace CacheTasks {

/// Starts the worker, which opens the cache
CacheTask* init();

CacheTask* saveTile(const CacheTile& tile);

/// \a fetched gets the tile: null for a tile the cache lacks, which is no failure of the task, and for a task that
/// failed
CacheTask* fetchTile(const QString& hash, const QObject* context, std::function<void(const CacheTile* tile)> fetched);

/// Deletes the tiles that no set but the default one has and that were saved or used longest ago, until \a amount
/// bytes are freed
CacheTask* pruneCache(quint64 amount);

CacheTask* resetCache();

}  // namespace CacheTasks
