#include "QGCMapTaskTest.h"

#include <QtCore/QPointer>
#include <QtCore/QQueue>
#include <QtCore/QSharedPointer>
#include <QtTest/QSignalSpy>

#include "QGCCacheTile.h"
#include "QGCCachedTileSet.h"
#include "QGCMapTasks.h"
#include "QGCTile.h"
#include "QGCTileCacheWorker.h"

void QGCMapTaskTest::_testBaseTypeAndError()
{
    QGCFetchTileTask task(QStringLiteral("abc"));
    QCOMPARE(task.type(), QGCMapTask::TaskType::taskFetchTile);

    QSignalSpy spy(&task, &QGCMapTask::error);
    QVERIFY(spy.isValid());
    task.setError(QStringLiteral("boom"));
    QCOMPARE(spy.count(), 1);
    const QList<QVariant> args = spy.takeFirst();
    QCOMPARE(args.at(0).value<QGCMapTask::TaskType>(), QGCMapTask::TaskType::taskFetchTile);
    QCOMPARE(args.at(1).toString(), QStringLiteral("boom"));
}

void QGCMapTaskTest::_testFetchTileSetTask()
{
    QGCFetchTileSetTask task;
    QCOMPARE(task.type(), QGCMapTask::TaskType::taskFetchTileSets);

    QGCCachedTileSet* captured = nullptr;
    connect(&task, &QGCFetchTileSetTask::tileSetFetched, &task, [&captured](QGCCachedTileSet* s) { captured = s; });

    QGCCachedTileSet set(QStringLiteral("set"));
    task.setTileSetFetched(&set);
    QCOMPARE(captured, &set);
}

void QGCMapTaskTest::_testCreateTileSetTaskSavedTransfersOwnership()
{
    QPointer<QGCCachedTileSet> set = new QGCCachedTileSet(QStringLiteral("owned"));
    {
        QGCCreateTileSetTask task(set);
        QCOMPARE(task.type(), QGCMapTask::TaskType::taskCreateTileSet);
        QCOMPARE(task.tileSet(), set.data());

        QGCCachedTileSet* captured = nullptr;
        connect(&task, &QGCCreateTileSetTask::tileSetSaved, &task, [&captured](QGCCachedTileSet* s) { captured = s; });
        task.setTileSetSaved();
        QCOMPARE(captured, set.data());
    }
    QVERIFY(!set.isNull());
    delete set;
}

void QGCMapTaskTest::_testCreateTileSetTaskUnsavedDeletesTileSet()
{
    QPointer<QGCCachedTileSet> set = new QGCCachedTileSet(QStringLiteral("owned"));
    {
        QGCCreateTileSetTask task(set);
    }
    QVERIFY(set.isNull());
}

void QGCMapTaskTest::_testFetchTileTask()
{
    QGCFetchTileTask task(QStringLiteral("hash123"));
    QCOMPARE(task.type(), QGCMapTask::TaskType::taskFetchTile);
    QCOMPARE(task.hash(), QStringLiteral("hash123"));

    QSharedPointer<QGCCacheTile> captured;
    connect(&task, &QGCFetchTileTask::tileFetched, &task,
            [&captured](QSharedPointer<QGCCacheTile> t) { captured = t; });

    auto tile = QSharedPointer<QGCCacheTile>::create(QStringLiteral("hash123"), 7);
    task.setTileFetched(tile);
    QCOMPARE(captured, tile);
}

void QGCMapTaskTest::_testSaveTileTaskDeletesTileOnDestroy()
{
    auto* tile = new QGCCacheTile(QStringLiteral("h"), QByteArrayLiteral("img"), QStringLiteral("png"),
                                  QStringLiteral("Bing Road"));
    QGCSaveTileTask task(tile);
    QCOMPARE(task.type(), QGCMapTask::TaskType::taskCacheTile);
    QCOMPARE(task.tile(), tile);
    QCOMPARE(static_cast<const QGCSaveTileTask&>(task).tile(), tile);
    // Task dtor owns and deletes `tile`; exercised on scope exit (sanitizer-verified).
}

