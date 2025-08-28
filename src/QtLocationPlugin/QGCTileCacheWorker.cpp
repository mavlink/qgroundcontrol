/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCTileCacheWorker.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QSettings>

#include "QGCCachedTileSet.h"
#include "QGCLoggingCategory.h"
#include "QGCMapTasks.h"
#include "QGCMapUrlEngine.h"
#include "QGCTileCacheDatabase.h"

QGC_LOGGING_CATEGORY(QGCTileCacheWorkerLog, "QtLocationPlugin.QGCTileCacheWorker")

using namespace Qt::StringLiterals;


QGCCacheWorker::QGCCacheWorker(QObject *parent)
    : QThread(parent)
{
    qCDebug(QGCTileCacheWorkerLog) << this;
}

QGCCacheWorker::~QGCCacheWorker()
{
    requestInterruption();
    _waitc.wakeAll();
    if (!wait(5000)) {
        qCWarning(QGCTileCacheWorkerLog) << "Worker thread failed to stop cleanly";
    }

    QMutexLocker lock(&_taskQueueMutex);
    qDeleteAll(_taskQueue);
    _taskQueue.clear();

    qCDebug(QGCTileCacheWorkerLog) << this;
}

void QGCCacheWorker::stop()
{
    _running.store(false);

    QMutexLocker lock(&_taskQueueMutex);
    qDeleteAll(_taskQueue);
    _taskQueue.clear();
    lock.unlock();

    if (isRunning()) {
        _waitc.wakeAll();
    }
}

int QGCCacheWorker::pendingTaskCount() const
{
    QMutexLocker lock(&_taskQueueMutex);
    return _taskQueue.count();
}

