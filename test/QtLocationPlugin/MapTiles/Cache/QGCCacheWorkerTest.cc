#include "QGCCacheWorkerTest.h"

#include <atomic>
#include <cmath>
#include <memory>
#include <optional>

#include <QtCore/QTemporaryDir>
#include <QtPositioning/QGeoCoordinate>
#include <QtSql/QSqlQuery>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "AppSettings.h"
#include "BaseClasses/TerrainTest.h"
#include "ElevationMapProvider.h"
#include "Fixtures/RAIIFixtures.h"
#include "MapProvider.h"
#include "MapsSettings.h"
#include "QGCCacheTile.h"
#include "QGCCacheWorker.h"
#include "QGCCachedTileSet.h"
#include "QGCMapEngine.h"
#include "QGCMapTasks.h"
#include "QGCTileCacheDatabase.h"
#include "QGCTileSet.h"
#include "SettingsManager.h"
#include "TerrainTile.h"
#include "TerrainTileCopernicus.h"
#include "TileCacheTestHelpers.h"
#include "UrlFactory.h"

static const QString kTestProviderType = QStringLiteral("Bing Road");

namespace {

/// The provider that fetches from the URL template the user sets
const QString kCacheWorkerCustomUrlType = QStringLiteral("CustomURL Custom");
const QString kCacheWorkerSourceA = QStringLiteral("https://a.example/{z}/{x}/{y}.png");
const QString kCacheWorkerSourceB = QStringLiteral("https://b.example/{z}/{x}/{y}.png");

SharedMapProvider cacheWorkerCustomUrlProvider()
{
    return UrlFactory::getMapProviderFromProviderType(kCacheWorkerCustomUrlType);
}

/// A cache of schema version 2, where the key of a tile did not tell its source, with a tile of the custom URL
/// provider. Version 2 has the tables of today.
bool createVersion2CacheWithCustomUrlTile(const QString& path, int x, int y, int z, const QByteArray& image)
{
    QGCTileCacheDatabase db(path);
    if (!db.init() || !db.connectDB() ||
        !db.saveTile(cacheWorkerCustomUrlProvider()->getTileHash(x, y, z, QString()), QStringLiteral("png"), image,
                     kCacheWorkerCustomUrlType, QGCTileCacheDatabase::kInvalidTileSet)) {
        return false;
    }
    QSqlQuery query(db.database());
    return query.exec(QStringLiteral("PRAGMA user_version = 2"));
}

constexpr int kCacheWorkerLargeTileSize = 64 * 1024;

/// Adds a tile set with tiles that take hundreds of pages together to the cache at \a path. Its tiles go to the
/// default set instead when \a setID is null. With \a deleteSet the set is deleted again, which leaves its pages
/// unused in the file.
bool addLargeTileSet(const QString& path, const QString& name, int tileCount, quint64* setID, bool deleteSet = false)
{
    QGCTileCacheDatabase db(path);
    if (!db.init() || !db.connectDB()) {
        return false;
    }

    quint64 id = QGCTileCacheDatabase::kInvalidTileSet;
    if (setID) {
        QSqlQuery query(db.database());
        if (!query.prepare(QStringLiteral("INSERT INTO TileSets(name) VALUES(?)"))) {
            return false;
        }
        query.addBindValue(name);
        if (!query.exec()) {
            return false;
        }
        id = query.lastInsertId().toULongLong();
        *setID = id;
    }

    const QByteArray image(kCacheWorkerLargeTileSize, 'T');
    for (int i = 0; i < tileCount; i++) {
        if (!db.saveTile(QStringLiteral("%1_%2").arg(name).arg(i), QStringLiteral("png"), image, kTestProviderType,
                         id)) {
            return false;
        }
    }
    return !deleteSet || db.deleteTileSet(id);
}

/// Queues tasks for \a worker
std::function<bool(QGCMapTask*)> cacheWorkerTasks(QGCCacheWorker& worker)
{
    return [&worker](QGCMapTask* task) { return worker.enqueueTask(task); };
}

/// A task that the worker has nothing to do for
QGCMapTask* cacheWorkerIdleTask()
{
    return new QGCMapTask(QStringLiteral("Idle"), [](QGCCacheWorker&, QGCMapTask&) { return QString(); });
}

QGCCacheTile cacheWorkerTestTile(const QString& hash, const QByteArray& image)
{
    return QGCCacheTile(hash, image, QStringLiteral("png"), kTestProviderType);
}

}  // namespace

