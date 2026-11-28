#include "CacheWorkerTest.h"

#include <atomic>
#include <cmath>
#include <memory>
#include <optional>

#include <QtCore/QSemaphore>
#include <QtCore/QTemporaryDir>
#include <QtPositioning/QGeoCoordinate>
#include <QtTest/QTest>

#include "AppSettings.h"
#include "BaseClasses/TerrainTest.h"
#include "CacheTasks.h"
#include "CacheTile.h"
#include "CacheWorker.h"
#include "CachedTileSet.h"
#include "CopernicusTileFormat.h"
#include "ElevationMapProvider.h"
#include "Fixtures/RAIIFixtures.h"
#include "MapProvider.h"
#include "MapProviderRegistry.h"
#include "SettingsManager.h"
#include "TerrainTile.h"
#include "TileCacheTestHelpers.h"
#include "TileRegion.h"
#include "TileSetTasks.h"

namespace {

/// Queues tasks for \a worker
TileCacheAddTask cacheWorkerTasks(CacheWorker& worker)
{
    return [&worker](CacheTask* task) { return worker.enqueueTask(task); };
}

const QString kCacheWorkerSetNamesSql = QStringLiteral("SELECT name FROM TileSets ORDER BY setID");

CacheTile cacheWorkerTestTile(const QString& hash, const QByteArray& image)
{
    return CacheTile(hash, image, QStringLiteral("png"), kTileCacheTestMapId);
}

}  // namespace

void CacheWorkerTest::initTestCase()
{
    UnitTest::initTestCase();
    QCOMPARE(MapProviderRegistry::getMapIdFromProviderType(kTileCacheTestProviderType), kTileCacheTestMapId);
}

bool CacheWorkerTest::_startWorker(CacheWorker& worker, int timeoutMs)
{
    bool totalsReceived = false;
    // Declared after what the lambda writes: a signal still queued when this function returns is dropped with it
    const QObject totalsContext;
    auto conn = connect(
        &worker, &CacheWorker::updateTotals, &totalsContext, [&]() { totalsReceived = true; }, Qt::QueuedConnection);

    if (!worker.enqueueTask(CacheTasks::init())) {
        disconnect(conn);
        return false;
    }

    const bool ok = UnitTest::waitForCondition([&]() { return totalsReceived; }, timeoutMs,
                                               QStringLiteral("CacheWorker::updateTotals"));
    disconnect(conn);
    return ok;
}

void CacheWorkerTest::_testStartAndStop()
{
    QTemporaryDir tempDir;
    CacheWorker worker;
    worker.setDatabaseFile(tempDir.filePath("start_stop.db"));
    QVERIFY(!worker.isRunning());

    QVERIFY(_startWorker(worker));
    QVERIFY(worker.isRunning());

    worker.stop();
    QVERIFY(worker.wait(TestTimeout::mediumMs()));
    QVERIFY(!worker.isRunning());
}

void CacheWorkerTest::_testEnqueueBeforeInit()
{
    QTemporaryDir tempDir;
    CacheWorker worker;
    worker.setDatabaseFile(tempDir.filePath("pre_init.db"));

    // A task with work is refused, and ends with an error at once
    CacheTask* const task = tileCacheIdleTask();
    QString error;
    connect(task, &CacheTask::finished, this, [&error](const QString& taskError) { error = taskError; });
    QVERIFY(!worker.enqueueTask(task));
    QVERIFY(!error.isEmpty());
    QVERIFY(!worker.isRunning());
}

void CacheWorkerTest::_testUpdateTotalsOnInit()
{
    QTemporaryDir tempDir;
    CacheWorker worker;
    worker.setDatabaseFile(tempDir.filePath("totals.db"));

    quint32 totalTiles = UINT32_MAX;
    quint64 totalSize = UINT64_MAX;
    const QObject totalsContext;
    auto conn = connect(
        &worker, &CacheWorker::updateTotals, &totalsContext,
        [&](quint32 tiles, quint64 size, quint32, quint64) {
            totalTiles = tiles;
            totalSize = size;
        },
        Qt::QueuedConnection);

    QVERIFY(worker.enqueueTask(CacheTasks::init()));
    QTRY_VERIFY_WITH_TIMEOUT(totalTiles != UINT32_MAX, TestTimeout::mediumMs());
    disconnect(conn);

    QCOMPARE(totalTiles, static_cast<quint32>(0));
    QCOMPARE(totalSize, static_cast<quint64>(0));

    worker.stop();
    worker.wait(TestTimeout::mediumMs());
}

