#include "QGCTileCacheWorker.h"
#include "QGCTileCacheDatabase.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QSettings>

#include "QGCCachedTileSet.h"
#include "QGCLoggingCategory.h"
#include "QGCMapTasks.h"
#include "QGCMapUrlEngine.h"

QGC_LOGGING_CATEGORY(QGCTileCacheWorkerLog, "QtLocationPlugin.QGCTileCacheWorker")

QGCCacheWorker::QGCCacheWorker(QObject *parent)
    : QThread(parent)
{
    qCDebug(QGCTileCacheWorkerLog) << this;
}

// L3: Defensive destructor ensures thread is stopped even if caller forgets
QGCCacheWorker::~QGCCacheWorker()
{
    stop();
    wait();
    qCDebug(QGCTileCacheWorkerLog) << this;
}

// C1: Added _taskQueue.clear() — qDeleteAll does not clear the container,
// leaving dangling pointers that the worker thread would dequeue.
void QGCCacheWorker::stop()
{
    _stopRequested = true;
    QMutexLocker lock(&_taskQueueMutex);
    qDeleteAll(_taskQueue);
    _taskQueue.clear();
    lock.unlock();

    if (isRunning()) {
        _waitc.wakeAll();
    }
}

bool QGCCacheWorker::enqueueTask(QGCMapTask *task)
{
    if (!_dbValid && !isRunning() && (task->type() != QGCMapTask::TaskType::taskInit)) {
        task->setError(tr("Database Not Initialized"));
        task->deleteLater();
        return false;
    }

    QMutexLocker lock(&_taskQueueMutex);
    _taskQueue.enqueue(task);
    lock.unlock();

    if (isRunning()) {
        _waitc.wakeAll();
    } else {
        start(QThread::NormalPriority);
    }

    return true;
}

void QGCCacheWorker::run()
{
    _stopRequested = false;
    _database = std::make_unique<QGCTileCacheDatabase>(_databasePath);

    if (!_database->init()) {
        qCWarning(QGCTileCacheWorkerLog) << "Failed To Init Database";
        _database.reset();

        QMutexLocker lock(&_taskQueueMutex);
        for (QGCMapTask *orphan : _taskQueue) {
            orphan->setError(tr("Database Init Failed"));
            orphan->deleteLater();
        }
        _taskQueue.clear();
        return;
    }

    if (_database->isValid()) {
        if (_database->connectDB()) {
            _database->deleteBingNoTileTiles();
        }
    }

    _dbValid = _database->isValid();

    // M1: Start timer before the loop — hasExpired() on an unstarted timer is UB
    _updateTimer.start();

    QMutexLocker lock(&_taskQueueMutex);
    while (!_stopRequested) {
        if (!_taskQueue.isEmpty()) {
            QGCMapTask* const task = _taskQueue.dequeue();
            lock.unlock();
            _runTask(task);
            lock.relock();
            task->deleteLater();

            const qsizetype count = _taskQueue.count();
            if (count > 100) {
                _updateTimeout = kLongTimeoutMs;
            } else if (count < 25) {
                _updateTimeout = kShortTimeoutMs;
            }

            if ((count == 0) || _updateTimer.hasExpired(_updateTimeout)) {
                if (_database && _database->isValid()) {
                    lock.unlock();
                    _emitTotals();
                    lock.relock();
                }
            }
        } else {
            (void) _waitc.wait(lock.mutex(), 5000);
        }
    }

    // H1: Drain any tasks enqueued between the break decision and shutdown.
    // Tasks are main-thread QObjects so deleteLater() posts to the main event loop.
    for (QGCMapTask *orphan : _taskQueue) {
        orphan->setError(tr("Worker shutting down"));
        orphan->deleteLater();
    }
    _taskQueue.clear();
    lock.unlock();

    _dbValid = false;
    if (_database) {
        _database->disconnectDB();
        _database.reset();
    }
}

void QGCCacheWorker::_runTask(QGCMapTask *task)
{
    switch (task->type()) {
    case QGCMapTask::TaskType::taskInit:
        // L2: No-op — used only to bootstrap the worker thread
        break;
    case QGCMapTask::TaskType::taskCacheTile:
        _saveTile(task);
        break;
    case QGCMapTask::TaskType::taskFetchTile:
        _getTile(task);
        break;
    case QGCMapTask::TaskType::taskFetchTileSets:
        _getTileSets(task);
        break;
    case QGCMapTask::TaskType::taskCreateTileSet:
        _createTileSet(task);
        break;
    case QGCMapTask::TaskType::taskGetTileDownloadList:
        _getTileDownloadList(task);
        break;
    case QGCMapTask::TaskType::taskUpdateTileDownloadState:
        _updateTileDownloadState(task);
        break;
    case QGCMapTask::TaskType::taskDeleteTileSet:
        _deleteTileSet(task);
        break;
    case QGCMapTask::TaskType::taskRenameTileSet:
        _renameTileSet(task);
        break;
    case QGCMapTask::TaskType::taskPruneCache:
        _pruneCache(task);
        break;
    case QGCMapTask::TaskType::taskReset:
        _resetCacheDatabase(task);
        break;
    case QGCMapTask::TaskType::taskExport:
        _exportSets(task);
        break;
    case QGCMapTask::TaskType::taskImport:
        _importSets(task);
        break;
    default:
        qCWarning(QGCTileCacheWorkerLog) << "given unhandled task type" << task->type();
        break;
    }
}