void QGCCacheWorkerTest::initTestCase()
{
    UnitTest::initTestCase();
    QVERIFY2(UrlFactory::getMapIdFromProviderType(kTestProviderType) != -1,
             ("Provider type '" + kTestProviderType.toLatin1() + "' not available in this build").constData());
}

bool QGCCacheWorkerTest::_startWorker(QGCCacheWorker& worker, int timeoutMs)
{
    bool totalsReceived = false;
    // Declared after what the lambda writes: a signal still queued when this function returns is dropped with it
    const QObject totalsContext;
    auto conn = connect(
        &worker, &QGCCacheWorker::updateTotals, &totalsContext, [&]() { totalsReceived = true; }, Qt::QueuedConnection);

    if (!worker.enqueueTask(QGCMapTasks::init())) {
        disconnect(conn);
        return false;
    }

    const bool ok = UnitTest::waitForCondition([&]() { return totalsReceived; }, timeoutMs,
                                               QStringLiteral("QGCCacheWorker::updateTotals"));
    disconnect(conn);
    return ok;
}

std::optional<QGCCacheTile> QGCCacheWorkerTest::_fetchTile(const AddTask& addTask, const QString& hash)
{
    const auto answer = std::make_shared<std::optional<std::optional<QGCCacheTile>>>();
    QGCMapTask* const task = QGCMapTasks::fetchTile(hash, this, [answer](const QGCCacheTile* tile) {
        *answer = tile ? std::optional<QGCCacheTile>(*tile) : std::nullopt;
    });
    if (!addTask(task)) {
        return std::nullopt;
    }

    (void) UnitTest::waitForCondition([answer] { return answer->has_value(); }, TestTimeout::mediumMs(),
                                      QStringLiteral("answer of the tile cache"));
    return answer->value_or(std::nullopt);
}

std::optional<QString> QGCCacheWorkerTest::_runTask(const AddTask& addTask, QGCMapTask* task)
{
    const auto error = std::make_shared<std::optional<QString>>();
    connect(task, &QGCMapTask::finished, this, [error](const QString& taskError) { *error = taskError; });
    if (!addTask(task)) {
        return std::nullopt;
    }

    (void) UnitTest::waitForCondition([error] { return error->has_value(); }, TestTimeout::mediumMs(),
                                      QStringLiteral("end of the task"));
    return *error;
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

    // A task with work is refused, and ends with an error at once
    QGCMapTask* const task = cacheWorkerIdleTask();
    QString error;
    connect(task, &QGCMapTask::finished, this, [&error](const QString& taskError) { error = taskError; });
    QVERIFY(!worker.enqueueTask(task));
    QVERIFY(!error.isEmpty());
    QVERIFY(!worker.isRunning());
}

void QGCCacheWorkerTest::_testUpdateTotalsOnInit()
{
    QTemporaryDir tempDir;
    QGCCacheWorker worker;
    worker.setDatabaseFile(tempDir.filePath("totals.db"));

    quint32 totalTiles = UINT32_MAX;
    quint64 totalSize = UINT64_MAX;
    const QObject totalsContext;
    auto conn = connect(
        &worker, &QGCCacheWorker::updateTotals, &totalsContext,
        [&](quint32 tiles, quint64 size, quint32, quint64) {
            totalTiles = tiles;
            totalSize = size;
        },
        Qt::QueuedConnection);

    QVERIFY(worker.enqueueTask(QGCMapTasks::init()));
    QTRY_VERIFY_WITH_TIMEOUT(totalTiles != UINT32_MAX, TestTimeout::mediumMs());
    disconnect(conn);

    QCOMPARE(totalTiles, static_cast<quint32>(0));
    QCOMPARE(totalSize, static_cast<quint64>(0));

    worker.stop();
    worker.wait(TestTimeout::mediumMs());
}

