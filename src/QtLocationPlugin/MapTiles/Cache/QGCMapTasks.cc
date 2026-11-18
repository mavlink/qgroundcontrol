#include "QGCMapTasks.h"

#include <memory>
#include <utility>

#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QHash>

#include "MapProvider.h"
#include "QGCCacheTile.h"
#include "QGCCacheWorker.h"
#include "QGCCachedTileSet.h"
#include "QGCTileCacheDatabase.h"
#include "UrlFactory.h"

namespace {

// A result is made on the worker thread and handed over on another. It is shared with the handler and not read from
// the task, which can be deleted before its finished() arrives. What never reaches a handler is deleted here.

struct MapTaskTileSets
{
    ~MapTaskTileSets()
    {
        for (QGCCachedTileSet* const tileSet : std::as_const(tileSets)) {
            tileSet->deleteLater();
        }
    }

    QList<QGCCachedTileSet*> tileSets;
};

struct MapTaskDownloadList
{
    ~MapTaskDownloadList() { qDeleteAll(tiles); }

    QQueue<QGCTile*> tiles;
};

void applyMapTaskSetTotals(QGCCachedTileSet& tileSet, const SetTotalsResult& totals)
{
    tileSet.setSavedTileCount(totals.savedTileCount);
    tileSet.setSavedTileSize(totals.savedTileSize);
    tileSet.setTotalTileSize(totals.totalTileSize);
    tileSet.setUniqueTileCount(totals.uniqueTileCount);
    tileSet.setUniqueTileSize(totals.uniqueTileSize);
}

}  // namespace