bool QGCCacheWorker::_testTask(QGCMapTask *mtask)
{
    if (!_database || !_database->isValid()) {
        mtask->setError("No Cache Database");
        return false;
    }

    return true;
}

void QGCCacheWorker::_emitTotals()
{
    TotalsResult t = _database->computeTotals();
    emit updateTotals(t.totalCount, t.totalSize, t.defaultCount, t.defaultSize);
    _updateTimer.restart();
}

// M2: Check saveTile return value
void QGCCacheWorker::_saveTile(QGCMapTask *mtask)
{
    if (!_testTask(mtask)) {
        return;
    }

    QGCSaveTileTask *task = static_cast<QGCSaveTileTask*>(mtask);
    if (!_database->saveTile(task->tile()->hash, task->tile()->format,
                             task->tile()->img, task->tile()->type, task->tile()->tileSet)) {
        mtask->setError("Error saving tile to cache");
    }
}

void QGCCacheWorker::_getTile(QGCMapTask *mtask)
{
    if (!_testTask(mtask)) {
        return;
    }

    QGCFetchTileTask *task = static_cast<QGCFetchTileTask*>(mtask);
    auto tile = _database->getTile(task->hash());
    if (tile) {
        task->setTileFetched(tile.release());
    } else {
        task->setError("Tile not in cache database");
    }
}

// M4: Empty result is not an error (fresh DB or all sets deleted)
// M5: Block signals on worker-thread-created QObjects until moveToThread
void QGCCacheWorker::_getTileSets(QGCMapTask *mtask)
{
    if (!_testTask(mtask)) {
        return;
    }

    QGCFetchTileSetTask *task = static_cast<QGCFetchTileSetTask*>(mtask);
    const QList<TileSetRecord> records = _database->getTileSets();

    for (const auto &rec : records) {
        QGCCachedTileSet *set = new QGCCachedTileSet(rec.name);
        set->blockSignals(true);

        set->setId(rec.setID);
        set->setMapTypeStr(rec.mapTypeStr);
        set->setTopleftLat(rec.topleftLat);
        set->setTopleftLon(rec.topleftLon);
        set->setBottomRightLat(rec.bottomRightLat);
        set->setBottomRightLon(rec.bottomRightLon);
        set->setMinZoom(rec.minZoom);
        set->setMaxZoom(rec.maxZoom);
        set->setType(UrlFactory::getProviderTypeFromQtMapId(rec.type));
        set->setTotalTileCount(rec.numTiles);
        set->setDefaultSet(rec.defaultSet);
        set->setCreationDate(QDateTime::fromSecsSinceEpoch(rec.date));

        const SetTotalsResult totals = _database->computeSetTotals(rec.setID, rec.defaultSet, rec.numTiles, set->type());
        set->setSavedTileCount(totals.savedTileCount);
        set->setSavedTileSize(totals.savedTileSize);
        set->setTotalTileSize(totals.totalTileSize);
        set->setUniqueTileCount(totals.uniqueTileCount);
        set->setUniqueTileSize(totals.uniqueTileSize);

        set->blockSignals(false);
        (void) set->moveToThread(QCoreApplication::instance()->thread());
        task->setTileSetFetched(set);
    }
}

// H2: Block signals while modifying the tile set from the worker thread.
// The object is exclusively owned by this task (no concurrent access), but
// emitting signals from the wrong thread is technically incorrect.
void QGCCacheWorker::_createTileSet(QGCMapTask *mtask)
{
    if (!_testTask(mtask)) {
        return;
    }

    QGCCreateTileSetTask *task = static_cast<QGCCreateTileSetTask*>(mtask);
    const auto setID = _database->createTileSet(
        task->tileSet()->name(), task->tileSet()->mapTypeStr(),
        task->tileSet()->topleftLat(), task->tileSet()->topleftLon(),
        task->tileSet()->bottomRightLat(), task->tileSet()->bottomRightLon(),
        task->tileSet()->minZoom(), task->tileSet()->maxZoom(),
        task->tileSet()->type(), task->tileSet()->totalTileCount());

    if (!setID.has_value()) {
        mtask->setError("Error saving tile set");
        return;
    }

    task->tileSet()->blockSignals(true);
    task->tileSet()->setId(setID.value());

    const SetTotalsResult totals = _database->computeSetTotals(
        setID.value(), task->tileSet()->defaultSet(),
        task->tileSet()->totalTileCount(), task->tileSet()->type());
    task->tileSet()->setSavedTileCount(totals.savedTileCount);
    task->tileSet()->setSavedTileSize(totals.savedTileSize);
    task->tileSet()->setTotalTileSize(totals.totalTileSize);
    task->tileSet()->setUniqueTileCount(totals.uniqueTileCount);
    task->tileSet()->setUniqueTileSize(totals.uniqueTileSize);
    task->tileSet()->blockSignals(false);

    task->setTileSetSaved();
}

