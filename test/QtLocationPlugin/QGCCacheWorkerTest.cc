#include "QGCCacheWorkerTest.h"

#include <QtTest/QTest>

#include "QGCCacheTile.h"
#include "QGCCachedTileSet.h"
#include "QGCMapTasks.h"
#include "QGCTileCacheWorker.h"

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
    QGCCacheWorker worker;
    worker.setDatabaseFile(tempPath("start_stop.db"));
    QVERIFY(!worker.isRunning());

    QVERIFY(_startWorker(worker));
    QVERIFY(worker.isRunning());

    worker.stop();
    QVERIFY(worker.wait(TestTimeout::mediumMs()));
    QVERIFY(!worker.isRunning());
}

void QGCCacheWorkerTest::_testEnqueueBeforeInit()
{
    QGCCacheWorker worker;
    worker.setDatabaseFile(tempPath("pre_init.db"));

    auto* task = new QGCFetchTileTask(QStringLiteral("hash"));
    bool errorReceived = false;
    connect(task, &QGCMapTask::error, this, [&](QGCMapTask::TaskType, const QString&) { errorReceived = true; });
    QVERIFY(!worker.enqueueTask(task));
    QVERIFY_TRUE_WAIT(errorReceived, TestTimeout::shortMs());
}

void QGCCacheWorkerTest::_testUpdateTotalsOnInit()
{
    QGCCacheWorker worker;
    worker.setDatabaseFile(tempPath("totals.db"));

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

void QGCCacheWorkerTest::_testSaveAndFetchTile()
{
    QGCCacheWorker worker;
    worker.setDatabaseFile(tempPath("save_fetch.db"));
    QVERIFY(_startWorker(worker));

    auto* tile =
        new QGCCacheTile(QStringLiteral("h1"), QByteArray("tile_data"), QStringLiteral("png"), QStringLiteral("T"));
    QVERIFY(worker.enqueueTask(new QGCSaveTileTask(tile)));

    // Fetch — FIFO guarantees save completes first
    auto* fetchTask = new QGCFetchTileTask(QStringLiteral("h1"));
    QGCCacheTile* fetched = nullptr;
    bool fetchError = false;
    connect(
        fetchTask, &QGCFetchTileTask::tileFetched, this, [&](QGCCacheTile* t) { fetched = t; }, Qt::QueuedConnection);
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
    delete fetched;

    worker.stop();
    worker.wait(TestTimeout::mediumMs());
}

void QGCCacheWorkerTest::_testFetchTileNotFound()
{
    QGCCacheWorker worker;
    worker.setDatabaseFile(tempPath("not_found.db"));
    QVERIFY(_startWorker(worker));

    auto* fetchTask = new QGCFetchTileTask(QStringLiteral("nonexistent"));
    QGCCacheTile* fetched = nullptr;
    bool fetchError = false;
    connect(
        fetchTask, &QGCFetchTileTask::tileFetched, this, [&](QGCCacheTile* t) { fetched = t; }, Qt::QueuedConnection);
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
    QGCCacheWorker worker;
    worker.setDatabaseFile(tempPath("tile_sets.db"));
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

void QGCCacheWorkerTest::_testCreateAndDeleteTileSet()
{
    QGCCacheWorker worker;
    worker.setDatabaseFile(tempPath("create_delete.db"));
    QVERIFY(_startWorker(worker));

    auto* tileSet = new QGCCachedTileSet(QStringLiteral("Test Set"));
    tileSet->setMapTypeStr(QStringLiteral("TestMap"));
    tileSet->setType(QStringLiteral("T"));
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
    auto* deleteTask = new QGCDeleteTileSetTask(setID);
    quint64 deletedID = 0;
    connect(
        deleteTask, &QGCDeleteTileSetTask::tileSetDeleted, this, [&](quint64 id) { deletedID = id; },
        Qt::QueuedConnection);

    QVERIFY(worker.enqueueTask(deleteTask));
    QTRY_VERIFY_WITH_TIMEOUT(deletedID != 0, TestTimeout::mediumMs());
    QCOMPARE(deletedID, setID);

    delete savedSet;

    worker.stop();
    worker.wait(TestTimeout::mediumMs());
}

void QGCCacheWorkerTest::_testPruneCache()
{
    QGCCacheWorker worker;
    worker.setDatabaseFile(tempPath("prune.db"));
    QVERIFY(_startWorker(worker));

    for (int i = 0; i < 10; i++) {
        auto* tile = new QGCCacheTile(QStringLiteral("pr_%1").arg(i), QByteArray(100, 'P'), QStringLiteral("png"),
                                      QStringLiteral("T"));
        QVERIFY(worker.enqueueTask(new QGCSaveTileTask(tile)));
    }

    auto* pruneTask = new QGCPruneCacheTask(500);
    bool pruneDone = false;
    connect(pruneTask, &QGCPruneCacheTask::pruned, this, [&]() { pruneDone = true; }, Qt::QueuedConnection);
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
    QGCCacheWorker worker;
    worker.setDatabaseFile(tempPath("reset.db"));
    QVERIFY(_startWorker(worker));

    auto* tile = new QGCCacheTile(QStringLiteral("r1"), QByteArray("data"), QStringLiteral("png"), QStringLiteral("T"));
    QVERIFY(worker.enqueueTask(new QGCSaveTileTask(tile)));

    auto* resetTask = new QGCResetTask();
    bool resetDone = false;
    connect(resetTask, &QGCResetTask::resetCompleted, this, [&]() { resetDone = true; }, Qt::QueuedConnection);
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
    QGCCacheWorker worker;
    worker.setDatabaseFile(tempPath("stop_busy.db"));
    QVERIFY(_startWorker(worker));

    for (int i = 0; i < 100; i++) {
        auto* tile = new QGCCacheTile(QStringLiteral("stop_%1").arg(i), QByteArray(50, 'S'), QStringLiteral("png"),
                                      QStringLiteral("T"));
        worker.enqueueTask(new QGCSaveTileTask(tile));
    }

    worker.stop();
    QVERIFY(worker.wait(TestTimeout::mediumMs()));
    QVERIFY(!worker.isRunning());
}

UT_REGISTER_TEST(QGCCacheWorkerTest, TestLabel::Unit)
