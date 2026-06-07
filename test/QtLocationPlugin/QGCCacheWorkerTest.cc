#include "QGCCacheWorkerTest.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QTemporaryDir>
#include <QtTest/QTest>

#include "QGCCacheTile.h"
#include "QGCCachedTileSet.h"
#include "QGCMapTasks.h"
#include "QGCMapUrlEngine.h"
#include "QGCTileCacheDatabase.h"
#include "QGCTileCacheWorker.h"

static const QString kTestProviderType = QStringLiteral("Bing Road");

void QGCCacheWorkerTest::initTestCase()
{
    UnitTest::initTestCase();
    QVERIFY2(UrlFactory::getQtMapIdFromProviderType(kTestProviderType) != -1,
             ("Provider type '" + kTestProviderType.toLatin1() + "' not available in this build").constData());
}

bool QGCCacheWorkerTest::_startWorker(QGCCacheWorker& worker, int timeoutMs)
{
    bool totalsReceived = false;
    auto conn =
        connect(&worker, &QGCCacheWorker::updateTotals, this, [&]() { totalsReceived = true; }, Qt::QueuedConnection);

    auto* initTask = new QGCMapTask(QGCMapTask::TaskType::taskInit);
    if (!worker.enqueueTask(initTask)) {
        disconnect(conn);
        return false;
    }

    const bool ok = UnitTest::waitForCondition([&]() { return totalsReceived; }, timeoutMs,
                                               QStringLiteral("QGCCacheWorker::updateTotals"));
    disconnect(conn);
    return ok;
}

void QGCCacheWorkerTest::_testStartAndStop()
{
    QTemporaryDir tempDir;
    QGCCacheWorker worker;
    worker.setDatabaseFile(tempDir.filePath("start_stop.db"));
    QVERIFY(!worker.isRunning());

    QVERIFY(_startWorker(worker));
    QVERIFY(worker.isRunning());

    worker.stop();
    QVERIFY(worker.wait(TestTimeout::mediumMs()));
    QVERIFY(!worker.isRunning());
}

void QGCCacheWorkerTest::_testEnqueueBeforeInit()
{
    QTemporaryDir tempDir;
    QGCCacheWorker worker;
    worker.setDatabaseFile(tempDir.filePath("pre_init.db"));

    auto* task = new QGCFetchTileTask(QStringLiteral("hash"));
    bool errorReceived = false;
    connect(task, &QGCMapTask::error, this, [&](QGCMapTask::TaskType, const QString&) { errorReceived = true; });
    QVERIFY(!worker.enqueueTask(task));
    QVERIFY_TRUE_WAIT(errorReceived, TestTimeout::shortMs());
}

void QGCCacheWorkerTest::_testEnqueueAfterStop()
{
    QTemporaryDir tempDir;
    QGCCacheWorker worker;
    worker.setDatabaseFile(tempDir.filePath("enqueue_after_stop.db"));
    QVERIFY(_startWorker(worker));

    worker.stop();
    QVERIFY(worker.wait(TestTimeout::mediumMs()));
    QVERIFY(!worker.isRunning());

    // A taskInit would restart the worker, so exercise post-stop rejection with a non-init
    // task: enqueueTask() rejects with "Database Not Initialized" without restarting the thread.
    auto* task = new QGCFetchTileTask(QStringLiteral("after_stop_hash"));
    bool errorReceived = false;
    QString errorString;
    connect(task, &QGCMapTask::error, this, [&](QGCMapTask::TaskType, const QString& error) {
        errorReceived = true;
        errorString = error;
    });

    QVERIFY(!worker.enqueueTask(task));
    QVERIFY_TRUE_WAIT(errorReceived, TestTimeout::shortMs());
    QCOMPARE(errorString, QStringLiteral("Database Not Initialized"));
    QVERIFY(!worker.isRunning());
}

