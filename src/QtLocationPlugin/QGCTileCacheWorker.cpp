#include "QGCTileCacheWorker.h"
#include "QGCLoggingCategory.h"
#include "QGCMapTasks.h"
#include "QGCTileCacheDatabase.h"

#include <QtCore/QCoreApplication>

QGC_LOGGING_CATEGORY(QGCTileCacheWorkerLog, "qgc.qtlocationplugin.qgctilecacheworker")

QGCTileCacheWorker::QGCTileCacheWorker(QObject *parent)
    : QThread(parent)
{
    // qCDebug(QGCTileCacheWorkerLog) << Q_FUNC_INFO << this;
}

QGCTileCacheWorker::~QGCTileCacheWorker()
{
    stop();
    // qCDebug(QGCTileCacheWorkerLog) << Q_FUNC_INFO << this;
}

bool QGCTileCacheWorker::enqueueTask(QGCMapTask *task)
{
    QMutexLocker lock(&_taskQueueMutex);
    _taskQueue.enqueue(task);
    lock.unlock();
    _waitc.wakeAll();
    if (!isRunning()) {
        start(QThread::HighPriority);
    }
    return true;
}

void QGCTileCacheWorker::stop()
{
    const QMutexLocker lock(&_taskQueueMutex);
    qDeleteAll(_taskQueue);
    _taskQueue.clear();
    _waitc.wakeAll();
}

void QGCTileCacheWorker::run()
{
    _dbManager = new QGCTileCacheDatabase(_databasePath);
    if (!_dbManager->connect() || !_dbManager->createDatabase()) {
        qCWarning(QGCTileCacheWorkerLog) << "QGCTileCacheWorker: Failed to initialize database";
        return;
    }
    _dbManager->deleteBingNoTileTiles();

    while (true) {
        _taskQueueMutex.lock();
        if (!_taskQueue.isEmpty()) {
            QGCMapTask* task = _taskQueue.dequeue();
            _taskQueueMutex.unlock();
            _processTask(task);
            task->deleteLater();
        } else {
            if (!_waitc.wait(&_taskQueueMutex, 5000)) {
                _taskQueueMutex.unlock();
                break;
            }
            _taskQueueMutex.unlock();
        }
    }
    _dbManager->disconnect();
    delete _dbManager;
    _dbManager = nullptr;
}

void QGCTileCacheWorker::_processTask(QGCMapTask *task) const
{
    switch (task->type()) {
    case QGCMapTask::taskCacheTile:
        _handleSaveTileTask(task);
        break;
    case QGCMapTask::taskFetchTile:
        _handleFetchTileTask(task);
        break;
    case QGCMapTask::taskFetchTileSets:
        _handleFetchTileSetsTask(task);
        break;
    case QGCMapTask::taskCreateTileSet:
        _handleCreateTileSetTask(task);
        break;
    case QGCMapTask::taskGetTileDownloadList:
        _handleGetTileDownloadListTask(task);
        break;
    case QGCMapTask::taskUpdateTileDownloadState:
        _handleUpdateTileDownloadStateTask(task);
        break;
    case QGCMapTask::taskPruneCache:
        _handlePruneCacheTask(task);
        break;
    case QGCMapTask::taskDeleteTileSet:
        _handleDeleteTileSetTask(task);
        break;
    case QGCMapTask::taskRenameTileSet:
        _handleRenameTileSetTask(task);
        break;
    case QGCMapTask::taskReset:
        _handleResetCacheDatabaseTask(task);
        break;
    case QGCMapTask::taskExport:
        _handleExportSetsTask(task);
        break;
    case QGCMapTask::taskImport:
        _handleImportSetsTask(task);
        break;
    default:
        qCWarning(QGCTileCacheWorkerLog) << "QGCTileCacheWorker: Unhandled task type:" << task->type();
        break;
    }
}

void QGCTileCacheWorker::_handleSaveTileTask(QGCMapTask *task) const
{
    QGCSaveTileTask *const saveTask = static_cast<QGCSaveTileTask*>(task);
    quint64 tileID = 0;
    const quint64 tileSetID = saveTask->tile()->tileSet();
    if (_dbManager->saveTile(saveTask->tile(), tileID, tileSetID)) {
        qCDebug(QGCTileCacheWorkerLog) << "handleSaveTileTask: Tile saved with ID:" << tileID;
    } else {
        qCWarning(QGCTileCacheWorkerLog) << "handleSaveTileTask: Error saving tile";
    }
}

