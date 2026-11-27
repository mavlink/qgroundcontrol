#include "TileSetTasks.h"

#include <memory>
#include <optional>
#include <utility>

#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QFileInfo>
#include <QtCore/QHash>
#include <QtCore/QSignalBlocker>

#include "CacheTask.h"
#include "CacheWorker.h"
#include "CachedTileSet.h"
#include "MapEngine.h"
#include "MapProvider.h"
#include "MapProviderRegistry.h"
#include "MapProviderTable.h"
#include "TileCacheDatabase.h"

namespace {

// A result is made on the worker thread and handed over on another. It is shared with the handler and not read from
// the task, which can be deleted before its finished() arrives. What never reaches a handler is deleted here.

struct TileSetTaskSets
{
    TileSetTaskSets() = default;

    ~TileSetTaskSets()
    {
        for (CachedTileSet* const tileSet : std::as_const(tileSets)) {
            tileSet->deleteLater();
        }
    }

    Q_DISABLE_COPY_MOVE(TileSetTaskSets)

    QList<CachedTileSet*> tileSets;
};

}  // namespace

namespace TileSetTasks {

CacheTask* fetchTileSets(const QObject* context, std::function<void(const QList<CachedTileSet*>& tileSets)> fetched)
{
    const auto result = std::make_shared<TileSetTaskSets>();
    CacheTask* const task = new CacheTask(
        QCoreApplication::translate("TileSetTasks", "Fetch Tile Set"), [result](CacheWorker& worker, CacheTask&) {
            TileCacheDatabase& database = worker.database();
            const QList<TileSetRecord> records = database.getTileSets();
            for (const TileSetRecord& rec : records) {
                CachedTileSet* const set = new CachedTileSet(rec);
                {
                    // No signal on this thread: the set is for the thread of the UI
                    const QSignalBlocker blocker(set);
                    set->setTotals(database.computeSetTotals(rec.setID, rec.defaultSet, rec.numTiles, rec.mapId));
                }
                (void) set->moveToThread(QCoreApplication::instance()->thread());
                result->tileSets.append(set);
            }
            return QString();
        });
    (void) QObject::connect(task, &CacheTask::finished, context,
                            [result, fetched = std::move(fetched)](const QString& error) {
                                if (error.isEmpty()) {
                                    fetched(std::exchange(result->tileSets, {}));
                                }
                            });
    return task;
}

CacheTask* createTileSet(CachedTileSet* tileSet, const QObject* context,
                         std::function<void(CachedTileSet* tileSet)> saved)
{
    const auto result = std::make_shared<TileSetTaskSets>();
    result->tileSets.append(tileSet);
    const TileSetRecord record = tileSet->record();
    const SharedMapProvider provider = MapProviderRegistry::getMapProviderFromMapId(record.mapId);
    const QString cacheVariant = provider ? provider->cacheVariant() : QString();

    // The task keeps the set alive for its work, also when its context is destroyed
    CacheTask* const task = new CacheTask(
        QCoreApplication::translate("TileSetTasks", "Create Tile Set"),
        [result, record, cacheVariant](CacheWorker& worker, CacheTask&) {
            CachedTileSet* const set = result->tileSets.constFirst();
            TileCacheDatabase& database = worker.database();
            const auto setID = database.createTileSet(record, cacheVariant);
            if (!setID.has_value()) {
                return QCoreApplication::translate("TileSetTasks", "Error saving tile set");
            }

            // The total of the set stays the one it was planned with
            TileSetTotals totals = database.computeSetTotals(setID.value(), false, record.numTiles, record.mapId);
            totals.totalTileCount = record.numTiles;
            // No signal on this thread: the set is of the thread of the UI
            const QSignalBlocker blocker(set);
            set->setId(setID.value());
            set->setTotals(totals);
            return QString();
        });
    (void) QObject::connect(task, &CacheTask::finished, context,
                            [result, saved = std::move(saved)](const QString& error) {
                                if (error.isEmpty()) {
                                    saved(std::exchange(result->tileSets, {}).constFirst());
                                }
                            });
    return task;
}

CacheTask* tileDownloadList(quint64 setID, int count, const QObject* context,
                            std::function<void(const QList<DownloadTile>& tiles)> fetched)
{
    const auto result = std::make_shared<QList<DownloadTile>>();
    CacheTask* const task = new CacheTask(
        QCoreApplication::translate("TileSetTasks", "Get Tile Download List"),
        [setID, count, result](CacheWorker& worker, CacheTask&) {
            const std::optional<QList<DownloadTile>> tiles = worker.database().getTileDownloadList(setID, count);
            // Not an empty list: the download would end as if the set had every tile
            if (!tiles.has_value()) {
                return QCoreApplication::translate("TileSetTasks", "Error listing tiles to download");
            }
            *result = tiles.value();
            return QString();
        });
    (void) QObject::connect(task, &CacheTask::finished, context,
                            [result, fetched = std::move(fetched)](const QString& error) {
                                if (error.isEmpty()) {
                                    fetched(*result);
                                }
                            });
    return task;
}

CacheTask* resetTileDownloads(quint64 setID)
{
    return new CacheTask(
        QCoreApplication::translate("TileSetTasks", "Reset Tile Downloads"), [setID](CacheWorker& worker, CacheTask&) {
            return worker.database().updateAllTileDownloadStates(setID, DownloadTile::TileState::Pending)
                       ? QString()
                       : QCoreApplication::translate("TileSetTasks", "Error resetting tile download states");
        });
}

CacheTask* updateTileDownloadStates(quint64 setID, const QList<DownloadTileState>& states)
{
    return new CacheTask(QCoreApplication::translate("TileSetTasks", "Update Tile Download Status"),
                         [setID, states](CacheWorker& worker, CacheTask&) {
                             return worker.database().updateTileDownloadStates(setID, states)
                                        ? QString()
                                        : QCoreApplication::translate("TileSetTasks",
                                                                      "Error updating tile download states");
                         });
}

CacheTask* deleteTileSet(quint64 setID)
{
    return new CacheTask(QCoreApplication::translate("TileSetTasks", "Delete Tile Set"),
                         [setID](CacheWorker& worker, CacheTask&) {
                             if (!worker.database().deleteTileSet(setID)) {
                                 return QCoreApplication::translate("TileSetTasks", "Error deleting tile set");
                             }
                             worker.setReleasePending();
                             worker.emitTotals();
                             return QString();
                         });
}

CacheTask* deleteCachedTiles()
{
    return new CacheTask(QCoreApplication::translate("TileSetTasks", "Clear Tile Cache"),
                         [](CacheWorker& worker, CacheTask&) {
                             if (!worker.database().deleteCachedTiles()) {
                                 return QCoreApplication::translate("TileSetTasks", "Error deleting cached tiles");
                             }
                             worker.setReleasePending();
                             worker.emitTotals();
                             return QString();
                         });
}

CacheTask* renameTileSet(quint64 setID, const QString& newName)
{
    return new CacheTask(QCoreApplication::translate("TileSetTasks", "Rename Tile Set"),
                         [setID, newName](CacheWorker& worker, CacheTask&) {
                             return worker.database().renameTileSet(setID, newName)
                                        ? QString()
                                        : QCoreApplication::translate("TileSetTasks", "Error renaming tile set");
                         });
}

CacheTask* exportSets(const QList<TileSetRecord>& sets, const QString& path)
{
    return new CacheTask(QCoreApplication::translate("TileSetTasks", "Export Tile Sets"),
                         [sets, path](CacheWorker& worker, CacheTask& task) {
                             const TileCacheResult result = worker.database().exportSets(
                                 sets, path, [&task](int percentage) { emit task.progress(percentage); });
                             return result.success ? QString() : result.errorString;
                         });
}

CacheTask* importSets(const QString& path, bool replace)
{
    const QHash<int, QString> cacheVariants = MapProviderRegistry::cacheVariants();
    TileSetRecord mbtilesSet;
    mbtilesSet.mapId = MapProviderTable::kImportedMapId;
    mbtilesSet.mapName = MapProviderRegistry::getProviderTypeFromMapId(mbtilesSet.mapId);
    // A file that is named by its extension alone has no name to give the set
    const QString fileName = QFileInfo(path).completeBaseName();
    mbtilesSet.name = fileName.isEmpty() ? mbtilesSet.mapName : fileName;
    const auto mbtilesImported = std::make_shared<bool>(false);

    CacheTask* const task = new CacheTask(
        QCoreApplication::translate("TileSetTasks", "Import Tile Sets"),
        [path, replace, cacheVariants, mbtilesSet, mbtilesImported](CacheWorker& worker, CacheTask& task) {
            TileCacheDatabase& database = worker.database();
            const auto progress = [&task](int percentage) { emit task.progress(percentage); };
            TileCacheResult result;
            if (TileCacheDatabase::isMBTiles(path)) {
                result = database.importMBTiles(path, mbtilesSet, replace, progress);
                *mbtilesImported = result.success;
            } else {
                database.setCacheVariants(cacheVariants);
                result =
                    replace ? database.importSetsReplace(path, progress) : database.importSetsMerge(path, progress);
            }
            // A merge deletes the sets it has no tile to add to, and what replaces the cache leaves unused pages
            worker.setReleasePending();
            return result.success ? QString() : result.errorString;
        });
    // The maps asked for these tiles in vain before, or have those of a file that was imported earlier
    (void) QObject::connect(task, &CacheTask::finished, MapEngine::instance(), [mbtilesSet, mbtilesImported] {
        if (*mbtilesImported) {
            MapEngine::instance()->tilesImported(mbtilesSet.mapId);
        }
    });
    return task;
}

}  // namespace TileSetTasks