void QGCCacheWorkerTest::_testSaveAndFetchTile()
{
    QTemporaryDir tempDir;
    QGCCacheWorker worker;
    worker.setDatabaseFile(tempDir.filePath("save_fetch.db"));
    QVERIFY(_startWorker(worker));

    QVERIFY(worker.enqueueTask(QGCMapTasks::saveTile(cacheWorkerTestTile(QStringLiteral("h1"), "tile_data"))));

    // Fetch — FIFO guarantees save completes first
    const std::optional<QGCCacheTile> fetched = _fetchTile(cacheWorkerTasks(worker), QStringLiteral("h1"));
    QVERIFY2(fetched.has_value(), "Expected tile to be fetched");
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

    // The task ends, with an error
    const std::optional<QString> error =
        _runTask(cacheWorkerTasks(worker), QGCMapTasks::fetchTile(QStringLiteral("nonexistent"), this, [](auto) {}));
    QVERIFY(error.has_value());
    QVERIFY(!error->isEmpty());
    QVERIFY(!_fetchTile(cacheWorkerTasks(worker), QStringLiteral("nonexistent")).has_value());

    worker.stop();
    worker.wait(TestTimeout::mediumMs());
}

void QGCCacheWorkerTest::_testFetchMapTileMissServesFakeTile()
{
    QTemporaryDir tempDir;
    QGCCacheWorker worker;
    worker.setDatabaseFile(tempDir.filePath("map_miss_fake.db"));
    QVERIFY(_startWorker(worker));

    // Cache miss on a valid map provider hash: under unit tests the worker must
    // serve a fake tile instead of erroring, so the map UI never falls back to
    // real network fetches.
    const QString hash = UrlFactory::getMapProviderFromProviderType(kTestProviderType)->getTileHash(1, 2, 3);
    const std::optional<QGCCacheTile> fetched = _fetchTile(cacheWorkerTasks(worker), hash);

    QVERIFY2(fetched.has_value(), "Expected fake tile on map tile cache miss under unit tests");
    QCOMPARE(fetched->hash, hash);
    QCOMPARE(fetched->type, kTestProviderType);
    QVERIFY(!fetched->img.isEmpty());
    QCOMPARE(fetched->format, QStringLiteral("png"));

    worker.stop();
    worker.wait(TestTimeout::mediumMs());
}

void QGCCacheWorkerTest::_testFetchElevationTileMissServesSyntheticTerrain()
{
    QTemporaryDir tempDir;
    QGCCacheWorker worker;
    worker.setDatabaseFile(tempDir.filePath("elev_miss.db"));
    QVERIFY(_startWorker(worker));

    // An elevation tile cache miss serves a synthetic terrain tile built from the
    // unit test terrain regions, with 0 height everywhere else.
    const QString elevationType = QString::fromLatin1(CopernicusElevationProvider::kProviderKey);
    const SharedMapProvider elevationProvider = UrlFactory::getMapProviderFromProviderType(elevationType);
    const QGeoCoordinate flatCenter = UnitTestTerrainData::flat10Region.center();
    const int x =
        static_cast<int>(std::floor((flatCenter.longitude() + 180.0) / TerrainTileCopernicus::kTileSizeDegrees));
    const int y =
        static_cast<int>(std::floor((flatCenter.latitude() + 90.0) / TerrainTileCopernicus::kTileSizeDegrees));
    const QString hash = elevationProvider->getTileHash(x, y, 1);

    const std::optional<QGCCacheTile> fetched = _fetchTile(cacheWorkerTasks(worker), hash);
    QVERIFY2(fetched.has_value(), "Expected synthetic terrain tile on elevation cache miss under unit tests");
    QCOMPARE(fetched->hash, hash);
    QCOMPARE(fetched->type, elevationType);
    QCOMPARE(fetched->format, QStringLiteral("bin"));

    const TerrainTile tile(fetched->img);
    QVERIFY(tile.isValid());
    QCOMPARE(tile.elevation(flatCenter), UnitTestTerrainData::Flat10Region::amslElevation);

    // A tile far away from the test regions serves 0 heights
    const QGeoCoordinate elsewhere(0.005, 0.005);
    const int farX =
        static_cast<int>(std::floor((elsewhere.longitude() + 180.0) / TerrainTileCopernicus::kTileSizeDegrees));
    const int farY =
        static_cast<int>(std::floor((elsewhere.latitude() + 90.0) / TerrainTileCopernicus::kTileSizeDegrees));
    const std::optional<QGCCacheTile> farFetched =
        _fetchTile(cacheWorkerTasks(worker), elevationProvider->getTileHash(farX, farY, 1));
    QVERIFY2(farFetched.has_value(), "Expected synthetic terrain tile for coordinate outside test regions");

    const TerrainTile farTile(farFetched->img);
    QVERIFY(farTile.isValid());
    QCOMPARE(farTile.elevation(elsewhere), 0.0);

    worker.stop();
    worker.wait(TestTimeout::mediumMs());
}