void QGCCacheWorker::_getTileDownloadList(QGCMapTask *mtask)
{
    if (!_testTask(mtask)) {
        return;
    }

    QGCGetTileDownloadListTask *task = static_cast<QGCGetTileDownloadListTask*>(mtask);
    const QList<QGCTile> tileValues = _database->getTileDownloadList(task->setID(), task->count());
    QQueue<QGCTile*> tiles;
    for (const auto &t : tileValues) {
        tiles.enqueue(new QGCTile(t));
    }
    task->setTileListFetched(tiles);
}

void QGCCacheWorker::_updateTileDownloadState(QGCMapTask *mtask)
{
    if (!_testTask(mtask)) {
        return;
    }

    QGCUpdateTileDownloadStateTask *task = static_cast<QGCUpdateTileDownloadStateTask*>(mtask);
    bool ok;
    if (task->hash() == QStringLiteral("*")) {
        ok = _database->updateAllTileDownloadStates(task->setID(), static_cast<int>(task->state()));
    } else {
        ok = _database->updateTileDownloadState(task->setID(), static_cast<int>(task->state()), task->hash());
    }
    if (!ok) {
        mtask->setError("Error updating tile download state");
    }
}

// M2: Check return value, don't signal success on failure
void QGCCacheWorker::_pruneCache(QGCMapTask *mtask)
{
    if (!_testTask(mtask)) {
        return;
    }

    QGCPruneCacheTask *task = static_cast<QGCPruneCacheTask*>(mtask);
    if (!_database->pruneCache(task->amount())) {
        mtask->setError("Error pruning cache");
        return;
    }
    task->setPruned();
}

// M2: Check return value, don't signal success on failure
void QGCCacheWorker::_deleteTileSet(QGCMapTask *mtask)
{
    if (!_testTask(mtask)) {
        return;
    }

    QGCDeleteTileSetTask *task = static_cast<QGCDeleteTileSetTask*>(mtask);
    if (!_database->deleteTileSet(task->setID())) {
        mtask->setError("Error deleting tile set");
        return;
    }
    _emitTotals();
    task->setTileSetDeleted();
}

void QGCCacheWorker::_renameTileSet(QGCMapTask *mtask)
{
    if (!_testTask(mtask)) {
        return;
    }

    QGCRenameTileSetTask *task = static_cast<QGCRenameTileSetTask*>(mtask);
    if (!_database->renameTileSet(task->setID(), task->newName())) {
        task->setError("Error renaming tile set");
    }
}

// M2: Check return value, don't signal success on failure
void QGCCacheWorker::_resetCacheDatabase(QGCMapTask *mtask)
{
    if (!_testTask(mtask)) {
        return;
    }

    QGCResetTask *task = static_cast<QGCResetTask*>(mtask);
    if (!_database->resetDatabase()) {
        mtask->setError("Error resetting cache database");
        return;
    }
    _dbValid = _database->isValid();
    task->setResetCompleted();
}

// H4: Don't emit completion on failure
void QGCCacheWorker::_importSets(QGCMapTask *mtask)
{
    if (!_testTask(mtask)) {
        return;
    }

    QGCImportTileTask *task = static_cast<QGCImportTileTask*>(mtask);
    auto progress = [task](int pct) { task->setProgress(pct); };

    DatabaseResult result;
    if (task->replace()) {
        result = _database->importSetsReplace(task->path(), progress);
    } else {
        result = _database->importSetsMerge(task->path(), progress);
    }

    _dbValid = _database->isValid();

    if (!result.success) {
        task->setError(result.errorString);
        return;
    }

    if (task->replace() && _database->isValid()) {
        QSettings settings;
        settings.remove(QLatin1String(QGCTileCacheDatabase::kBingNoTileDoneKey));
        _database->deleteBingNoTileTiles();
    }

    task->setImportCompleted();
}

// H3: Records are now snapshotted on the main thread via QGCExportTileTask,
// eliminating cross-thread reads of live QGCCachedTileSet objects.
// H4: Don't emit completion on failure
void QGCCacheWorker::_exportSets(QGCMapTask *mtask)
{
    if (!_testTask(mtask)) {
        return;
    }

    QGCExportTileTask *task = static_cast<QGCExportTileTask*>(mtask);

    auto progress = [task](int pct) { task->setProgress(pct); };
    DatabaseResult result = _database->exportSets(task->sets(), task->path(), progress);

    if (!result.success) {
        task->setError(result.errorString);
        return;
    }

    task->setExportCompleted();
}
