#include "QGCCacheWorker.h"

#include "QGCCacheTile.h"
#include "QGCLoggingCategory.h"
#include "QGCMapTask.h"
#include "QGCTileCacheDatabase.h"

#ifdef QGC_UNITTEST_BUILD
#include "AppMessages.h"
#endif

QGC_LOGGING_CATEGORY(QGCCacheWorkerLog, "MapTiles.QGCCacheWorker")

#ifdef QGC_UNITTEST_BUILD
std::function<QGCCacheTile*(const QString&)> QGCCacheWorker::_unitTestTileGenerator;

void QGCCacheWorker::setUnitTestTileGenerator(std::function<QGCCacheTile*(const QString& hash)> generator)
{
    _unitTestTileGenerator = std::move(generator);
}
#endif

QGCCacheWorker::QGCCacheWorker(QObject* parent)
    : QThread(parent)
{
    qCDebug(QGCCacheWorkerLog) << this;
}

QGCCacheWorker::~QGCCacheWorker()
{
    stop();
    wait();
    qCDebug(QGCCacheWorkerLog) << this;
}

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

bool QGCCacheWorker::enqueueTask(QGCMapTask* task)
{
    // Only a task without work, which is there to start the worker, can be queued before the cache is open
    if (!_dbValid && !isRunning() && task->work()) {
        task->finish(tr("Database Not Initialized"));
        task->deleteLater();
        return false;
    }

    QMutexLocker lock(&_taskQueueMutex);
    _taskQueue.enqueue(task);
    lock.unlock();

    if (isRunning()) {
        _waitc.wakeAll();
    } else {
        // Reset before starting: clearing it in run() could discard a stop() issued before the thread runs
        _stopRequested = false;
        start(QThread::NormalPriority);
    }

    return true;
}

void QGCCacheWorker::run()
{
    _database = std::make_unique<QGCTileCacheDatabase>(_databasePath);
    _database->setCacheVariants(_cacheVariants);

    if (!_database->init()) {
        qCWarning(QGCCacheWorkerLog) << "Failed To Init Database";
        _database.reset();

        QMutexLocker lock(&_taskQueueMutex);
        for (QGCMapTask* orphan : _taskQueue) {
            orphan->finish(tr("Database Init Failed"));
            orphan->deleteLater();
        }
        _taskQueue.clear();
        return;
    }

    if (_database->isValid()) {
        (void) _database->connectDB();
    }

    _dbValid = _database->isValid();
    // An earlier run can have quit with pages left to release, and init() can have reset the cache
    _releasePending = true;

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
                    emitTotals();
                    lock.relock();
                }
            }
        } else if (_releasePending) {
            // While there is nothing else to do, and a batch at a time: a task queued meanwhile waits for one batch
            lock.unlock();
            _releasePending =
                _database && _database->isValid() && (_database->releaseFreePages(kReleaseBatchPages) > 0);
            lock.relock();
        } else {
            (void) _waitc.wait(lock.mutex(), 5000);
        }
    }

    for (QGCMapTask* orphan : _taskQueue) {
        orphan->finish(tr("Worker shutting down"));
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

void QGCCacheWorker::_runTask(QGCMapTask* task)
{
    QString error;
    if (task->work()) {
        error = _database->isValid() ? task->work()(*this, *task) : QStringLiteral("No Cache Database");
    }
    // A reset and an import rebuild the cache
    _dbValid = _database->isValid();
    task->finish(error);
}

void QGCCacheWorker::emitTotals()
{
    const TotalsResult totals = _database->computeTotals();
    emit updateTotals(totals.totalCount, totals.totalSize, totals.defaultCount, totals.defaultSize);
    _updateTimer.restart();
}

std::unique_ptr<QGCCacheTile> QGCCacheWorker::tile(const QString& hash)
{
    std::unique_ptr<QGCCacheTile> tile = _database->getTile(hash);

    // Under unit tests a cache miss consults the synthetic tile generator instead of
    // erroring, so tile consumers never fall back to real network fetches (late
    // replies race UI teardown and crash). See UnitTestTileGenerator.
#ifdef QGC_UNITTEST_BUILD
    if (!tile && QGC::runningUnitTests() && _unitTestTileGenerator) {
        tile.reset(_unitTestTileGenerator(hash));
    }
#endif

    return tile;
}