void QGCCacheWorkerTest::_testFetchTileSets()
{
    QTemporaryDir tempDir;
    QGCCacheWorker worker;
    worker.setDatabaseFile(tempDir.filePath("tile_sets.db"));
    QVERIFY(_startWorker(worker));

    QList<QGCCachedTileSet*> sets;
    const auto deleteSets = qScopeGuard([&sets] { qDeleteAll(sets); });
    // Declared after what the handler writes: a handler still queued when this function returns is dropped with it
    const QObject fetchContext;
    const std::optional<QString> error =
        _runTask(cacheWorkerTasks(worker),
                 QGCMapTasks::fetchTileSets(&fetchContext,
                                            [&sets](const QList<QGCCachedTileSet*>& fetched) { sets = fetched; }));
    QCOMPARE(error, QString());

    QCOMPARE(sets.size(), 1);
    QVERIFY(sets[0]->defaultSet());
    QCOMPARE(sets[0]->name(), QStringLiteral("Default Tile Set"));

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

    // The task owns the set until it is saved
    std::unique_ptr<QGCCachedTileSet> savedSet;
    const QObject createContext;
    const std::optional<QString> createError =
        _runTask(cacheWorkerTasks(worker),
                 QGCMapTasks::createTileSet(tileSet, &createContext,
                                            [&savedSet](QGCCachedTileSet* set) { savedSet.reset(set); }));
    QCOMPARE(createError, QString());
    QVERIFY2(savedSet, "Expected tile set to be created");
    QVERIFY(savedSet->id() != 0);

    QCOMPARE(_runTask(cacheWorkerTasks(worker), QGCMapTasks::deleteTileSet(savedSet->id())), QString());

    QList<QGCCachedTileSet*> sets;
    const auto deleteSets = qScopeGuard([&sets] { qDeleteAll(sets); });
    const QObject fetchContext;
    QCOMPARE(_runTask(cacheWorkerTasks(worker),
                      QGCMapTasks::fetchTileSets(&fetchContext,
                                                 [&sets](const QList<QGCCachedTileSet*>& fetched) { sets = fetched; })),
             QString());
    QCOMPARE(sets.size(), 1);
    QVERIFY(sets[0]->defaultSet());

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
        QVERIFY(worker.enqueueTask(
            QGCMapTasks::saveTile(cacheWorkerTestTile(QStringLiteral("pr_%1").arg(i), QByteArray(100, 'P')))));
    }

    QCOMPARE(_runTask(cacheWorkerTasks(worker), QGCMapTasks::pruneCache(500)), QString());

    // Trigger totals to verify count decreased
    quint32 remaining = UINT32_MAX;
    const QObject totalsContext;
    auto conn = connect(
        &worker, &QGCCacheWorker::updateTotals, &totalsContext,
        [&](quint32 tiles, quint64, quint32, quint64) { remaining = tiles; }, Qt::QueuedConnection);
    QVERIFY(worker.enqueueTask(QGCMapTasks::init()));
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

    QVERIFY(worker.enqueueTask(QGCMapTasks::saveTile(cacheWorkerTestTile(QStringLiteral("r1"), "data"))));

    QCOMPARE(_runTask(cacheWorkerTasks(worker), QGCMapTasks::resetCache()), QString());

    // Verify saved tile is gone
    QVERIFY(!_fetchTile(cacheWorkerTasks(worker), QStringLiteral("r1")).has_value());

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
        worker.enqueueTask(
            QGCMapTasks::saveTile(cacheWorkerTestTile(QStringLiteral("stop_%1").arg(i), QByteArray(50, 'S'))));
    }

    worker.stop();
    QVERIFY(worker.wait(TestTimeout::mediumMs()));
    QVERIFY(!worker.isRunning());
}