namespace QGCMapTasks {

QGCMapTask* init()
{
    return new QGCMapTask(QStringLiteral("Open Tile Cache"));
}

QGCMapTask* saveTile(const QGCCacheTile& tile)
{
    return new QGCMapTask(QStringLiteral("Save Tile"), [tile](QGCCacheWorker& worker, QGCMapTask&) {
        return worker.database().saveTile(tile.hash, tile.format, tile.img, tile.type, tile.tileSet)
                   ? QString()
                   : QStringLiteral("Error saving tile to cache");
    });
}

QGCMapTask* fetchTile(const QString& hash, const QObject* context,
                      std::function<void(const QGCCacheTile* tile)> fetched)
{
    const auto tile = std::make_shared<std::unique_ptr<QGCCacheTile>>();
    QGCMapTask* const task =
        new QGCMapTask(QStringLiteral("Fetch Tile"), [hash, tile](QGCCacheWorker& worker, QGCMapTask&) {
            *tile = worker.tile(hash);
            return *tile ? QString() : QStringLiteral("Tile not in cache database");
        });
    (void) QObject::connect(task, &QGCMapTask::finished, context,
                            [tile, fetched = std::move(fetched)] { fetched(tile->get()); });
    return task;
}

QGCMapTask* fetchTileSets(const QObject* context, std::function<void(const QList<QGCCachedTileSet*>& tileSets)> fetched)
{
    const auto result = std::make_shared<MapTaskTileSets>();
    QGCMapTask* const task =
        new QGCMapTask(QStringLiteral("Fetch Tile Set"), [result](QGCCacheWorker& worker, QGCMapTask&) {
            QGCTileCacheDatabase& database = worker.database();
            const QList<TileSetRecord> records = database.getTileSets();
            for (const TileSetRecord& rec : records) {
                QGCCachedTileSet* const set = new QGCCachedTileSet(rec.name);
                set->blockSignals(true);

                set->setId(rec.setID);
                set->setMapTypeStr(rec.mapTypeStr);
                set->setTopleftLat(rec.topleftLat);
                set->setTopleftLon(rec.topleftLon);
                set->setBottomRightLat(rec.bottomRightLat);
                set->setBottomRightLon(rec.bottomRightLon);
                set->setMinZoom(rec.minZoom);
                set->setMaxZoom(rec.maxZoom);
                set->setType(UrlFactory::getProviderTypeFromMapId(rec.type));
                set->setDefaultSet(rec.defaultSet);
                set->setCreationDate(QDateTime::fromSecsSinceEpoch(rec.date));

                const SetTotalsResult totals =
                    database.computeSetTotals(rec.setID, rec.defaultSet, rec.numTiles, set->type());
                set->setTotalTileCount(totals.totalTileCount);
                applyMapTaskSetTotals(*set, totals);

                set->blockSignals(false);
                (void) set->moveToThread(QCoreApplication::instance()->thread());
                result->tileSets.append(set);
            }
            return QString();
        });
    (void) QObject::connect(task, &QGCMapTask::finished, context,
                            [result, fetched = std::move(fetched)](const QString& error) {
                                if (error.isEmpty()) {
                                    fetched(std::exchange(result->tileSets, {}));
                                }
                            });
    return task;
}

QGCMapTask* createTileSet(QGCCachedTileSet* tileSet, const QObject* context,
                          std::function<void(QGCCachedTileSet* tileSet)> saved)
{
    const auto result = std::make_shared<MapTaskTileSets>();
    result->tileSets.append(tileSet);
    const SharedMapProvider provider = UrlFactory::findProvider(tileSet->type());
    const QString cacheVariant = provider ? provider->cacheVariant() : QString();

    // The task keeps the set alive for its work, also when its context is destroyed
    QGCMapTask* const task =
        new QGCMapTask(QStringLiteral("Create Tile Set"), [result, cacheVariant](QGCCacheWorker& worker, QGCMapTask&) {
            QGCCachedTileSet* const set = result->tileSets.constFirst();
            QGCTileCacheDatabase& database = worker.database();
            const auto setID =
                database.createTileSet(set->name(), set->mapTypeStr(), set->topleftLat(), set->topleftLon(),
                                       set->bottomRightLat(), set->bottomRightLon(), set->minZoom(), set->maxZoom(),
                                       set->type(), set->totalTileCount(), cacheVariant);
            if (!setID.has_value()) {
                return QStringLiteral("Error saving tile set");
            }

            set->blockSignals(true);
            set->setId(setID.value());
            applyMapTaskSetTotals(
                *set, database.computeSetTotals(setID.value(), set->defaultSet(), set->totalTileCount(), set->type()));
            set->blockSignals(false);
            return QString();
        });
    (void) QObject::connect(task, &QGCMapTask::finished, context,
                            [result, saved = std::move(saved)](const QString& error) {
                                if (error.isEmpty()) {
                                    saved(std::exchange(result->tileSets, {}).constFirst());
                                }
                            });
    return task;
}

QGCMapTask* tileDownloadList(quint64 setID, int count, const QObject* context,
                             std::function<void(const QQueue<QGCTile*>& tiles)> fetched)
{
    const auto result = std::make_shared<MapTaskDownloadList>();
    QGCMapTask* const task = new QGCMapTask(
        QStringLiteral("Get Tile Download List"), [setID, count, result](QGCCacheWorker& worker, QGCMapTask&) {
            const QList<QGCTile> tiles = worker.database().getTileDownloadList(setID, count);
            for (const QGCTile& tile : tiles) {
                result->tiles.enqueue(new QGCTile(tile));
            }
            return QString();
        });
    (void) QObject::connect(task, &QGCMapTask::finished, context,
                            [result, fetched = std::move(fetched)](const QString& error) {
                                if (error.isEmpty()) {
                                    fetched(std::exchange(result->tiles, {}));
                                }
                            });
    return task;
}

QGCMapTask* updateTileDownloadState(quint64 setID, QGCTile::TileState state, const QString& hash)
{
    return new QGCMapTask(
        QStringLiteral("Update Tile Download Status"), [setID, state, hash](QGCCacheWorker& worker, QGCMapTask&) {
            QGCTileCacheDatabase& database = worker.database();
            const bool updated = (hash == QStringLiteral("*"))
                                     ? database.updateAllTileDownloadStates(setID, static_cast<int>(state))
                                     : database.updateTileDownloadState(setID, static_cast<int>(state), hash);
            return updated ? QString() : QStringLiteral("Error updating tile download state");
        });
}

QGCMapTask* deleteTileSet(quint64 setID)
{
    return new QGCMapTask(QStringLiteral("Delete Tile Set"), [setID](QGCCacheWorker& worker, QGCMapTask&) {
        if (!worker.database().deleteTileSet(setID)) {
            return QStringLiteral("Error deleting tile set");
        }
        worker.setReleasePending();
        worker.emitTotals();
        return QString();
    });
}

QGCMapTask* renameTileSet(quint64 setID, const QString& newName)
{
    return new QGCMapTask(QStringLiteral("Rename Tile Set"), [setID, newName](QGCCacheWorker& worker, QGCMapTask&) {
        return worker.database().renameTileSet(setID, newName) ? QString() : QStringLiteral("Error renaming tile set");
    });
}

QGCMapTask* pruneCache(quint64 amount)
{
    return new QGCMapTask(QStringLiteral("Prune Tile Cache"), [amount](QGCCacheWorker& worker, QGCMapTask&) {
        if (!worker.database().pruneCache(amount)) {
            return QStringLiteral("Error pruning cache");
        }
        worker.setReleasePending();
        return QString();
    });
}

QGCMapTask* resetCache()
{
    return new QGCMapTask(QStringLiteral("Reset Tile Sets"), [](QGCCacheWorker& worker, QGCMapTask&) {
        return worker.database().resetDatabase() ? QString() : QStringLiteral("Error resetting cache database");
    });
}

QGCMapTask* exportSets(const QList<TileSetRecord>& sets, const QString& path)
{
    return new QGCMapTask(QStringLiteral("Export Tile Sets"), [sets, path](QGCCacheWorker& worker, QGCMapTask& task) {
        const DatabaseResult result =
            worker.database().exportSets(sets, path, [&task](int percentage) { emit task.progress(percentage); });
        return result.success ? QString() : result.errorString;
    });
}

QGCMapTask* importSets(const QString& path, bool replace)
{
    const QHash<int, QString> cacheVariants = UrlFactory::cacheVariants();
    return new QGCMapTask(
        QStringLiteral("Import Tile Sets"), [path, replace, cacheVariants](QGCCacheWorker& worker, QGCMapTask& task) {
            QGCTileCacheDatabase& database = worker.database();
            database.setCacheVariants(cacheVariants);
            const auto progress = [&task](int percentage) { emit task.progress(percentage); };
            const DatabaseResult result =
                replace ? database.importSetsReplace(path, progress) : database.importSetsMerge(path, progress);
            // A merge deletes the sets it has no tile to add to, and a file that replaces the
            // cache can have unused pages
            worker.setReleasePending();
            return result.success ? QString() : result.errorString;
        });
}

}  // namespace QGCMapTasks
