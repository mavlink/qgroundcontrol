#include "CacheTasks.h"

#include <memory>
#include <utility>

#include <QtCore/QCoreApplication>

#include "CacheTile.h"
#include "CacheWorker.h"
#include "TileCacheDatabase.h"

namespace CacheTasks {

CacheTask* init()
{
    return new CacheTask(QStringLiteral("Open Tile Cache"));
}

CacheTask* saveTile(const CacheTile& tile)
{
    return new CacheTask(QStringLiteral("Save Tile"), [tile](CacheWorker& worker, CacheTask&) {
        return worker.database().saveTile(tile.hash, tile.format, tile.img, tile.mapId, tile.tileSet)
                   ? QString()
                   : QStringLiteral("Error saving tile to cache");
    });
}

CacheTask* fetchTile(const QString& hash, const QObject* context, std::function<void(const CacheTile* tile)> fetched)
{
    const auto tile = std::make_shared<std::unique_ptr<CacheTile>>();
    CacheTask* const task = new CacheTask(QStringLiteral("Fetch Tile"), [hash, tile](CacheWorker& worker, CacheTask&) {
        *tile = worker.tile(hash);
        return QString();
    });
    (void) QObject::connect(task, &CacheTask::finished, context,
                            [tile, fetched = std::move(fetched)] { fetched(tile->get()); });
    return task;
}

CacheTask* pruneCache(quint64 amount)
{
    return new CacheTask(QStringLiteral("Prune Tile Cache"), [amount](CacheWorker& worker, CacheTask&) {
        if (!worker.database().pruneCache(amount)) {
            return QStringLiteral("Error pruning cache");
        }
        worker.setReleasePending();
        return QString();
    });
}

CacheTask* resetCache()
{
    return new CacheTask(QCoreApplication::translate("CacheTasks", "Reset Tile Sets"),
                         [](CacheWorker& worker, CacheTask&) {
                             return worker.database().resetDatabase()
                                        ? QString()
                                        : QCoreApplication::translate("CacheTasks", "Error resetting cache database");
                         });
}

}  // namespace CacheTasks