void CacheWorkerTest::_testSaveAndFetchTile()
{
    QTemporaryDir tempDir;
    CacheWorker worker;
    worker.setDatabaseFile(tempDir.filePath("save_fetch.db"));
    QVERIFY(_startWorker(worker));

    QVERIFY(worker.enqueueTask(CacheTasks::saveTile(cacheWorkerTestTile(QStringLiteral("h1"), "tile_data"))));

    // Fetch — FIFO guarantees save completes first
    const std::optional<CacheTile> fetched = tileCacheFetchTile(this, cacheWorkerTasks(worker), QStringLiteral("h1"));
    QVERIFY2(fetched.has_value(), "Expected tile to be fetched");
    QCOMPARE(fetched->hash, QStringLiteral("h1"));
    QCOMPARE(fetched->img, QByteArray("tile_data"));
    QCOMPARE(fetched->format, QStringLiteral("png"));

    worker.stop();
    worker.wait(TestTimeout::mediumMs());
}

void CacheWorkerTest::_testFetchTileNotFound()
{
    QTemporaryDir tempDir;
    CacheWorker worker;
    worker.setDatabaseFile(tempDir.filePath("not_found.db"));
    QVERIFY(_startWorker(worker));

    // The task ends, and has not failed: the cache lacks the tile
    const std::optional<QString> error = tileCacheRunTask(
        this, cacheWorkerTasks(worker), CacheTasks::fetchTile(QStringLiteral("nonexistent"), this, [](auto) {}));
    QCOMPARE(error, QString());
    QVERIFY(!tileCacheFetchTile(this, cacheWorkerTasks(worker), QStringLiteral("nonexistent")).has_value());

    worker.stop();
    worker.wait(TestTimeout::mediumMs());
}

void CacheWorkerTest::_testFetchMapTileMissServesFakeTile()
{
    QTemporaryDir tempDir;
    CacheWorker worker;
    worker.setDatabaseFile(tempDir.filePath("map_miss_fake.db"));
    QVERIFY(_startWorker(worker));

    // Cache miss on a valid map provider hash: under unit tests the worker must
    // serve a fake tile instead of erroring, so the map UI never falls back to
    // real network fetches.
    const QString hash =
        MapProviderRegistry::getMapProviderFromProviderType(kTileCacheTestProviderType)->getTileHash(1, 2, 3);
    const std::optional<CacheTile> fetched = tileCacheFetchTile(this, cacheWorkerTasks(worker), hash);

    QVERIFY2(fetched.has_value(), "Expected fake tile on map tile cache miss under unit tests");
    QCOMPARE(fetched->hash, hash);
    QCOMPARE(fetched->mapId, kTileCacheTestMapId);
    QVERIFY(!fetched->img.isEmpty());
    QCOMPARE(fetched->format, QStringLiteral("png"));

    worker.stop();
    worker.wait(TestTimeout::mediumMs());
}

void CacheWorkerTest::_testFetchElevationTileMissServesSyntheticTerrain()
{
    QTemporaryDir tempDir;
    CacheWorker worker;
    worker.setDatabaseFile(tempDir.filePath("elev_miss.db"));
    QVERIFY(_startWorker(worker));

    // An elevation tile cache miss serves a synthetic terrain tile built from the
    // unit test terrain regions, with 0 height everywhere else.
    const QString elevationType = QString::fromLatin1(CopernicusElevationProvider::kProviderKey);
    const SharedMapProvider elevationProvider = MapProviderRegistry::getMapProviderFromProviderType(elevationType);
    const QGeoCoordinate flatCenter = UnitTestTerrainData::flat10Region.center();
    const int x =
        static_cast<int>(std::floor((flatCenter.longitude() + 180.0) / CopernicusTileFormat::kTileSizeDegrees));
    const int y = static_cast<int>(std::floor((flatCenter.latitude() + 90.0) / CopernicusTileFormat::kTileSizeDegrees));
    const QString hash = elevationProvider->getTileHash(x, y, 1);

    const std::optional<CacheTile> fetched = tileCacheFetchTile(this, cacheWorkerTasks(worker), hash);
    QVERIFY2(fetched.has_value(), "Expected synthetic terrain tile on elevation cache miss under unit tests");
    QCOMPARE(fetched->hash, hash);
    QCOMPARE(fetched->mapId, elevationProvider->mapId());
    QCOMPARE(fetched->format, QStringLiteral("bin"));

    const TerrainTile tile(fetched->img);
    QVERIFY(tile.isValid());
    QCOMPARE(tile.elevation(flatCenter), UnitTestTerrainData::Flat10Region::amslElevation);

    // A tile far away from the test regions serves 0 heights
    const QGeoCoordinate elsewhere(0.005, 0.005);
    const int farX =
        static_cast<int>(std::floor((elsewhere.longitude() + 180.0) / CopernicusTileFormat::kTileSizeDegrees));
    const int farY =
        static_cast<int>(std::floor((elsewhere.latitude() + 90.0) / CopernicusTileFormat::kTileSizeDegrees));
    const std::optional<CacheTile> farFetched =
        tileCacheFetchTile(this, cacheWorkerTasks(worker), elevationProvider->getTileHash(farX, farY, 1));
    QVERIFY2(farFetched.has_value(), "Expected synthetic terrain tile for coordinate outside test regions");

    const TerrainTile farTile(farFetched->img);
    QVERIFY(farTile.isValid());
    QCOMPARE(farTile.elevation(elsewhere), 0.0);

    worker.stop();
    worker.wait(TestTimeout::mediumMs());
}