void QGCCacheWorkerTest::_testUpdateTotalsOnInit()
{
    QTemporaryDir tempDir;
    QGCCacheWorker worker;
    worker.setDatabaseFile(tempDir.filePath("totals.db"));

    quint32 totalTiles = UINT32_MAX;
    quint64 totalSize = UINT64_MAX;
    auto conn = connect(
        &worker, &QGCCacheWorker::updateTotals, this,
        [&](quint32 tiles, quint64 size, quint32, quint64) {
            totalTiles = tiles;
            totalSize = size;
        },
        Qt::QueuedConnection);

    QVERIFY(worker.enqueueTask(new QGCMapTask(QGCMapTask::TaskType::taskInit)));
    QTRY_VERIFY_WITH_TIMEOUT(totalTiles != UINT32_MAX, TestTimeout::mediumMs());
    disconnect(conn);

    QCOMPARE(totalTiles, static_cast<quint32>(0));
    QCOMPARE(totalSize, static_cast<quint64>(0));

    worker.stop();
    worker.wait(TestTimeout::mediumMs());
}

void QGCCacheWorkerTest::_testGetTotalsTask()
{
    QTemporaryDir tempDir;
    QGCCacheWorker worker;
    worker.setDatabaseFile(tempDir.filePath("get_totals.db"));
    QVERIFY(_startWorker(worker));

    auto* tile =
        new QGCCacheTile(QStringLiteral("gt1"), QByteArray("totals_data"), QStringLiteral("png"), kTestProviderType);
    QVERIFY(worker.enqueueTask(new QGCSaveTileTask(tile)));

    quint32 totalTiles = UINT32_MAX;
    quint64 totalSize = UINT64_MAX;
    auto conn = connect(
        &worker, &QGCCacheWorker::updateTotals, this,
        [&](quint32 tiles, quint64 size, quint32, quint64) {
            totalTiles = tiles;
            totalSize = size;
        },
        Qt::QueuedConnection);

    QVERIFY(worker.enqueueTask(new QGCMapTask(QGCMapTask::TaskType::taskInit)));
    QTRY_VERIFY_WITH_TIMEOUT(totalTiles != UINT32_MAX, TestTimeout::mediumMs());
    disconnect(conn);

    QVERIFY(totalTiles >= 1);
    QVERIFY(totalSize > 0);

    worker.stop();
    worker.wait(TestTimeout::mediumMs());
}

void QGCCacheWorkerTest::_testSaveAndFetchTile()
{
    QTemporaryDir tempDir;
    QGCCacheWorker worker;
    worker.setDatabaseFile(tempDir.filePath("save_fetch.db"));
    QVERIFY(_startWorker(worker));

    auto* tile =
        new QGCCacheTile(QStringLiteral("h1"), QByteArray("tile_data"), QStringLiteral("png"), kTestProviderType);
    QVERIFY(worker.enqueueTask(new QGCSaveTileTask(tile)));

    // Fetch — FIFO guarantees save completes first
    auto* fetchTask = new QGCFetchTileTask(QStringLiteral("h1"));
    QSharedPointer<QGCCacheTile> fetched;
    bool fetchError = false;
    connect(
        fetchTask, &QGCFetchTileTask::tileFetched, this, [&](QSharedPointer<QGCCacheTile> t) { fetched = t; },
        Qt::QueuedConnection);
    connect(
        fetchTask, &QGCMapTask::error, this, [&](QGCMapTask::TaskType, const QString&) { fetchError = true; },
        Qt::QueuedConnection);

    QVERIFY(worker.enqueueTask(fetchTask));
    QTRY_VERIFY_WITH_TIMEOUT(fetched || fetchError, TestTimeout::mediumMs());

    QVERIFY2(fetched != nullptr, "Expected tile to be fetched");
    QVERIFY(!fetchError);
    QCOMPARE(fetched->hash, QStringLiteral("h1"));
    QCOMPARE(fetched->img, QByteArray("tile_data"));
    QCOMPARE(fetched->format, QStringLiteral("png"));

    worker.stop();
    worker.wait(TestTimeout::mediumMs());
}

