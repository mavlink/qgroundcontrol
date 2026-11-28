#include "CacheWorker.h"

#include <utility>

#include <QtCore/QCoreApplication>
#include <QtCore/QDeadlineTimer>

#include "CacheTask.h"
#include "CacheTile.h"
#include "QGCLoggingCategory.h"
#include "TileCacheDatabase.h"

#ifdef QGC_UNITTEST_BUILD
#include "MapTilesApp.h"
#endif

QGC_LOGGING_CATEGORY(CacheWorkerLog, "MapTiles.CacheWorker")

// The messages for the user keep the translation context of the name this class had, QGCCacheWorker, so that their
// existing translations still apply: QCoreApplication::translate() in place of tr()

#ifdef QGC_UNITTEST_BUILD
std::function<CacheTile*(const QString&)> CacheWorker::_unitTestTileGenerator;

void CacheWorker::setUnitTestTileGenerator(std::function<CacheTile*(const QString& hash)> generator)
{
    _unitTestTileGenerator = std::move(generator);
}
#endif

CacheWorker::CacheWorker(QObject* parent)
    : QThread(parent)
{
    qCDebug(CacheWorkerLog) << this;
}

CacheWorker::~CacheWorker()
{
    stop();
    wait();
    qCDebug(CacheWorkerLog) << this;
}

void CacheWorker::stop()
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

bool CacheWorker::enqueueTask(CacheTask* task)
{
    QMutexLocker lock(&_taskQueueMutex);
    if (_takingTasks) {
        _taskQueue.enqueue(task);
        lock.unlock();
        _waitc.wakeAll();
        return true;
    }

    // A thread that still runs has ended its last tasks and is about to quit. Only a task without work, which is there
    // to start the worker, can be queued before the cache is open.
    QString refusal;
    if (isRunning()) {
        refusal = QCoreApplication::translate("QGCCacheWorker", "Worker shutting down");
    } else if (!_dbValid && task->work()) {
        refusal = QCoreApplication::translate("QGCCacheWorker", "Database Not Initialized");
    }
    if (!refusal.isEmpty()) {
        // Unlocked: a handler of finished() can queue another task
        lock.unlock();
        task->finish(refusal);
        task->deleteLater();
        return false;
    }

    _taskQueue.enqueue(task);
    _takingTasks = true;
    // Reset before starting: clearing it in run() could discard a stop() issued before the thread runs
    _stopRequested = false;
    start(QThread::NormalPriority);
    return true;
}

void CacheWorker::run()
{
    _database = std::make_unique<TileCacheDatabase>(_databasePath);
    _database->setCacheVariants(_cacheVariants);

    if (!_database->init()) {
        qCWarning(CacheWorkerLog) << "Failed To Init Database";
        _database.reset();
        _endQueuedTasks(QCoreApplication::translate("QGCCacheWorker", "Database Init Failed"));
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
            CacheTask* const task = _taskQueue.dequeue();
            lock.unlock();
            _runTask(task);
            lock.relock();
            task->deleteLater();

            const qsizetype count = _taskQueue.size();
            if (count > 100) {
                _updateInterval = kLongUpdateInterval;
            } else if (count < 25) {
                _updateInterval = kShortUpdateInterval;
            }

            if ((count == 0) || (_updateTimer.durationElapsed() >= _updateInterval)) {
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
            (void) _waitc.wait(lock.mutex(), QDeadlineTimer(kIdleWait));
        }
    }

    lock.unlock();
    _endQueuedTasks(QCoreApplication::translate("QGCCacheWorker", "Worker shutting down"));

    _dbValid = false;
    if (_database) {
        _database->disconnectDB();
        _database.reset();
    }
}

void CacheWorker::_endQueuedTasks(const QString& error)
{
    QMutexLocker lock(&_taskQueueMutex);
    _takingTasks = false;
    const QQueue<CacheTask*> orphans = std::exchange(_taskQueue, {});
    // Unlocked: a handler of finished() can queue another task, which is refused
    lock.unlock();

    for (CacheTask* orphan : orphans) {
        orphan->finish(error);
        orphan->deleteLater();
    }
}

void CacheWorker::_runTask(CacheTask* task)
{
    QString error;
    if (task->work()) {
        error = _database->isValid() ? task->work()(*this, *task)
                                     : QCoreApplication::translate("QGCCacheWorker", "No Cache Database");
    }
    // A reset and an import rebuild the cache
    _dbValid = _database->isValid();
    task->finish(error);
}

void CacheWorker::emitTotals()
{
    const TileCacheTotals totals = _database->computeTotals();
    emit updateTotals(totals.totalCount, totals.totalSize, totals.defaultCount, totals.defaultSize);
    _updateTimer.restart();
}

std::unique_ptr<CacheTile> CacheWorker::tile(const QString& hash)
{
    std::unique_ptr<CacheTile> tile = _database->getTile(hash);
    if (tile) {
        _database->markTileUsed(hash);
    }

    // Under unit tests a cache miss consults the synthetic tile generator instead of
    // erroring, so tile consumers never fall back to real network fetches (late
    // replies race UI teardown and crash). See UnitTestTileGenerator.
#ifdef QGC_UNITTEST_BUILD
    if (!tile && MapTilesApp::runningUnitTests() && _unitTestTileGenerator) {
        tile.reset(_unitTestTileGenerator(hash));
    }
#endif

    return tile;
}