void CacheWorkerTest::_testFetchTileSets()
{
    QTemporaryDir tempDir;
    CacheWorker worker;
    worker.setDatabaseFile(tempDir.filePath("tile_sets.db"));
    QVERIFY(_startWorker(worker));

    QList<CachedTileSet*> sets;
    const auto deleteSets = qScopeGuard([&sets] { qDeleteAll(sets); });
    // Declared after what the handler writes: a handler still queued when this function returns is dropped with it
    const QObject fetchContext;
    const std::optional<QString> error = tileCacheRunTask(
        this, cacheWorkerTasks(worker),
        TileSetTasks::fetchTileSets(&fetchContext, [&sets](const QList<CachedTileSet*>& fetched) { sets = fetched; }));
    QCOMPARE(error, QString());

    QCOMPARE(sets.size(), 1);
    QVERIFY(sets[0]->defaultSet());
    QCOMPARE(sets[0]->name(), QStringLiteral("Default Tile Set"));

    worker.stop();
    worker.wait(TestTimeout::mediumMs());
}

void CacheWorkerTest::_testCreateAndDeleteTileSet()
{
    QTemporaryDir tempDir;
    CacheWorker worker;
    worker.setDatabaseFile(tempDir.filePath("create_delete.db"));
    QVERIFY(_startWorker(worker));

    auto* tileSet = new CachedTileSet(TileSetRecord{.name = QStringLiteral("Test Set"),
                                                    .mapName = QStringLiteral("TestMap"),
                                                    .topleftLat = 37.0,
                                                    .topleftLon = -122.0,
                                                    .bottomRightLat = 36.0,
                                                    .bottomRightLon = -121.0,
                                                    .minZoom = 5,
                                                    .maxZoom = 5,
                                                    .mapId = kTileCacheTestMapId,
                                                    .numTiles = 1});

    // The task owns the set until it is saved
    std::unique_ptr<CachedTileSet> savedSet;
    const QObject createContext;
    const std::optional<QString> createError = tileCacheRunTask(
        this, cacheWorkerTasks(worker),
        TileSetTasks::createTileSet(tileSet, &createContext, [&savedSet](CachedTileSet* set) { savedSet.reset(set); }));
    QCOMPARE(createError, QString());
    QVERIFY2(savedSet, "Expected tile set to be created");
    QVERIFY(savedSet->id() != 0);

    QCOMPARE(tileCacheRunTask(this, cacheWorkerTasks(worker), TileSetTasks::deleteTileSet(savedSet->id())), QString());

    QCOMPARE(tileCacheWorkerRows(this, cacheWorkerTasks(worker), kCacheWorkerSetNamesSql),
             QStringList{QStringLiteral("Default Tile Set")});

    worker.stop();
    worker.wait(TestTimeout::mediumMs());
}

