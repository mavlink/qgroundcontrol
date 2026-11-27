#include "TileCacheTest.h"

#include <optional>

#include <QtCore/QCoreApplication>
#include <QtCore/QRegularExpression>
#include <QtCore/QSemaphore>
#include <QtCore/QTemporaryDir>
#include <QtTest/QTest>

#include "AppSettings.h"
#include "CacheTask.h"
#include "CacheTile.h"
#include "Fixtures/RAIIFixtures.h"
#include "MapsSettings.h"
#include "SettingsManager.h"
#include "TileCache.h"
#include "TileCacheTestHelpers.h"

void TileCacheTest::_testCloseStopsWorkerAndRefusesInit()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString databasePath = tempDir.filePath(QStringLiteral("cache.db"));

    TileCache cache;
    cache.init(databasePath);
    QVERIFY(cache.addTask(tileCacheIdleTask()));

    cache.close();
    QVERIFY(!cache.addTask(tileCacheIdleTask()));

    // A map created during application teardown must not restart the worker.
    cache.init(databasePath);
    QVERIFY(!cache.addTask(tileCacheIdleTask()));
}

// GeoMap needs the tile cache without ever creating a QtLocation map, which is what normally initializes it
void TileCacheTest::_testEnsureInitializedAcceptsTasks()
{
    TileCache cache;
    QVERIFY(!cache.addTask(tileCacheIdleTask()));

    cache.ensureInitialized();
    QVERIFY(cache.addTask(tileCacheIdleTask()));

    // Calling it again keeps the running worker
    cache.ensureInitialized();
    QVERIFY(cache.addTask(tileCacheIdleTask()));

    cache.close();
}