void QGCCacheWorkerTest::_testMapEngineCloseStopsWorkerAndRefusesInit()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString databasePath = tempDir.filePath(QStringLiteral("engine.db"));

    QGCMapEngine engine;
    engine.init(databasePath);
    QVERIFY(engine.addTask(cacheWorkerIdleTask()));

    engine.close();
    QVERIFY(!engine.addTask(cacheWorkerIdleTask()));

    // A map created during application teardown must not restart the worker.
    engine.init(databasePath);
    QVERIFY(!engine.addTask(cacheWorkerIdleTask()));
}

// GeoMap needs the tile cache without ever creating a QtLocation map, which is what normally initializes it
void QGCCacheWorkerTest::_testMapEngineEnsureInitializedAcceptsTasks()
{
    QGCMapEngine engine;
    QVERIFY(!engine.addTask(cacheWorkerIdleTask()));

    engine.ensureInitialized();
    QVERIFY(engine.addTask(cacheWorkerIdleTask()));

    // Calling it again keeps the running worker
    engine.ensureInitialized();
    QVERIFY(engine.addTask(cacheWorkerIdleTask()));

    engine.close();
}

void QGCCacheWorkerTest::_testMapEnginePrunesAgainAfterFailedPrune()
{
    constexpr quint64 kMaxCacheBytes = 1024 * 1024;
    TestFixtures::SettingsFixture saved;
    saved.setFactValue(SettingsManager::instance()->mapsSettings()->maxCacheDiskSize(), 1);

    // A cache whose default set is over the limit, and that refuses to delete a tile
    QTemporaryDir tempDir;
    const QString path = tempDir.filePath("tiles.db");
    QVERIFY(addLargeTileSet(path, QStringLiteral("default"), 20, nullptr));
    QVERIFY(tileCacheExec(path, QStringLiteral("CREATE TRIGGER refuse_delete BEFORE DELETE ON Tiles "
                                               "BEGIN SELECT RAISE(ABORT, 'refused'); END")));

    quint64 defaultSize = 0;
    QGCMapEngine engine;
    connect(&engine, &QGCMapEngine::updateTotals, this,
            [&defaultSize](quint32, quint64, quint32, quint64 size) { defaultSize = size; });
    const auto addTask = [&engine](QGCMapTask* task) { return engine.addTask(task); };

    // The engine prunes for the totals of the cache it opens, and the prune fails
    engine.init(path);
    QTRY_VERIFY_WITH_TIMEOUT(defaultSize > kMaxCacheBytes, TestTimeout::mediumMs());
    QCOMPARE(_runTask(addTask, cacheWorkerIdleTask()), QString());
    QVERIFY(defaultSize > kMaxCacheBytes);

    // The next change of the cache has the engine prune again
    QVERIFY(tileCacheExec(path, QStringLiteral("DROP TRIGGER refuse_delete")));
    engine.cacheTile(kTestProviderType, QStringLiteral("one_more"), QByteArray(1024, 'M'), QStringLiteral("png"));
    QTRY_VERIFY_WITH_TIMEOUT(defaultSize <= kMaxCacheBytes, TestTimeout::mediumMs());

    engine.close();
}

