#include "QGCTileCacheWorker.h"

#include "QGCCacheTile.h"
#include "QGCLoggingCategory.h"
#include "QGCMapTasks.h"
#include "QGCTileCacheDatabase.h"

QGC_LOGGING_CATEGORY(QGCTileCacheWorkerLog, "QtLocationPlugin.QGCTileCacheWorker")

QGCCacheWorker::QGCCacheWorker() : QObject(nullptr)
{
    // No QObject parent: the worker is moved to its own thread, which a parented
    // QObject forbids. QGCMapEngine owns and deletes it explicitly.
    moveToThread(&_thread);
    (void) connect(&_thread, &QThread::started, this, &QGCCacheWorker::_init);
    // Cleanup at loop-exit (finished, on the worker thread) — not a queued
    // _shutdown, which app teardown's missing event loop would never deliver.
    (void) connect(&_thread, &QThread::finished, this, &QGCCacheWorker::_shutdown, Qt::DirectConnection);
    qCDebug(QGCTileCacheWorkerLog) << this;
}

QGCCacheWorker::~QGCCacheWorker()
{
    stop();
    (void) _thread.wait();
    qCDebug(QGCTileCacheWorkerLog) << this;
}

void QGCCacheWorker::stop()
{
    _stopRequested = true;

    if (_thread.isRunning()) {
        // quit() directly, not a queued _shutdown: at app teardown QCoreApplication
        // is gone, so a cross-thread posted event never runs and wait() would hang.
        _thread.quit();
        return;
    }

    QMutexLocker lock(&_taskQueueMutex);
    qDeleteAll(_taskQueue);
    _taskQueue.clear();
}

bool QGCCacheWorker::enqueueTask(QGCMapTask* task)
{
    if (!_dbValid && !_thread.isRunning() && (task->type() != QGCMapTask::TaskType::taskInit)) {
        task->setError(tr("Database Not Initialized"));
        task->deleteLater();
        return false;
    }

    QMutexLocker lock(&_taskQueueMutex);
    _taskQueue.enqueue(task);
    lock.unlock();

    if (!_thread.isRunning()) {
        _thread.start(QThread::NormalPriority);
    }

    if (!_drainScheduled.exchange(true)) {
        (void) QMetaObject::invokeMethod(this, &QGCCacheWorker::_drainQueue, Qt::QueuedConnection);
    }

    return true;
}

bool QGCCacheWorker::validateDatabase(QGCMapTask* task)
{
    if (!_database || !_database->isValid()) {
        task->setError("No Cache Database");
        return false;
    }

    return true;
}

void QGCCacheWorker::emitTotals()
{
    TotalsResult t = _database->computeTotals();
    emit updateTotals(t.totalCount, t.totalSize, t.defaultCount, t.defaultSize);
}

void QGCCacheWorker::_saveTileBatch(const QList<QGCMapTask*>& tasks)
{
    if (tasks.isEmpty()) {
        return;
    }

    if (!_database || !_database->isValid()) {
        for (QGCMapTask* mtask : tasks) {
            mtask->setError("No Cache Database");
        }
        return;
    }

    QList<const QGCCacheTile*> tiles;
    tiles.reserve(tasks.size());
    for (QGCMapTask* mtask : tasks) {
        tiles.append(static_cast<QGCSaveTileTask*>(mtask)->tile());
    }

    if (!_database->saveTileBatch(tiles)) {
        for (QGCMapTask* mtask : tasks) {
            mtask->setError("Error saving tile to cache");
        }
    }
}

void QGCCacheWorker::_init()
{
    _stopRequested = false;
    // A _drainQueue invocation queued before the previous stop() may have been
    // discarded when the thread's event loop exited, leaving _drainScheduled
    // stuck true; clear it so enqueueTask re-posts a drain after restart.
    _drainScheduled = false;
    _database = std::make_unique<QGCTileCacheDatabase>(_databasePath);

    if (!_database->init()) {
        qCWarning(QGCTileCacheWorkerLog) << "Failed To Init Database";
        _database.reset();

        QMutexLocker lock(&_taskQueueMutex);
        for (QGCMapTask* orphan : _taskQueue) {
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

    if (!_updateTimer) {
        _updateTimer = new QTimer(this);
        _updateTimer->setInterval(kUpdateTimerIntervalMs);
        (void) connect(_updateTimer, &QTimer::timeout, this, [this]() {
            if (_database && _database->isValid()) {
                emitTotals();
            }
        });
    }
    _updateTimer->start();
}

void QGCCacheWorker::_drainQueue()
{
    _drainScheduled = false;

    if (_stopRequested) {
        return;
    }

    QMutexLocker lock(&_taskQueueMutex);
    while (!_stopRequested && !_taskQueue.isEmpty()) {
        QGCMapTask* const task = _taskQueue.dequeue();
        if (task->type() == QGCMapTask::TaskType::taskCacheTile) {
            QList<QGCMapTask*> batch;
            batch.append(task);
            while ((batch.size() < kMaxSaveBatch) && !_taskQueue.isEmpty() &&
                   (_taskQueue.head()->type() == QGCMapTask::TaskType::taskCacheTile)) {
                batch.append(_taskQueue.dequeue());
            }
            lock.unlock();
            _saveTileBatch(batch);
            for (QGCMapTask* batched : batch) {
                batched->deleteLater();
            }
            lock.relock();
        } else {
            lock.unlock();
            task->execute(*this);
            task->deleteLater();
            lock.relock();
        }
    }
    lock.unlock();

    if (!_stopRequested && _database && _database->isValid()) {
        emitTotals();
    }
}

void QGCCacheWorker::_shutdown()
{
    if (_updateTimer) {
        _updateTimer->stop();
    }

    QMutexLocker lock(&_taskQueueMutex);
    for (QGCMapTask* orphan : _taskQueue) {
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
