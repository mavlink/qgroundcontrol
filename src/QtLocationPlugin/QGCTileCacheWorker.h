#pragma once

#include <QtCore/QElapsedTimer>
#include <QtCore/QMutex>
#include <QtCore/QQueue>
#include <QtCore/QString>
#include <QtCore/QThread>
#include <QtCore/QWaitCondition>

#include <memory>

#ifdef QGC_UNITTEST_BUILD
#include <functional>
#endif

class QGCMapTask;
class QGCTileCacheDatabase;

#ifdef QGC_UNITTEST_BUILD
struct QGCCacheTile;
#endif

class QGCCacheWorker : public QThread
{
    Q_OBJECT

public:
    explicit QGCCacheWorker(QObject *parent = nullptr);
    ~QGCCacheWorker();

    void setDatabaseFile(const QString &path) { if (isRunning()) { return; } _databasePath = path; }

#ifdef QGC_UNITTEST_BUILD
    /// Unit-test hook: consulted on tile cache miss to synthesize a tile instead of
    /// erroring, so tests never fall back to real network fetches. Only active while
    /// running unit tests. A nullptr result falls through to the normal miss error.
    ///
    /// Threading contract: the generator is read from worker threads without
    /// synchronization. It must be installed once, before any QGCCacheWorker thread
    /// has started, and never changed afterward (see UnitTestTileGenerator::install,
    /// called at test-run startup). The generator itself must be thread-safe.
    static void setUnitTestTileGenerator(std::function<QGCCacheTile*(const QString &hash)> generator);
#endif

public slots:
    bool enqueueTask(QGCMapTask *task);
    void stop();

signals:
    void updateTotals(quint32 totaltiles, quint64 totalsize, quint32 defaulttiles, quint64 defaultsize);

protected:
    void run() final;

private:
    void _runTask(QGCMapTask *task);

    void _saveTile(QGCMapTask *task);
    void _getTile(QGCMapTask *task);
    void _getTileSets(QGCMapTask *task);
    void _createTileSet(QGCMapTask *task);
    void _getTileDownloadList(QGCMapTask *task);
    void _updateTileDownloadState(QGCMapTask *task);
    void _pruneCache(QGCMapTask *task);
    void _deleteTileSet(QGCMapTask *task);
    void _renameTileSet(QGCMapTask *task);
    void _resetCacheDatabase(QGCMapTask *task);
    void _importSets(QGCMapTask *task);
    void _exportSets(QGCMapTask *task);
    bool _testTask(QGCMapTask *task);
    void _emitTotals();

    std::unique_ptr<QGCTileCacheDatabase> _database;
    QMutex _taskQueueMutex;
    QQueue<QGCMapTask*> _taskQueue;
    QWaitCondition _waitc;
    QString _databasePath;
    QElapsedTimer _updateTimer;
    int _updateTimeout = kShortTimeoutMs;
    std::atomic_bool _dbValid = false;
    std::atomic_bool _stopRequested = false;

    static constexpr int kShortTimeoutMs = 2000;
    static constexpr int kLongTimeoutMs = 5000;

#ifdef QGC_UNITTEST_BUILD
    static std::function<QGCCacheTile*(const QString&)> _unitTestTileGenerator;
#endif
};