bool QGCCacheWorker::enqueueTask(QGCMapTask *task)
{
    if (!task) {
        return false;
    }

    if (!isValid() && (task->type() != QGCMapTask::TaskType::taskInit)) {
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
    if (!isValid() && !hasFailed()) {
        if (!_init()) {
            _failed.store(true);
            qCWarning(QGCTileCacheWorkerLog) << "Failed To Init Database";
            return;
        }
        _valid.store(true);
    }

    if (isValid()) {
        _deleteBingNoTileTiles();
    }

    if (!_updateTimer.isValid()) {
        _updateTimer.start();
    }

    QMutexLocker lock(&_taskQueueMutex);
    while (_running.load()) {
        if (!_taskQueue.isEmpty()) {
            QGCMapTask *task = _taskQueue.dequeue();
            lock.unlock();

            _runTask(task);
            task->deleteLater();

            lock.relock();

            const qsizetype count = _taskQueue.count();
            if (count > 100) {
                _updateTimeout = kLongTimeout;
            } else if (count < 25) {
                _updateTimeout = kShortTimeout;
            }

            if ((count == 0) || _updateTimer.hasExpired(_updateTimeout)) {
                if (isValid()) {
                    lock.unlock();
                    _updateTotals();
                    lock.relock();
                }
            }
        } else {
            (void) _waitc.wait(lock.mutex(), kLongTimeout);
        }
    }

    // ensure DB is closed on the owning thread
    if (_database) {
        lock.unlock();
        _database->close();
        lock.relock();
    }
}

void QGCCacheWorker::_runTask(QGCMapTask *task)
{
    if (!task) {
        return;
    }

    switch (task->type()) {
    case QGCMapTask::TaskType::taskInit:
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
        qCWarning(QGCTileCacheWorkerLog) << "unhandled task type:" << static_cast<int>(task->type());
        break;
    }
}

bool QGCCacheWorker::_init()
{
    if (_databasePath.isEmpty()) {
        qCCritical(QGCTileCacheWorkerLog) << "Could not find suitable cache directory.";
        return false;
    }

    qCDebug(QGCTileCacheWorkerLog) << "Mapping cache directory:" << _databasePath;

    // Create database only when needed (lazy initialization)
    if (!_database) {
        _database = std::make_unique<QGCTileCacheDatabase>(QString(kConnectionName), _databasePath);
    }

    if (!_database->open()) {
        qCCritical(QGCTileCacheWorkerLog) << "Failed to open database:" << _database->lastError();
        return false;
    }

    if (!_database->createSchema(true)) {
        qCCritical(QGCTileCacheWorkerLog) << "Failed to create database schema:" << _database->lastError();
        return false;
    }

    // Optimize database for better performance
    (void) _database->analyze();

    return true;
}

bool QGCCacheWorker::_testTask(QGCMapTask *mtask) const
{
    if (!mtask) {
        return false;
    }

    if (!isValid()) {
        mtask->setError(tr("No Cache Database"));
        return false;
    }

    if (!_database || !_database->isOpen()) {
        mtask->setError(tr("Database not open"));
        return false;
    }

    return true;
}

void QGCCacheWorker::_deleteBingNoTileTiles()
{
    static const QString alreadyDoneKey = u"_deleteBingNoTileTilesDone"_s;

    QSettings settings;
    if (settings.value(alreadyDoneKey, false).toBool()) {
        return;
    }
    settings.setValue(alreadyDoneKey, true);

    QFile file(kBingNoTileBytesPath);
    if (!file.open(QFile::ReadOnly)) {
        qCWarning(QGCTileCacheWorkerLog) << "Failed to Open File" << file.fileName() << ":" << file.errorString();
        return;
    }

    const QByteArray noTileBytes = file.readAll();
    file.close();

    if (!_database->deleteTilesMatchingBytes(noTileBytes)) {
        qCWarning(QGCTileCacheWorkerLog) << "Failed to deleteTilesMatchingBytes";
    }
}

void QGCCacheWorker::_updateTotals()
{
    DatabaseTotals totals{};
    if (_database->getTotals(totals)) {
        QMutexLocker lock(&_statsMutex);
        _totalCount = totals.totalCount;
        _totalSize = totals.totalSize;
        _defaultCount = totals.defaultCount;
        _defaultSize = totals.defaultSize;
        lock.unlock();

        emit updateTotals(_totalCount, _totalSize, _defaultCount, _defaultSize);
    }

    (void) _updateTimer.restart();
}

void QGCCacheWorker::_updateSetTotals(QGCCachedTileSet *set)
{
    if (!set) {
        return;
    }

    if (set->defaultSet()) {
        _updateTotals();

        QMutexLocker lock(&_statsMutex);
        set->setSavedTileCount(_totalCount);
        set->setSavedTileSize(_totalSize);
        set->setTotalTileCount(_defaultCount);
        set->setTotalTileSize(_defaultSize);
        return;
    }

    // Use the new optimized statistics function
    DatabaseStatistics stats{};
    if (!_database->getSetStatistics(set->id(), stats)) {
        return;
    }

    set->setSavedTileCount(stats.savedCount);
    set->setSavedTileSize(stats.savedSize);

    // Update estimated size
    quint64 avg = UrlFactory::averageSizeForType(set->type());
    if (set->totalTileCount() <= set->savedTileCount()) {
        set->setTotalTileSize(set->savedTileSize());
    } else {
        if ((set->savedTileCount() > 10) && set->savedTileSize()) {
            avg = set->savedTileSize() / set->savedTileCount();
        }
        set->setTotalTileSize(avg * set->totalTileCount());
    }

    const quint32 expectedUcount = set->totalTileCount() - set->savedTileCount();
    if (stats.uniqueCount == 0) {
        set->setUniqueTileSize(expectedUcount * avg);
        set->setUniqueTileCount(expectedUcount);
    } else {
        set->setUniqueTileCount(stats.uniqueCount);
        set->setUniqueTileSize(stats.uniqueSize);
    }
}

void QGCCacheWorker::_saveTile(QGCMapTask *mtask)
{
    if (!_testTask(mtask)) {
        return;
    }

    const QGCSaveTileTask *task = static_cast<QGCSaveTileTask*>(mtask);
    if (!task) {
        mtask->setError(tr("Invalid task"));
        return;
    }

    const QGCCacheTile *cacheTile = task->tile();
    if (!cacheTile) {
        mtask->setError(tr("Invalid tile data"));
        return;
    }

    TileInfo tile{};
    tile.hash = cacheTile->hash;
    tile.format = cacheTile->format;
    tile.tile = cacheTile->img;
    tile.size = cacheTile->img.size();
    // store Qt map id as int
    tile.type = UrlFactory::getQtMapIdFromProviderType(cacheTile->type);
    tile.date = QDateTime::currentSecsSinceEpoch();

    const quint64 setID = (cacheTile->tileSet == UINT64_MAX) ? _database->getDefaultTileSet() : cacheTile->tileSet;
    if (!_database->saveTile(tile, setID)) {
        // Tile might already exist - not necessarily an error
        qCDebug(QGCTileCacheWorkerLog) << "Tile already exists or save failed for HASH:" << tile.hash;
    } else {
        qCDebug(QGCTileCacheWorkerLog) << "Saved tile HASH:" << tile.hash;
    }
}

void QGCCacheWorker::_getTile(QGCMapTask* mtask)
{
    if (!_testTask(mtask)) {
        return;
    }

    QGCFetchTileTask *task = static_cast<QGCFetchTileTask*>(mtask);
    if (!task) {
        return;
    }

    TileInfo tile{};
    const QString hash = task->hash();
    if (!_database->getTile(hash, tile)) {
        qCDebug(QGCTileCacheWorkerLog) << "Tile not in DB:" << hash;
        task->setError(tr("Tile not in cache database"));
        return;
    }

    qCDebug(QGCTileCacheWorkerLog) << "Found tile in DB:" << hash;

    // convert back to provider type string for QGCCacheTile
    auto cacheTile = std::make_unique<QGCCacheTile>(
        hash, tile.tile, tile.format,
        UrlFactory::getProviderTypeFromQtMapId(tile.type)
    );
    task->setTileFetched(cacheTile.release());
}

void QGCCacheWorker::_getTileSets(QGCMapTask *mtask)
{
    if (!_testTask(mtask)) {
        return;
    }

    QGCFetchTileSetTask *task = static_cast<QGCFetchTileSetTask*>(mtask);
    if (!task) {
        return;
    }

    QList<TileSetInfo> sets;
    if (!_database->getTileSets(sets)) {
        task->setError(tr("Failed to fetch tile sets"));
        return;
    }

    for (const TileSetInfo &info : std::as_const(sets)) {
        auto set = std::make_unique<QGCCachedTileSet>(info.name);
        set->setId(info.setID);
        set->setMapTypeStr(info.typeStr);
        set->setTopleftLat(info.topleftLat);
        set->setTopleftLon(info.topleftLon);
        set->setBottomRightLat(info.bottomRightLat);
        set->setBottomRightLon(info.bottomRightLon);
        set->setMinZoom(info.minZoom);
        set->setMaxZoom(info.maxZoom);
        set->setType(UrlFactory::getProviderTypeFromQtMapId(info.type));
        set->setTotalTileCount(info.numTiles);
        set->setDefaultSet(info.defaultSet);
        set->setCreationDate(QDateTime::fromSecsSinceEpoch(info.date));

        _updateSetTotals(set.get());

        (void) set->moveToThread(QCoreApplication::instance()->thread());
        task->setTileSetFetched(set.release());
    }
}

void QGCCacheWorker::_createTileSet(QGCMapTask *mtask)
{
    if (!_testTask(mtask)) {
        return;
    }

    QGCCreateTileSetTask *task = static_cast<QGCCreateTileSetTask*>(mtask);
    if (!task) {
        return;
    }

    QGCCachedTileSet *tileSet = task->tileSet();
    if (!tileSet) {
        mtask->setError(tr("Invalid tile set"));
        return;
    }

    // Create tile set info
    TileSetInfo info{};
    info.name = tileSet->name();
    info.typeStr = tileSet->mapTypeStr();
    info.topleftLat = tileSet->topleftLat();
    info.topleftLon = tileSet->topleftLon();
    info.bottomRightLat = tileSet->bottomRightLat();
    info.bottomRightLon = tileSet->bottomRightLon();
    info.minZoom = tileSet->minZoom();
    info.maxZoom = tileSet->maxZoom();
    info.type = UrlFactory::getQtMapIdFromProviderType(tileSet->type());
    info.numTiles = tileSet->totalTileCount();
    info.date = QDateTime::currentSecsSinceEpoch();

    quint64 setID;
    if (!_database->createTileSet(info, setID)) {
        mtask->setError(tr("Error creating tile set"));
        return;
    }

    tileSet->setId(setID);

    // Prepare download list using batch operations
    QList<TileDownloadInfo> downloadQueue;

    for (int z = tileSet->minZoom(); z <= tileSet->maxZoom(); z++) {
        const QGCTileSet set = UrlFactory::getTileCount(z,
            tileSet->topleftLon(), tileSet->topleftLat(),
            tileSet->bottomRightLon(), tileSet->bottomRightLat(),
            tileSet->type());

        for (int x = set.tileX0; x <= set.tileX1; x++) {
            for (int y = set.tileY0; y <= set.tileY1; y++) {
                const QString type = tileSet->type();
                const QString hash = UrlFactory::getTileHash(type, x, y, z);
                const quint64 tileID = _database->getTileID(hash);

                if (tileID == 0) {
                    // Add to download queue
                    TileDownloadInfo dlInfo{};
                    dlInfo.hash = hash;
                    dlInfo.type = UrlFactory::getQtMapIdFromProviderType(type);
                    dlInfo.x = x;
                    dlInfo.y = y;
                    dlInfo.z = z;
                    dlInfo.state = 0;

                    downloadQueue.append(dlInfo);
                } else {
                    // Tile exists, add to set
                    if (!_database->addTileToSet(tileID, setID)) {
                        qCWarning(QGCTileCacheWorkerLog) << "Failed to add existing tile to set";
                    }
                    qCDebug(QGCTileCacheWorkerLog) << "Already Cached:" << hash;
                }
            }
        }
    }

    // Use batch operation for download queue
    if (!downloadQueue.isEmpty()) {
        if (!_database->addBatchToDownloadQueue(setID, downloadQueue)) {
            qCWarning(QGCTileCacheWorkerLog) << "Failed to add tiles to download queue";
        }
    }

    _updateSetTotals(tileSet);
    task->setTileSetSaved();
}

void QGCCacheWorker::_getTileDownloadList(QGCMapTask *mtask)
{
    if (!_testTask(mtask)) {
        return;
    }

    QGCGetTileDownloadListTask *task = static_cast<QGCGetTileDownloadListTask*>(mtask);
    if (!task) {
        return;
    }

    QList<TileDownloadInfo> downloadList;
    if (!_database->getDownloadList(task->setID(), task->count(), downloadList)) {
        task->setError(tr("Failed to get download list"));
        return;
    }

    QQueue<QGCTile*> tiles;
    for (const TileDownloadInfo &info : std::as_const(downloadList)) {
        auto tile = std::make_unique<QGCTile>();
        tile->hash = info.hash;
        tile->type = UrlFactory::getProviderTypeFromQtMapId(info.type);
        tile->x = info.x;
        tile->y = info.y;
        tile->z = info.z;
        tiles.enqueue(tile.release());

        if (!_database->updateDownloadState(task->setID(), info.hash, static_cast<int>(QGCTile::StateDownloading))) {
            qCWarning(QGCTileCacheWorkerLog) << "Failed to update download state";
        }
    }

    task->setTileListFetched(tiles);
}

void QGCCacheWorker::_updateTileDownloadState(QGCMapTask *mtask)
{
    if (!_testTask(mtask)) {
        return;
    }

    QGCUpdateTileDownloadStateTask *task = static_cast<QGCUpdateTileDownloadStateTask*>(mtask);
    if (!task) {
        return;
    }

    if (!_database->updateDownloadState(task->setID(), task->hash(), static_cast<int>(task->state()))) {
        task->setError(tr("Failed to update download state"));
    }
}

void QGCCacheWorker::_deleteTileSet(QGCMapTask *mtask)
{
    if (!_testTask(mtask)) {
        return;
    }

    QGCDeleteTileSetTask *task = static_cast<QGCDeleteTileSetTask*>(mtask);
    if (!task) {
        return;
    }

    if (!_database->deleteTileSet(task->setID())) {
        task->setError(tr("Failed to delete tile set"));
        return;
    }

    _updateTotals();
    task->setTileSetDeleted();
}

void QGCCacheWorker::_renameTileSet(QGCMapTask *mtask)
{
    if (!_testTask(mtask)) {
        return;
    }

    QGCRenameTileSetTask *task = static_cast<QGCRenameTileSetTask*>(mtask);
    if (!task) {
        return;
    }

    if (!_database->renameTileSet(task->setID(), task->newName())) {
        task->setError(tr("Error renaming tile set"));
    }
}

void QGCCacheWorker::_pruneCache(QGCMapTask *mtask)
{
    if (!_testTask(mtask)) {
        return;
    }

    QGCPruneCacheTask *task = static_cast<QGCPruneCacheTask*>(mtask);
    if (!task) {
        return;
    }

    qint64 pruned = 0;

    // Use the new LRU pruning if no specific set is specified
    bool result = false;
    if (task->amount() > 0) {
        result = _database->pruneLRU(task->amount(), pruned);
    } else {
        // Prune specific set
        result = _database->pruneCache(_database->getDefaultTileSet(), task->amount(), pruned);
    }

    if (!result) {
        task->setError(tr("Failed to prune cache"));
        return;
    }

    task->setPruned();

    // Run vacuum after pruning to reclaim space
    (void) _database->vacuum();
}

void QGCCacheWorker::_resetCacheDatabase(QGCMapTask *mtask)
{
    if (!_testTask(mtask)) {
        return;
    }

    QGCResetTask *task = static_cast<QGCResetTask*>(mtask);
    if (!task) {
        return;
    }

    _valid.store(_database->reset());
    task->setResetCompleted();
}

void QGCCacheWorker::_exportSets(QGCMapTask *mtask)
{
    if (!_testTask(mtask)) {
        return;
    }

    QGCExportTileTask *task = static_cast<QGCExportTileTask*>(mtask);
    if (!task) {
        return;
    }

    const auto progressCallback = [task](int progress) {
        if (task) {
            task->setProgress(progress);
        }
    };

    if (!_database->exportDatabase(task->path(), task->setIDs(), progressCallback)) {
        task->setError(_database->lastError());
        return;
    }

    task->setExportCompleted();
}

void QGCCacheWorker::_importSets(QGCMapTask *mtask)
{
    if (!_testTask(mtask)) {
        return;
    }

    QGCImportTileTask *task = static_cast<QGCImportTileTask*>(mtask);
    if (!task) {
        return;
    }

    const auto progressCallback = [task](int progress) {
        if (task) {
            task->setProgress(progress);
        }
    };

    if (!_database->importDatabase(task->path(), task->replace(), progressCallback)) {
        task->setError(_database->lastError());
        return;
    }

    if (task->replace()) {
        _valid.store(true);
    }

    task->setImportCompleted();
}