void QGCMapTaskTest::_testGetTileDownloadListTask()
{
    QGCGetTileDownloadListTask task(42, 16);
    QCOMPARE(task.type(), QGCMapTask::TaskType::taskGetTileDownloadList);
    QCOMPARE(task.setID(), static_cast<quint64>(42));
    QCOMPARE(task.count(), 16);

    QQueue<QGCTile*> captured;
    bool fired = false;
    connect(&task, &QGCGetTileDownloadListTask::tileListFetched, &task, [&](const QQueue<QGCTile*>& tiles) {
        captured = tiles;
        fired = true;
    });

    auto* tile = new QGCTile();
    QQueue<QGCTile*> queue;
    queue.enqueue(tile);
    task.setTileListFetched(queue);

    QVERIFY(fired);
    QCOMPARE(captured.size(), 1);
    QCOMPARE(captured.first(), tile);
    delete tile;
}

void QGCMapTaskTest::_testCommandTask()
{
    QGCCacheWorker worker;

    QGCCommandTask okTask(QGCMapTask::TaskType::taskReset, [](QGCCacheWorker&, QGCMapTask&) { return true; });
    QCOMPARE(okTask.type(), QGCMapTask::TaskType::taskReset);

    QSignalSpy completedSpy(&okTask, &QGCCommandTask::completed);
    QVERIFY(completedSpy.isValid());
    okTask.execute(worker);
    QCOMPARE(completedSpy.count(), 1);

    QGCCommandTask failTask(QGCMapTask::TaskType::taskDeleteTileSet, [](QGCCacheWorker&, QGCMapTask& self) {
        self.setError(QStringLiteral("nope"));
        return false;
    });
    QSignalSpy failCompletedSpy(&failTask, &QGCCommandTask::completed);
    QSignalSpy errorSpy(&failTask, &QGCMapTask::error);
    failTask.execute(worker);
    QCOMPARE(failCompletedSpy.count(), 0);
    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(errorSpy.takeFirst().at(0).value<QGCMapTask::TaskType>(), QGCMapTask::TaskType::taskDeleteTileSet);
}

void QGCMapTaskTest::_testExportTileTask()
{
    TileSetRecord record;
    record.setID = 11;
    record.name = QStringLiteral("export");
    const QList<TileSetRecord> sets{record};

    QGCExportTileTask task(sets, QStringLiteral("/tmp/out.qgctiledb"));
    QCOMPARE(task.type(), QGCMapTask::TaskType::taskExport);
    QCOMPARE(task.path(), QStringLiteral("/tmp/out.qgctiledb"));
    QCOMPARE(task.sets().size(), 1);
    QCOMPARE(task.sets().first().setID, static_cast<quint64>(11));

    QSignalSpy progressSpy(&task, &QGCExportTileTask::actionProgress);
    QSignalSpy completedSpy(&task, &QGCExportTileTask::actionCompleted);
    QVERIFY(progressSpy.isValid());
    QVERIFY(completedSpy.isValid());

    task.setProgress(50);
    QCOMPARE(progressSpy.count(), 1);
    QCOMPARE(progressSpy.takeFirst().at(0).toInt(), 50);

    task.setExportCompleted();
    QCOMPARE(completedSpy.count(), 1);
}

void QGCMapTaskTest::_testImportTileTask()
{
    QGCImportTileTask task(QStringLiteral("/tmp/in.qgctiledb"), true);
    QCOMPARE(task.type(), QGCMapTask::TaskType::taskImport);
    QCOMPARE(task.path(), QStringLiteral("/tmp/in.qgctiledb"));
    QVERIFY(task.replace());
    QCOMPARE(task.progress(), 0);

    QSignalSpy progressSpy(&task, &QGCImportTileTask::actionProgress);
    QSignalSpy completedSpy(&task, &QGCImportTileTask::actionCompleted);
    QVERIFY(progressSpy.isValid());
    QVERIFY(completedSpy.isValid());

    task.setProgress(75);
    QCOMPARE(task.progress(), 75);
    QCOMPARE(progressSpy.count(), 1);
    QCOMPARE(progressSpy.takeFirst().at(0).toInt(), 75);

    task.setImportCompleted();
    QCOMPARE(completedSpy.count(), 1);
}

UT_REGISTER_TEST(QGCMapTaskTest, TestLabel::Unit)