void QGCCacheWorkerTest::_testFetchTileNotFound()
{
    QTemporaryDir tempDir;
    QGCCacheWorker worker;
    worker.setDatabaseFile(tempDir.filePath("not_found.db"));
    QVERIFY(_startWorker(worker));

    auto* fetchTask = new QGCFetchTileTask(QStringLiteral("nonexistent"));
    QSharedPointer<QGCCacheTile> fetched;
    bool fetchError = false;
    connect(
        fetchTask, &QGCFetchTileTask::tileFetched, this, [&](QSharedPointer<QGCCacheTile> t) { fetched = t; },
        Qt::QueuedConnection);
    connect(
        fetchTask, &QGCMapTask::error, this, [&](QGCMapTask::TaskType, const QString&) { fetchError = true; },
        Qt::QueuedConnection);

    QVERIFY(worker.enqueueTask(fetchTask));
    QTRY_VERIFY_WITH_TIMEOUT(fetched || fetchError, TestTimeout::mediumMs());

    QVERIFY(fetched == nullptr);
    QVERIFY(fetchError);

    worker.stop();
    worker.wait(TestTimeout::mediumMs());
}

void QGCCacheWorkerTest::_testFetchTileSets()
{
    QTemporaryDir tempDir;
    QGCCacheWorker worker;
    worker.setDatabaseFile(tempDir.filePath("tile_sets.db"));
    QVERIFY(_startWorker(worker));

    // Wait for updateTotals after the fetch task completes to know all sets were emitted
    bool taskDone = false;
    auto totalsConn =
        connect(&worker, &QGCCacheWorker::updateTotals, this, [&]() { taskDone = true; }, Qt::QueuedConnection);

    QList<QGCCachedTileSet*> sets;
    auto* fetchTask = new QGCFetchTileSetTask();
    connect(
        fetchTask, &QGCFetchTileSetTask::tileSetFetched, this, [&](QGCCachedTileSet* set) { sets.append(set); },
        Qt::QueuedConnection);

    QVERIFY(worker.enqueueTask(fetchTask));
    QTRY_VERIFY_WITH_TIMEOUT(taskDone, TestTimeout::mediumMs());
    disconnect(totalsConn);

    QCOMPARE(sets.size(), 1);
    QVERIFY(sets[0]->defaultSet());
    QCOMPARE(sets[0]->name(), QStringLiteral("Default Tile Set"));
    qDeleteAll(sets);

    worker.stop();
    worker.wait(TestTimeout::mediumMs());
}

void QGCCacheWorkerTest::_testFetchTileSetsMainThreadAffinity()
{
    QTemporaryDir tempDir;
    QGCCacheWorker worker;
    worker.setDatabaseFile(tempDir.filePath("tile_sets_affinity.db"));
    QVERIFY(_startWorker(worker));

    bool taskDone = false;
    auto totalsConn =
        connect(&worker, &QGCCacheWorker::updateTotals, this, [&]() { taskDone = true; }, Qt::QueuedConnection);

    bool fetchError = false;
    QList<QGCCachedTileSet*> sets;
    auto* fetchTask = new QGCFetchTileSetTask();
    connect(
        fetchTask, &QGCFetchTileSetTask::tileSetFetched, this, [&](QGCCachedTileSet* set) { sets.append(set); },
        Qt::QueuedConnection);
    connect(
        fetchTask, &QGCMapTask::error, this, [&](QGCMapTask::TaskType, const QString&) { fetchError = true; },
        Qt::QueuedConnection);

    QVERIFY(worker.enqueueTask(fetchTask));
    QTRY_VERIFY_WITH_TIMEOUT(taskDone, TestTimeout::mediumMs());
    disconnect(totalsConn);

    QVERIFY(!fetchError);
    QVERIFY(!sets.isEmpty());
    for (QGCCachedTileSet* set : sets) {
        QVERIFY(set != nullptr);
        QCOMPARE(set->thread(), QCoreApplication::instance()->thread());
    }
    qDeleteAll(sets);

    worker.stop();
    worker.wait(TestTimeout::mediumMs());
}