void QGCCacheWorkerTest::_testCreateTileSetKeepsSourceOfItsCreation()
{
    Fact* const customUrl = SettingsManager::instance()->appSettings()->customURL();
    TestFixtures::SettingsFixture saved;

    QTemporaryDir tempDir;
    QGCCacheWorker worker;
    worker.setDatabaseFile(tempDir.filePath("set_source.db"));
    QVERIFY(_startWorker(worker));

    // A region of one tile
    const QGCTileSet region = cacheWorkerCustomUrlProvider()->getTileCount(5, -122.0, 37.0, -121.0, 36.0);
    QCOMPARE(region.tileCount, 1U);
    auto* const tileSet = new QGCCachedTileSet(QStringLiteral("Custom URL Set"));
    tileSet->setMapTypeStr(kCacheWorkerCustomUrlType);
    tileSet->setType(kCacheWorkerCustomUrlType);
    tileSet->setTopleftLat(37.0);
    tileSet->setTopleftLon(-122.0);
    tileSet->setBottomRightLat(36.0);
    tileSet->setBottomRightLon(-121.0);
    tileSet->setMinZoom(5);
    tileSet->setMaxZoom(5);
    tileSet->setTotalTileCount(1);

    // The set is of the source configured when it is created. The worker computes the keys of its tiles later, and
    // cannot read the settings.
    std::unique_ptr<QGCCachedTileSet> savedSet;
    const QObject createContext;
    saved.setFactValue(customUrl, kCacheWorkerSourceA);
    QGCMapTask* const createTask = QGCMapTasks::createTileSet(
        tileSet, &createContext, [&savedSet](QGCCachedTileSet* set) { savedSet.reset(set); });
    saved.setFactValue(customUrl, kCacheWorkerSourceB);

    QCOMPARE(_runTask(cacheWorkerTasks(worker), createTask), QString());
    QVERIFY(savedSet);

    QStringList hashes;
    const QObject listContext;
    QGCMapTask* const listTask =
        QGCMapTasks::tileDownloadList(savedSet->id(), 10, &listContext, [&hashes](const QQueue<QGCTile*>& tiles) {
            for (const QGCTile* tile : tiles) {
                hashes.append(tile->hash);
            }
            qDeleteAll(tiles);
        });
    QCOMPARE(_runTask(cacheWorkerTasks(worker), listTask), QString());
    QCOMPARE(hashes, QStringList{cacheWorkerCustomUrlProvider()->getTileHash(region.tileX0, region.tileY0, 5,
                                                                             kCacheWorkerSourceA)});

    worker.stop();
    worker.wait(TestTimeout::mediumMs());
}

void QGCCacheWorkerTest::_testImportGivesOldTilesTheConfiguredSource()
{
    Fact* const customUrl = SettingsManager::instance()->appSettings()->customURL();
    TestFixtures::SettingsFixture saved;

    QTemporaryDir tempDir;
    const QString importPath = tempDir.filePath("version2.db");
    const QByteArray image("tile of the custom URL");
    QVERIFY(createVersion2CacheWithCustomUrlTile(importPath, 1, 2, 3, image));

    saved.setFactValue(customUrl, kCacheWorkerSourceA);
    QGCCacheWorker worker;
    worker.setDatabaseFile(tempDir.filePath("import_source.db"));
    QVERIFY(_startWorker(worker));

    // The source configured when the file is imported, not the one of the time the cache was opened
    saved.setFactValue(customUrl, kCacheWorkerSourceB);
    QCOMPARE(_runTask(cacheWorkerTasks(worker), QGCMapTasks::importSets(importPath, false)), QString());

    const std::optional<QGCCacheTile> tile =
        _fetchTile(cacheWorkerTasks(worker), cacheWorkerCustomUrlProvider()->getTileHash(1, 2, 3));
    QVERIFY(tile.has_value());
    QCOMPARE(tile->img, image);

    worker.stop();
    worker.wait(TestTimeout::mediumMs());
}