void CacheWorkerTest::_testPruneCache()
{
    QTemporaryDir tempDir;
    CacheWorker worker;
    worker.setDatabaseFile(tempDir.filePath("prune.db"));
    QVERIFY(_startWorker(worker));

    for (int i = 0; i < 10; i++) {
        QVERIFY(worker.enqueueTask(
            CacheTasks::saveTile(cacheWorkerTestTile(QStringLiteral("pr_%1").arg(i), QByteArray(100, 'P')))));
    }

    QCOMPARE(tileCacheRunTask(this, cacheWorkerTasks(worker), CacheTasks::pruneCache(500)), QString());

    // Trigger totals to verify count decreased
    quint32 remaining = UINT32_MAX;
    const QObject totalsContext;
    auto conn = connect(
        &worker, &CacheWorker::updateTotals, &totalsContext,
        [&](quint32 tiles, quint64, quint32, quint64) { remaining = tiles; }, Qt::QueuedConnection);
    QVERIFY(worker.enqueueTask(CacheTasks::init()));
    QTRY_VERIFY_WITH_TIMEOUT(remaining != UINT32_MAX, TestTimeout::mediumMs());
    disconnect(conn);

    QVERIFY2(remaining < 10,
             qPrintable(QStringLiteral("Expected fewer than 10 tiles after prune, got %1").arg(remaining)));

    worker.stop();
    worker.wait(TestTimeout::mediumMs());
}

// A tile that a map fetches from the cache is in use: a prune deletes the tile that nothing asked for
void CacheWorkerTest::_testPruneKeepsTileThatWasFetched()
{
    QTemporaryDir tempDir;
    const QString path = tempDir.filePath("used.db");
    CacheWorker worker;
    worker.setDatabaseFile(path);
    QVERIFY(_startWorker(worker));

    const QByteArray data(100, 'U');
    for (const QString& hash : {QStringLiteral("fetched"), QStringLiteral("not_fetched")}) {
        QCOMPARE(
            tileCacheRunTask(this, cacheWorkerTasks(worker), CacheTasks::saveTile(cacheWorkerTestTile(hash, data))),
            QString());
    }
    // Both were saved long ago, the one that is fetched before the other
    QVERIFY(tileCacheExec(path,
                          QStringLiteral("UPDATE Tiles SET date = CASE hash WHEN 'fetched' THEN 1000 ELSE 2000 END")));
    QVERIFY(tileCacheFetchTile(this, cacheWorkerTasks(worker), QStringLiteral("fetched")).has_value());

    QCOMPARE(tileCacheRunTask(this, cacheWorkerTasks(worker), CacheTasks::pruneCache(data.size())), QString());
    QVERIFY(tileCacheFetchTile(this, cacheWorkerTasks(worker), QStringLiteral("fetched")).has_value());
    QVERIFY(!tileCacheFetchTile(this, cacheWorkerTasks(worker), QStringLiteral("not_fetched")).has_value());

    worker.stop();
    worker.wait(TestTimeout::mediumMs());
}

void CacheWorkerTest::_testResetDatabase()
{
    QTemporaryDir tempDir;
    CacheWorker worker;
    worker.setDatabaseFile(tempDir.filePath("reset.db"));
    QVERIFY(_startWorker(worker));

    QVERIFY(worker.enqueueTask(CacheTasks::saveTile(cacheWorkerTestTile(QStringLiteral("r1"), "data"))));

    QCOMPARE(tileCacheRunTask(this, cacheWorkerTasks(worker), CacheTasks::resetCache()), QString());

    // Verify saved tile is gone
    QVERIFY(!tileCacheFetchTile(this, cacheWorkerTasks(worker), QStringLiteral("r1")).has_value());

    QVERIFY(worker.isRunning());

    worker.stop();
    worker.wait(TestTimeout::mediumMs());
}

void CacheWorkerTest::_testStopWhileProcessing()
{
    QTemporaryDir tempDir;
    CacheWorker worker;
    worker.setDatabaseFile(tempDir.filePath("stop_busy.db"));
    QVERIFY(_startWorker(worker));

    for (int i = 0; i < 100; i++) {
        worker.enqueueTask(
            CacheTasks::saveTile(cacheWorkerTestTile(QStringLiteral("stop_%1").arg(i), QByteArray(50, 'S'))));
    }

    worker.stop();
    QVERIFY(worker.wait(TestTimeout::mediumMs()));
    QVERIFY(!worker.isRunning());
}