void QGCCacheWorkerTest::_testCreateAndDeleteTileSet()
{
    QTemporaryDir tempDir;
    QGCCacheWorker worker;
    worker.setDatabaseFile(tempDir.filePath("create_delete.db"));
    QVERIFY(_startWorker(worker));

    auto* tileSet = new QGCCachedTileSet(QStringLiteral("Test Set"));
    tileSet->setMapTypeStr(QStringLiteral("TestMap"));
    tileSet->setType(kTestProviderType);
    tileSet->setTopleftLat(37.0);
    tileSet->setTopleftLon(-122.0);
    tileSet->setBottomRightLat(36.0);
    tileSet->setBottomRightLon(-121.0);
    tileSet->setMinZoom(5);
    tileSet->setMaxZoom(5);
    tileSet->setTotalTileCount(1);

    auto* createTask = new QGCCreateTileSetTask(tileSet);
    QGCCachedTileSet* savedSet = nullptr;
    bool createError = false;
    connect(
        createTask, &QGCCreateTileSetTask::tileSetSaved, this, [&](QGCCachedTileSet* set) { savedSet = set; },
        Qt::QueuedConnection);
    connect(
        createTask, &QGCMapTask::error, this, [&](QGCMapTask::TaskType, const QString&) { createError = true; },
        Qt::QueuedConnection);

    QVERIFY(worker.enqueueTask(createTask));
    QTRY_VERIFY_WITH_TIMEOUT(savedSet || createError, TestTimeout::mediumMs());
    QVERIFY2(savedSet != nullptr, "Expected tile set to be created");
    QVERIFY(!createError);
    QVERIFY(savedSet->id() != 0);

    // Delete it
    const quint64 setID = savedSet->id();
    auto* deleteTask =
        new QGCCommandTask(QGCMapTask::TaskType::taskDeleteTileSet, [setID](QGCCacheWorker& w, QGCMapTask& self) {
            if (!w.validateDatabase(&self)) {
                return false;
            }
            if (!w.database()->deleteTileSet(setID)) {
                self.setError("Error deleting tile set");
                return false;
            }
            w.emitTotals();
            return true;
        });
    bool deleted = false;
    connect(deleteTask, &QGCCommandTask::completed, this, [&]() { deleted = true; }, Qt::QueuedConnection);

    QVERIFY(worker.enqueueTask(deleteTask));
    QTRY_VERIFY_WITH_TIMEOUT(deleted, TestTimeout::mediumMs());
    QVERIFY(deleted);

    delete savedSet;

    worker.stop();
    worker.wait(TestTimeout::mediumMs());
}

void QGCCacheWorkerTest::_testPruneCache()
{
    QTemporaryDir tempDir;
    QGCCacheWorker worker;
    worker.setDatabaseFile(tempDir.filePath("prune.db"));
    QVERIFY(_startWorker(worker));

    for (int i = 0; i < 10; i++) {
        auto* tile = new QGCCacheTile(QStringLiteral("pr_%1").arg(i), QByteArray(100, 'P'), QStringLiteral("png"),
                                      kTestProviderType);
        QVERIFY(worker.enqueueTask(new QGCSaveTileTask(tile)));
    }

    auto* pruneTask = new QGCCommandTask(QGCMapTask::TaskType::taskPruneCache, [](QGCCacheWorker& w, QGCMapTask& self) {
        if (!w.validateDatabase(&self)) {
            return false;
        }
        if (!w.database()->pruneCache(500)) {
            self.setError("Error pruning cache");
            return false;
        }
        return true;
    });
    bool pruneDone = false;
    connect(pruneTask, &QGCCommandTask::completed, this, [&]() { pruneDone = true; }, Qt::QueuedConnection);
    QVERIFY(worker.enqueueTask(pruneTask));
    QTRY_VERIFY_WITH_TIMEOUT(pruneDone, TestTimeout::mediumMs());

    // Trigger totals to verify count decreased
    quint32 remaining = UINT32_MAX;
    auto conn = connect(
        &worker, &QGCCacheWorker::updateTotals, this,
        [&](quint32 tiles, quint64, quint32, quint64) { remaining = tiles; }, Qt::QueuedConnection);
    QVERIFY(worker.enqueueTask(new QGCMapTask(QGCMapTask::TaskType::taskInit)));
    QTRY_VERIFY_WITH_TIMEOUT(remaining != UINT32_MAX, TestTimeout::mediumMs());
    disconnect(conn);

    QVERIFY2(remaining < 10,
             qPrintable(QStringLiteral("Expected fewer than 10 tiles after prune, got %1").arg(remaining)));

    worker.stop();
    worker.wait(TestTimeout::mediumMs());
}