void QGCTileCacheWorker::_handleFetchTileTask(QGCMapTask *task) const
{
    QGCFetchTileTask *const fetchTask = static_cast<QGCFetchTileTask*>(task);
    QGCCacheTile *const tile = _dbManager->fetchTile(fetchTask->hash());
    if (tile) {
        fetchTask->setTileFetched(tile);
    } else {
        fetchTask->setError(QStringLiteral("Tile not in cache database"));
    }
}

void QGCTileCacheWorker::_handleFetchTileSetsTask(QGCMapTask *task) const
{
    QGCFetchTileSetTask *const fetchSetTask = static_cast<QGCFetchTileSetTask*>(task);
    const QList<QGCCachedTileSet*> sets = _dbManager->fetchTileSets();
    if (sets.isEmpty()) {
        fetchSetTask->setError(QStringLiteral("No tile set in database"));
    } else {
        for (auto set : sets) {
            fetchSetTask->tileSetFetched(set);
        }
    }
}

void QGCTileCacheWorker::_handleCreateTileSetTask(QGCMapTask *task) const
{
    QGCCreateTileSetTask *const createTask = static_cast<QGCCreateTileSetTask*>(task);
    if (_dbManager->createTileSet(createTask->tileSet())) {
        createTask->setTileSetSaved();
    } else {
        createTask->setError(QStringLiteral("Error saving tile set"));
    }
}

void QGCTileCacheWorker::_handleGetTileDownloadListTask(QGCMapTask *task) const
{
    QGCGetTileDownloadListTask *const getTask = static_cast<QGCGetTileDownloadListTask*>(task);
    const QList<QGCTile*> list = _dbManager->getTileDownloadList(getTask->setID(), getTask->count());
    getTask->setTileListFetched(list);
}

void QGCTileCacheWorker::_handleUpdateTileDownloadStateTask(QGCMapTask *task) const
{
    QGCUpdateTileDownloadStateTask *const updateTask = static_cast<QGCUpdateTileDownloadStateTask*>(task);
    if (!_dbManager->updateTileDownloadState(updateTask->setID(), updateTask->hash(), updateTask->state())) {
        qCWarning(QGCTileCacheWorkerLog) << "handleUpdateTileDownloadStateTask: Failed to update state";
    }
}

void QGCTileCacheWorker::_handlePruneCacheTask(QGCMapTask *task) const
{
    QGCPruneCacheTask *const pruneTask = static_cast<QGCPruneCacheTask*>(task);
    if (_dbManager->pruneCache(pruneTask->amount())) {
        pruneTask->setPruned();
    }
}

void QGCTileCacheWorker::_handleDeleteTileSetTask(QGCMapTask *task) const
{
    QGCDeleteTileSetTask *const deleteTask = static_cast<QGCDeleteTileSetTask*>(task);
    _dbManager->deleteTileSet(deleteTask->setID());
    deleteTask->setTileSetDeleted();
}

void QGCTileCacheWorker::_handleRenameTileSetTask(QGCMapTask *task) const
{
    QGCRenameTileSetTask *const renameTask = static_cast<QGCRenameTileSetTask*>(task);
    if (!_dbManager->renameTileSet(renameTask->setID(), renameTask->newName())) {
        renameTask->setError(QStringLiteral("Error renaming tile set"));
    }
}

void QGCTileCacheWorker::_handleResetCacheDatabaseTask(QGCMapTask *task) const
{
    QGCResetTask *const resetTask = static_cast<QGCResetTask*>(task);
    if (_dbManager->resetCacheDatabase()) {
        resetTask->setResetCompleted();
    } else {
        resetTask->setError(QStringLiteral("Error resetting cache database"));
    }
}

void QGCTileCacheWorker::_handleImportSetsTask(QGCMapTask *task) const
{
    QGCImportTileTask *const importTask = static_cast<QGCImportTileTask*>(task);
    if (_dbManager->importSets(importTask->path(), importTask->replace())) {
        importTask->setProgress(100);
        importTask->setImportCompleted();
    } else {
        importTask->setError(QStringLiteral("Error importing tile sets"));
    }
}

void QGCTileCacheWorker::_handleExportSetsTask(QGCMapTask *task) const
{
    QGCExportTileTask *const exportTask = static_cast<QGCExportTileTask*>(task);
    if (_dbManager->exportSets(exportTask->path(), exportTask->sets())) {
        exportTask->setExportCompleted();
    } else {
        exportTask->setError(QStringLiteral("Error exporting tile sets"));
    }
}