void CacheWorkerTest::_testTaskQueuedAsWorkerQuitsIsRefused()
{
    QTemporaryDir tempDir;

    // Used on the worker thread, so declared before the worker, which uses them until it is destroyed
    QSemaphore working;
    QSemaphore mayEnd;
    bool lateAccepted = true;
    QString lateError;

    CacheWorker worker;
    worker.setDatabaseFile(tempDir.filePath("quit.db"));
    QVERIFY(_startWorker(worker));

    // The worker is told to stop while a task holds it, so that the task queued after that is left when it quits
    QVERIFY(worker.enqueueTask(new CacheTask(QStringLiteral("Hold"), [&](CacheWorker&, CacheTask&) {
        working.release();
        mayEnd.acquire();
        return QString();
    })));
    QVERIFY(working.tryAcquire(1, TestTimeout::mediumMs()));
    worker.stop();

    // The worker ends the task that is left after its last look at the queue, and before its thread has quit: a task
    // queued from there, on the worker thread, is one that nobody would run
    CacheTask* const left = tileCacheIdleTask();
    CacheTask* const late = tileCacheIdleTask();
    connect(
        late, &CacheTask::finished, this, [&lateError](const QString& error) { lateError = error; },
        Qt::DirectConnection);
    connect(left, &CacheTask::finished, this, [&] { lateAccepted = worker.enqueueTask(late); }, Qt::DirectConnection);
    QVERIFY(worker.enqueueTask(left));
    mayEnd.release();
    QVERIFY(worker.wait(TestTimeout::mediumMs()));

    QVERIFY(!lateAccepted);
    QCOMPARE(lateError, QStringLiteral("Worker shutting down"));
}

void CacheWorkerTest::_testCreateTileSetKeepsSourceOfItsCreation()
{
    Fact* const customUrl = SettingsManager::instance()->appSettings()->customURL();
    TestFixtures::SettingsFixture saved;

    QTemporaryDir tempDir;
    CacheWorker worker;
    worker.setDatabaseFile(tempDir.filePath("set_source.db"));
    QVERIFY(_startWorker(worker));

    // A region of one tile
    const TileRegion region = tileCacheCustomUrlProvider()->getTileCount(5, -122.0, 37.0, -121.0, 36.0);
    QCOMPARE(region.tileCount, 1U);
    auto* const tileSet = new CachedTileSet(TileSetRecord{.name = QStringLiteral("Custom URL Set"),
                                                          .mapName = kTileCacheCustomUrlType,
                                                          .topleftLat = 37.0,
                                                          .topleftLon = -122.0,
                                                          .bottomRightLat = 36.0,
                                                          .bottomRightLon = -121.0,
                                                          .minZoom = 5,
                                                          .maxZoom = 5,
                                                          .mapId = tileCacheCustomUrlProvider()->mapId(),
                                                          .numTiles = 1});

    // The set is of the source configured when it is created. The worker computes the keys of its tiles later, and
    // cannot read the settings.
    std::unique_ptr<CachedTileSet> savedSet;
    const QObject createContext;
    saved.setFactValue(customUrl, kTileCacheTestSourceA);
    CacheTask* const createTask =
        TileSetTasks::createTileSet(tileSet, &createContext, [&savedSet](CachedTileSet* set) { savedSet.reset(set); });
    saved.setFactValue(customUrl, kTileCacheTestSourceB);

    QCOMPARE(tileCacheRunTask(this, cacheWorkerTasks(worker), createTask), QString());
    QVERIFY(savedSet);

    QStringList hashes;
    const QObject listContext;
    CacheTask* const listTask =
        TileSetTasks::tileDownloadList(savedSet->id(), 10, &listContext, [&hashes](const QList<DownloadTile>& tiles) {
            for (const DownloadTile& tile : tiles) {
                hashes.append(tile.hash);
            }
        });
    QCOMPARE(tileCacheRunTask(this, cacheWorkerTasks(worker), listTask), QString());
    QCOMPARE(hashes, QStringList{tileCacheCustomUrlProvider()->getTileHash(region.tileX0, region.tileY0, 5,
                                                                           kTileCacheTestSourceA)});

    worker.stop();
    worker.wait(TestTimeout::mediumMs());
}