void QGCCacheWorkerTest::_testResetDatabase()
{
    QTemporaryDir tempDir;
    QGCCacheWorker worker;
    worker.setDatabaseFile(tempDir.filePath("reset.db"));
    QVERIFY(_startWorker(worker));

    auto* tile = new QGCCacheTile(QStringLiteral("r1"), QByteArray("data"), QStringLiteral("png"), kTestProviderType);
    QVERIFY(worker.enqueueTask(new QGCSaveTileTask(tile)));

    auto* resetTask = new QGCCommandTask(QGCMapTask::TaskType::taskReset, [](QGCCacheWorker& w, QGCMapTask& self) {
        if (!w.validateDatabase(&self)) {
            return false;
        }
        if (!w.database()->resetDatabase()) {
            self.setError("Error resetting cache database");
            return false;
        }
        w.setDatabaseValid(w.database()->isValid());
        return true;
    });
    bool resetDone = false;
    connect(resetTask, &QGCCommandTask::completed, this, [&]() { resetDone = true; }, Qt::QueuedConnection);
    QVERIFY(worker.enqueueTask(resetTask));
    QTRY_VERIFY_WITH_TIMEOUT(resetDone, TestTimeout::mediumMs());

    // Verify saved tile is gone
    auto* fetchTask = new QGCFetchTileTask(QStringLiteral("r1"));
    bool fetchError = false;
    connect(
        fetchTask, &QGCMapTask::error, this, [&](QGCMapTask::TaskType, const QString&) { fetchError = true; },
        Qt::QueuedConnection);
    QVERIFY(worker.enqueueTask(fetchTask));
    QTRY_VERIFY_WITH_TIMEOUT(fetchError, TestTimeout::mediumMs());

    QVERIFY(worker.isRunning());

    worker.stop();
    worker.wait(TestTimeout::mediumMs());
}

void QGCCacheWorkerTest::_testStopWhileProcessing()
{
    QTemporaryDir tempDir;
    QGCCacheWorker worker;
    worker.setDatabaseFile(tempDir.filePath("stop_busy.db"));
    QVERIFY(_startWorker(worker));

    for (int i = 0; i < 100; i++) {
        auto* tile = new QGCCacheTile(QStringLiteral("stop_%1").arg(i), QByteArray(50, 'S'), QStringLiteral("png"),
                                      kTestProviderType);
        worker.enqueueTask(new QGCSaveTileTask(tile));
    }

    worker.stop();
    QVERIFY(worker.wait(TestTimeout::mediumMs()));
    QVERIFY(!worker.isRunning());
}

void QGCCacheWorkerTest::_testCacheTileBatchCoalesce()
{
    QTemporaryDir tempDir;
    QGCCacheWorker worker;
    worker.setDatabaseFile(tempDir.filePath("batch_coalesce.db"));
    QVERIFY(_startWorker(worker));

    constexpr int kTileCount = 200;
    for (int i = 0; i < kTileCount; i++) {
        auto* tile = new QGCCacheTile(QStringLiteral("bc_%1").arg(i), QByteArray(64, 'B'), QStringLiteral("png"),
                                      kTestProviderType);
        QVERIFY(worker.enqueueTask(new QGCSaveTileTask(tile)));
    }

    quint32 totalTiles = 0;
    bool totalsReceived = false;
    auto conn = connect(
        &worker, &QGCCacheWorker::updateTotals, this,
        [&](quint32 tiles, quint64, quint32, quint64) {
            totalTiles = tiles;
            totalsReceived = true;
        },
        Qt::QueuedConnection);
    QVERIFY(worker.enqueueTask(new QGCMapTask(QGCMapTask::TaskType::taskInit)));
    QTRY_VERIFY_WITH_TIMEOUT(totalsReceived && (totalTiles >= static_cast<quint32>(kTileCount)),
                             TestTimeout::mediumMs());
    disconnect(conn);

    QCOMPARE(totalTiles, static_cast<quint32>(kTileCount));

    worker.stop();
    QVERIFY(worker.wait(TestTimeout::mediumMs()));
    QVERIFY(!worker.isRunning());
}

UT_REGISTER_TEST(QGCCacheWorkerTest, TestLabel::Unit)
