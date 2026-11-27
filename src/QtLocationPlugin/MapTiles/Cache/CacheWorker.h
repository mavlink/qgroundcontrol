#pragma once

#include <chrono>
#include <memory>

#include <QtCore/QElapsedTimer>
#include <QtCore/QHash>
#include <QtCore/QMutex>
#include <QtCore/QQueue>
#include <QtCore/QString>
#include <QtCore/QThread>
#include <QtCore/QWaitCondition>

#ifdef QGC_UNITTEST_BUILD
#include <functional>
#endif

struct CacheTile;
class CacheTask;
class TileCacheDatabase;

class CacheWorker : public QThread
{
    Q_OBJECT

public:
    explicit CacheWorker(QObject* parent = nullptr);
    ~CacheWorker();

    void setDatabaseFile(const QString& path)
    {
        if (isRunning()) {
            return;
        }
        _databasePath = path;
    }

    /// The cache variants (MapProviderRegistry::cacheVariants()) that the tiles of a cache from before schema version 3
    /// get. Set them on the main thread, before the worker runs.
    void setCacheVariants(const QHash<int, QString>& cacheVariants)
    {
        if (isRunning()) {
            return;
        }
        _cacheVariants = cacheVariants;
    }

#ifdef QGC_UNITTEST_BUILD
    /// Unit-test hook: consulted on tile cache miss to synthesize a tile instead of
    /// erroring, so tests never fall back to real network fetches. Only active while
    /// running unit tests. A nullptr result falls through to the normal miss error.
    ///
    /// Threading contract: the generator is read from worker threads without
    /// synchronization. It must be installed once, before any CacheWorker thread
    /// has started, and never changed afterward (see UnitTestTileGenerator::install,
    /// called at test-run startup). The generator itself must be thread-safe.
    static void setUnitTestTileGenerator(std::function<CacheTile*(const QString& hash)> generator);
#endif

    // For the work of a task (CacheTask::Work), on the worker thread:

    /// The cache, which is open and valid while a task works
    TileCacheDatabase& database() { return *_database; }

    /// The tile of the cache, which then counts as used (TileCacheDatabase::markTileUsed()), or null. Under unit
    /// tests, a tile the cache lacks is the one their generator makes.
    std::unique_ptr<CacheTile> tile(const QString& hash);

    /// The work deleted rows: the worker gives their pages back to the file system while it has no task
    void setReleasePending() { _releasePending = true; }

    void emitTotals();

    // For any thread:

    /// Takes \a task: it is deleted once it has finished. A task that the worker cannot run finishes with an error
    /// here, and false is returned.
    bool enqueueTask(CacheTask* task);
    /// Tasks that are still queued are deleted, without CacheTask::finished()
    void stop();

signals:
    void updateTotals(quint32 totaltiles, quint64 totalsize, quint32 defaulttiles, quint64 defaultsize);

protected:
    void run() final;

private:
    void _runTask(CacheTask* task);
    /// Takes no more tasks, and ends those that are queued with \a error
    void _endQueuedTasks(const QString& error);

    std::unique_ptr<TileCacheDatabase> _database;
    QMutex _taskQueueMutex;
    QQueue<CacheTask*> _taskQueue;
    /// Under _taskQueueMutex: run() takes the queued tasks. Set when the thread is started, and cleared by run() as it
    /// ends the tasks that are left, before the thread quits.
    bool _takingTasks = false;
    QWaitCondition _waitc;
    QString _databasePath;
    QHash<int, QString> _cacheVariants;
    QElapsedTimer _updateTimer;
    std::chrono::milliseconds _updateInterval = kShortUpdateInterval;
    std::atomic_bool _dbValid = false;
    std::atomic_bool _stopRequested = false;
    /// The file may have unused pages to give back: see TileCacheDatabase::releaseFreePages()
    bool _releasePending = false;

    /// How often the totals of the cache are told while tasks wait, and how often while many do
    static constexpr std::chrono::milliseconds kShortUpdateInterval{2000};
    static constexpr std::chrono::milliseconds kLongUpdateInterval{5000};
    /// The longest that a worker without work waits before it looks at its queue again
    static constexpr std::chrono::seconds kIdleWait{5};
    /// Pages released before the task queue is looked at again. A batch took 4 ms on average and 22 ms at most on an
    /// SSD, with pages to move for those released.
    static constexpr int kReleaseBatchPages = 256;

#ifdef QGC_UNITTEST_BUILD
    static std::function<CacheTile*(const QString&)> _unitTestTileGenerator;
#endif
};