void QGCCacheWorkerTest::_testMapEngineGivesOldTilesTheConfiguredSource()
{
    Fact* const customUrl = SettingsManager::instance()->appSettings()->customURL();
    TestFixtures::SettingsFixture saved;

    QTemporaryDir tempDir;
    const QString databasePath = tempDir.filePath("version2.db");
    const QByteArray image("tile of the custom URL");
    QVERIFY(createVersion2CacheWithCustomUrlTile(databasePath, 1, 2, 3, image));

    // The engine takes the source from the settings for the worker thread, which migrates the cache
    saved.setFactValue(customUrl, kCacheWorkerSourceA);
    QGCMapEngine engine;
    engine.init(databasePath);

    const auto addTask = [&engine](QGCMapTask* task) { return engine.addTask(task); };
    const std::optional<QGCCacheTile> tile = _fetchTile(addTask, cacheWorkerCustomUrlProvider()->getTileHash(1, 2, 3));
    QVERIFY(tile.has_value());
    QCOMPARE(tile->img, image);

    engine.close();
}

void QGCCacheWorkerTest::_testMapEngineReportsSourceChange_data()
{
    QTest::addColumn<QString>("settingName");
    QTest::addColumn<QString>("providerType");
    QTest::newRow("Custom URL") << QStringLiteral("customURL") << kCacheWorkerCustomUrlType;
    QTest::newRow("Mapbox Custom account") << QStringLiteral("mapboxAccount") << QStringLiteral("Mapbox Custom");
    QTest::newRow("Mapbox Custom style") << QStringLiteral("mapboxStyle") << QStringLiteral("Mapbox Custom");
}

void QGCCacheWorkerTest::_testMapEngineReportsSourceChange()
{
    QFETCH(QString, settingName);
    QFETCH(QString, providerType);

    Fact* const setting = SettingsManager::instance()->appSettings()->property(qPrintable(settingName)).value<Fact*>();
    QVERIFY(setting);
    TestFixtures::SettingsFixture saved;
    saved.setFactValue(setting, QStringLiteral("source A"));

    // A map that is open holds tiles of the source that was configured, so it has to hear of another one
    QGCMapEngine engine;
    QSignalSpy sourceSpy(&engine, &QGCMapEngine::tileSourceChanged);
    saved.setFactValue(setting, QStringLiteral("source B"));

    QCOMPARE(sourceSpy.count(), 1);
    QCOMPARE(sourceSpy.first().first().toInt(), UrlFactory::getMapIdFromProviderType(providerType));
}

void QGCCacheWorkerTest::_testFreePagesReleasedWhenIdle_data()
{
    QTest::addColumn<QString>("task");
    QTest::newRow("tile set deleted") << QStringLiteral("delete set");
    QTest::newRow("cache pruned") << QStringLiteral("prune");
    QTest::newRow("file imported") << QStringLiteral("import");
}