void CacheWorkerTest::_testImportGivesOldTilesTheConfiguredSource()
{
    Fact* const customUrl = SettingsManager::instance()->appSettings()->customURL();
    TestFixtures::SettingsFixture saved;

    QTemporaryDir tempDir;
    const QString importPath = tempDir.filePath("version2.db");
    const QByteArray image("tile of the custom URL");
    QVERIFY(createVersion2TileCacheWithCustomUrlTile(importPath, 1, 2, 3, image));

    saved.setFactValue(customUrl, kTileCacheTestSourceA);
    CacheWorker worker;
    worker.setDatabaseFile(tempDir.filePath("import_source.db"));
    QVERIFY(_startWorker(worker));

    // The source configured when the file is imported, not the one of the time the cache was opened
    saved.setFactValue(customUrl, kTileCacheTestSourceB);
    QCOMPARE(tileCacheRunTask(this, cacheWorkerTasks(worker), TileSetTasks::importSets(importPath, false)), QString());

    const std::optional<CacheTile> tile =
        tileCacheFetchTile(this, cacheWorkerTasks(worker), tileCacheCustomUrlProvider()->getTileHash(1, 2, 3));
    QVERIFY(tile.has_value());
    QCOMPARE(tile->img, image);

    worker.stop();
    worker.wait(TestTimeout::mediumMs());
}

// The set of an MBTiles file is named after the file. A file that is named by its extension alone has no name to
// give: its set gets that of the map, not an empty one.
void CacheWorkerTest::_testImportNamesSetOfMBTilesFileWithoutName()
{
    QTemporaryDir tempDir;
    const QString importPath = tempDir.filePath(".mbtiles");
    QVERIFY(tileCacheCreateMBTiles(importPath, QString(), QStringLiteral("(1, 0, 0, x'89504E470D0A1A0A')")));

    CacheWorker worker;
    worker.setDatabaseFile(tempDir.filePath("import_nameless.db"));
    QVERIFY(_startWorker(worker));
    QCOMPARE(tileCacheRunTask(this, cacheWorkerTasks(worker), TileSetTasks::importSets(importPath, false)), QString());

    QCOMPARE(tileCacheWorkerRows(this, cacheWorkerTasks(worker), kCacheWorkerSetNamesSql),
             QStringList({QStringLiteral("Default Tile Set"), QStringLiteral("MBTiles Imported")}));

    worker.stop();
    worker.wait(TestTimeout::mediumMs());
}

void CacheWorkerTest::_testImportOfMBTilesFileReplacesCacheWhenAsked_data()
{
    QTest::addColumn<bool>("replace");

    QTest::newRow("append to existing sets") << false;
    QTest::newRow("replace existing sets") << true;
}

// The choice of the user between adding a file to the cache and replacing the cache with it holds for an MBTiles file
// as for a file of tile sets
void CacheWorkerTest::_testImportOfMBTilesFileReplacesCacheWhenAsked()
{
    QFETCH(bool, replace);

    QTemporaryDir tempDir;
    const QString importPath = tempDir.filePath("field.mbtiles");
    QVERIFY(tileCacheCreateMBTiles(importPath, QString(), QStringLiteral("(1, 0, 0, x'89504E470D0A1A0A')")));

    CacheWorker worker;
    worker.setDatabaseFile(tempDir.filePath("import_replace.db"));
    QVERIFY(_startWorker(worker));
    QVERIFY(worker.enqueueTask(CacheTasks::saveTile(cacheWorkerTestTile(QStringLiteral("h1"), "tile_data"))));

    QCOMPARE(tileCacheRunTask(this, cacheWorkerTasks(worker), TileSetTasks::importSets(importPath, replace)),
             QString());
    QCOMPARE(tileCacheFetchTile(this, cacheWorkerTasks(worker), QStringLiteral("h1")).has_value(), !replace);

    worker.stop();
    worker.wait(TestTimeout::mediumMs());
}

void CacheWorkerTest::_testFreePagesReleasedWhenIdle_data()
{
    QTest::addColumn<QString>("task");
    QTest::newRow("tile set deleted") << QStringLiteral("delete set");
    QTest::newRow("cache pruned") << QStringLiteral("prune");
    QTest::newRow("file imported") << QStringLiteral("import");
}