// Without its database the cache stores nothing, and has no offline map: the user is to know why
void TileCacheTest::_testTellsUserOfCacheThatCannotBeOpened()
{
    TileCache cache;
    // As when no directory can be made for the cache
    expectLogMessage("MapTiles.TileCacheDatabase", QtCriticalMsg,
                     QRegularExpression(QStringLiteral("Could not find suitable cache directory")));
    expectLogMessage("MapTiles.CacheWorker", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Failed To Init Database")));
    expectLogMessage("MapTiles.TileCache", QtWarningMsg,
                     QRegularExpression(QStringLiteral("The tile cache could not be opened")));
    expectAppMessage(QRegularExpression(QStringLiteral("The map tile cache could not be opened")));
    cache.init(QString());

    // The worker refuses tasks once it has ended those it had, the first of which tells of the failure: the end of
    // that one is queued by then.
    QVERIFY(QTest::qWaitFor([&cache] { return !cache.addTask(tileCacheIdleTask()); }, TestTimeout::mediumMs()));
    QCoreApplication::processEvents();
    for (int i = 0; i < 4; i++) {
        verifyExpectedLogMessage();
    }

    cache.close();
}

void TileCacheTest::_testPrunesAgainAfterFailedPrune()
{
    constexpr quint64 kMaxCacheBytes = 1024 * 1024;
    TestFixtures::SettingsFixture saved;
    saved.setFactValue(SettingsManager::instance()->mapsSettings()->maxCacheDiskSize(), 1);

    // A cache whose default set is over the limit, and that refuses to delete a tile
    QTemporaryDir tempDir;
    const QString path = tempDir.filePath("tiles.db");
    QVERIFY(tileCacheAddLargeTileSet(path, QStringLiteral("default"), 20, nullptr));
    QVERIFY(tileCacheExec(path, QStringLiteral("CREATE TRIGGER refuse_delete BEFORE DELETE ON Tiles "
                                               "BEGIN SELECT RAISE(ABORT, 'refused'); END")));

    quint64 defaultSize = 0;
    TileCache cache;
    connect(&cache, &TileCache::updateTotals, this,
            [&defaultSize](quint32, quint64, quint32, quint64 size) { defaultSize = size; });
    const auto addTask = [&cache](CacheTask* task) { return cache.addTask(task); };

    // The cache prunes for the totals of the database it opens, and the prune fails
    cache.init(path);
    QTRY_VERIFY_WITH_TIMEOUT(defaultSize > kMaxCacheBytes, TestTimeout::mediumMs());
    QCOMPARE(tileCacheRunTask(this, addTask, tileCacheIdleTask()), QString());
    QVERIFY(defaultSize > kMaxCacheBytes);

    // The next change of the cache has it prune again
    QVERIFY(tileCacheExec(path, QStringLiteral("DROP TRIGGER refuse_delete")));
    cache.cacheTile(kTileCacheTestMapId, QStringLiteral("one_more"), QByteArray(1024, 'M'), QStringLiteral("png"));
    QTRY_VERIFY_WITH_TIMEOUT(defaultSize <= kMaxCacheBytes, TestTimeout::mediumMs());

    cache.close();
}

// The worker deletes a prune that is still queued when the cache shuts down, without its end
void TileCacheTest::_testPrunesAgainAfterShutdownDroppedPrune()
{
    constexpr quint64 kMaxCacheBytes = 1024 * 1024;
    TestFixtures::SettingsFixture saved;
    saved.setFactValue(SettingsManager::instance()->mapsSettings()->maxCacheDiskSize(), 1);

    QTemporaryDir tempDir;
    const QString path = tempDir.filePath("tiles.db");
    QVERIFY(tileCacheAddLargeTileSet(path, QStringLiteral("default"), 20, nullptr));

    // A task queued with the first totals holds the worker: the prune for those totals is queued behind it. The
    // timeout only keeps a failed test from hanging.
    QSemaphore releaseWorker;
    bool holdQueued = false;
    quint64 defaultSize = 0;
    TileCache cache;
    connect(&cache, &TileCache::updateTotals, this, [&](quint32, quint64, quint32, quint64 size) {
        defaultSize = size;
        if (!holdQueued) {
            CacheTask* const hold = new CacheTask(QStringLiteral("Hold"), [&releaseWorker](CacheWorker&, CacheTask&) {
                (void) releaseWorker.tryAcquire(1, TestTimeout::mediumMs());
                return QString();
            });
            holdQueued = cache.addTask(hold);
        }
    });

    cache.init(path);
    QTRY_VERIFY_WITH_TIMEOUT(holdQueued, TestTimeout::mediumMs());
    QVERIFY(defaultSize > kMaxCacheBytes);

    // Deleted after the prune by shutdown(), which releases the worker only once the prune is gone
    CacheTask* const lastTask = tileCacheIdleTask();
    connect(lastTask, &QObject::destroyed, this, [&releaseWorker] { releaseWorker.release(); });
    QVERIFY(cache.addTask(lastTask));
    cache.shutdown();

    cache.init(path);
    QTRY_VERIFY_WITH_TIMEOUT(defaultSize <= kMaxCacheBytes, TestTimeout::mediumMs());

    cache.close();
}

void TileCacheTest::_testGivesOldTilesTheConfiguredSource()
{
    Fact* const customUrl = SettingsManager::instance()->appSettings()->customURL();
    TestFixtures::SettingsFixture saved;

    QTemporaryDir tempDir;
    const QString databasePath = tempDir.filePath("version2.db");
    const QByteArray image("tile of the custom URL");
    QVERIFY(createVersion2TileCacheWithCustomUrlTile(databasePath, 1, 2, 3, image));

    // The cache takes the source from the settings for the worker thread, which migrates the database
    saved.setFactValue(customUrl, kTileCacheTestSourceA);
    TileCache cache;
    cache.init(databasePath);

    const auto addTask = [&cache](CacheTask* task) { return cache.addTask(task); };
    const std::optional<CacheTile> tile =
        tileCacheFetchTile(this, addTask, tileCacheCustomUrlProvider()->getTileHash(1, 2, 3));
    QVERIFY(tile.has_value());
    QCOMPARE(tile->img, image);

    cache.close();
}

UT_REGISTER_TEST(TileCacheTest, TestLabel::Unit)