void QGCCacheWorkerTest::_testFreePagesReleasedWhenIdle()
{
    QFETCH(QString, task);

    // A cache with unused pages, as an earlier run leaves it that quit before it had released them
    constexpr int tileCount = 20;
    QTemporaryDir tempDir;
    const QString path = tempDir.filePath("tiles.db");
    quint64 deletedSetID = 0;
    quint64 setID = 0;
    QVERIFY(addLargeTileSet(path, QStringLiteral("set"), tileCount, &setID));
    QVERIFY(addLargeTileSet(path, QStringLiteral("default"), tileCount, nullptr));
    QVERIFY(addLargeTileSet(path, QStringLiteral("deleted"), tileCount, &deletedSetID, true));
    QVERIFY(tileCacheFreePages(path) > 0);

    // The unused pages at the time the task reports that it is done, read on the worker thread: the task does not
    // wait for their release. Declared before the worker, which uses them until it is destroyed.
    std::atomic<qint64> pagesWhenDone = -1;
    const auto readPages = [&pagesWhenDone, &path] { pagesWhenDone = tileCacheFreePages(path); };

    // The worker releases the pages once it has started, without a task to make it. From here on it has nothing
    // left to release until a task frees pages.
    QGCCacheWorker worker;
    worker.setDatabaseFile(path);
    QVERIFY(_startWorker(worker));
    QTRY_COMPARE_WITH_TIMEOUT(tileCacheFreePages(path), 0, TestTimeout::mediumMs());

    QGCMapTask* mapTask = nullptr;
    if (task == QStringLiteral("delete set")) {
        mapTask = QGCMapTasks::deleteTileSet(setID);
    } else if (task == QStringLiteral("prune")) {
        mapTask = QGCMapTasks::pruneCache(static_cast<quint64>(tileCount) * kCacheWorkerLargeTileSize);
    } else {
        // The file that replaces the cache has unused pages of its own
        const QString importPath = tempDir.filePath("import.db");
        quint64 importSetID = 0;
        QVERIFY(addLargeTileSet(importPath, QStringLiteral("deleted"), tileCount, &importSetID, true));
        mapTask = QGCMapTasks::importSets(importPath, true);
    }
    connect(mapTask, &QGCMapTask::finished, this, readPages, Qt::DirectConnection);
    QVERIFY(worker.enqueueTask(mapTask));
    QTRY_VERIFY_WITH_TIMEOUT(pagesWhenDone >= 0, TestTimeout::mediumMs());
    QVERIFY(pagesWhenDone > 0);

    QTRY_COMPARE_WITH_TIMEOUT(tileCacheFreePages(path), 0, TestTimeout::mediumMs());

    worker.stop();
    worker.wait(TestTimeout::mediumMs());
}

void QGCCacheWorkerTest::_testTileFetchedBeforeFreePagesReleased()
{
    // A set that takes several batches to release, and a tile that stays in the cache
    QTemporaryDir tempDir;
    const QString path = tempDir.filePath("tiles.db");
    quint64 setID = 0;
    QVERIFY(addLargeTileSet(path, QStringLiteral("set"), 64, &setID));
    QVERIFY(addLargeTileSet(path, QStringLiteral("default"), 1, nullptr));

    // Written on the worker thread, so declared before the worker, which uses them until it is destroyed
    std::atomic<qint64> pagesWhenFetched = -1;
    QByteArray image;

    QGCCacheWorker worker;
    worker.setDatabaseFile(path);
    QVERIFY(_startWorker(worker));

    // The tile is asked for as soon as the set is deleted, on the worker thread, so that the request is queued
    // before the worker looks for something to do. The unused pages are read there when the tile arrives.
    QGCMapTask* const fetchTask = new QGCMapTask(QStringLiteral("Fetch Tile"), [&](QGCCacheWorker& cache, QGCMapTask&) {
        if (const std::unique_ptr<QGCCacheTile> tile = cache.tile(QStringLiteral("default_0"))) {
            image = tile->img;
        }
        pagesWhenFetched = tileCacheFreePages(path);
        return QString();
    });
    QGCMapTask* const deleteTask = QGCMapTasks::deleteTileSet(setID);
    connect(
        deleteTask, &QGCMapTask::finished, this, [&worker, fetchTask] { (void) worker.enqueueTask(fetchTask); },
        Qt::DirectConnection);
    QVERIFY(worker.enqueueTask(deleteTask));

    // The tile does not wait for the pages of the set, which are released afterwards
    QTRY_VERIFY_WITH_TIMEOUT(pagesWhenFetched >= 0, TestTimeout::mediumMs());
    QCOMPARE(image, QByteArray(kCacheWorkerLargeTileSize, 'T'));
    QVERIFY(pagesWhenFetched > 0);
    QTRY_COMPARE_WITH_TIMEOUT(tileCacheFreePages(path), 0, TestTimeout::mediumMs());

    worker.stop();
    worker.wait(TestTimeout::mediumMs());
}

UT_REGISTER_TEST(QGCCacheWorkerTest, TestLabel::Unit)