void CacheWorkerTest::_testFreePagesReleasedWhenIdle()
{
    QFETCH(QString, task);

    // A cache with unused pages, as an earlier run leaves it that quit before it had released them
    constexpr int tileCount = 20;
    QTemporaryDir tempDir;
    const QString path = tempDir.filePath("tiles.db");
    quint64 deletedSetID = 0;
    quint64 setID = 0;
    QVERIFY(tileCacheAddLargeTileSet(path, QStringLiteral("set"), tileCount, &setID));
    QVERIFY(tileCacheAddLargeTileSet(path, QStringLiteral("default"), tileCount, nullptr));
    QVERIFY(tileCacheAddLargeTileSet(path, QStringLiteral("deleted"), tileCount, &deletedSetID, true));
    QVERIFY(tileCacheFreePages(path) > 0);

    // The unused pages at the time the task reports that it is done, read on the worker thread: the task does not
    // wait for their release. Declared before the worker, which uses them until it is destroyed.
    std::atomic<qint64> pagesWhenDone = -1;
    const auto readPages = [&pagesWhenDone, &path] { pagesWhenDone = tileCacheFreePages(path); };

    // The worker releases the pages once it has started, without a task to make it. From here on it has nothing
    // left to release until a task frees pages.
    CacheWorker worker;
    worker.setDatabaseFile(path);
    QVERIFY(_startWorker(worker));
    QTRY_COMPARE_WITH_TIMEOUT(tileCacheFreePages(path), 0, TestTimeout::mediumMs());

    CacheTask* cacheTask = nullptr;
    if (task == QStringLiteral("delete set")) {
        cacheTask = TileSetTasks::deleteTileSet(setID);
    } else if (task == QStringLiteral("prune")) {
        cacheTask = CacheTasks::pruneCache(static_cast<quint64>(tileCount) * kTileCacheLargeTileSize);
    } else {
        // The file that replaces the cache has unused pages of its own
        const QString importPath = tempDir.filePath("import.db");
        quint64 importSetID = 0;
        QVERIFY(tileCacheAddLargeTileSet(importPath, QStringLiteral("deleted"), tileCount, &importSetID, true));
        cacheTask = TileSetTasks::importSets(importPath, true);
    }
    connect(cacheTask, &CacheTask::finished, this, readPages, Qt::DirectConnection);
    QVERIFY(worker.enqueueTask(cacheTask));
    QTRY_VERIFY_WITH_TIMEOUT(pagesWhenDone >= 0, TestTimeout::mediumMs());
    QVERIFY(pagesWhenDone > 0);

    QTRY_COMPARE_WITH_TIMEOUT(tileCacheFreePages(path), 0, TestTimeout::mediumMs());

    worker.stop();
    worker.wait(TestTimeout::mediumMs());
}

void CacheWorkerTest::_testTileFetchedBeforeFreePagesReleased()
{
    // A set that takes several batches to release, and a tile that stays in the cache
    QTemporaryDir tempDir;
    const QString path = tempDir.filePath("tiles.db");
    quint64 setID = 0;
    QVERIFY(tileCacheAddLargeTileSet(path, QStringLiteral("set"), 64, &setID));
    QVERIFY(tileCacheAddLargeTileSet(path, QStringLiteral("default"), 1, nullptr));

    // Written on the worker thread, so declared before the worker, which uses them until it is destroyed
    std::atomic<qint64> pagesWhenFetched = -1;
    QByteArray image;

    CacheWorker worker;
    worker.setDatabaseFile(path);
    QVERIFY(_startWorker(worker));

    // The tile is asked for as soon as the set is deleted, on the worker thread, so that the request is queued
    // before the worker looks for something to do. The unused pages are read there when the tile arrives.
    CacheTask* const fetchTask = new CacheTask(QStringLiteral("Fetch Tile"), [&](CacheWorker& cache, CacheTask&) {
        if (const std::unique_ptr<CacheTile> tile = cache.tile(QStringLiteral("default_0"))) {
            image = tile->img;
        }
        pagesWhenFetched = tileCacheFreePages(path);
        return QString();
    });
    CacheTask* const deleteTask = TileSetTasks::deleteTileSet(setID);
    connect(
        deleteTask, &CacheTask::finished, this, [&worker, fetchTask] { (void) worker.enqueueTask(fetchTask); },
        Qt::DirectConnection);
    QVERIFY(worker.enqueueTask(deleteTask));

    // The tile does not wait for the pages of the set, which are released afterwards
    QTRY_VERIFY_WITH_TIMEOUT(pagesWhenFetched >= 0, TestTimeout::mediumMs());
    QCOMPARE(image, QByteArray(kTileCacheLargeTileSize, 'T'));
    QVERIFY(pagesWhenFetched > 0);
    QTRY_COMPARE_WITH_TIMEOUT(tileCacheFreePages(path), 0, TestTimeout::mediumMs());

    worker.stop();
    worker.wait(TestTimeout::mediumMs());
}

UT_REGISTER_TEST(CacheWorkerTest, TestLabel::Unit)
