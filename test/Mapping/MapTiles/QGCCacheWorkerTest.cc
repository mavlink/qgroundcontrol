#include "QGCCacheWorkerTest.h"

#include <atomic>
#include <cmath>
#include <memory>
#include <optional>

#include <QtCore/QScopeGuard>
#include <QtCore/QTemporaryDir>
#include <QtPositioning/QGeoCoordinate>
#include <QtSql/QSqlQuery>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "AppSettings.h"
#include "BaseClasses/TerrainTest.h"
#include "ElevationMapProvider.h"
#include "QGCCacheTile.h"
#include "QGCCachedTileSet.h"
#include "QGCMapEngine.h"
#include "QGCMapTasks.h"
#include "QGCMapUrlEngine.h"
#include "QGCTileCacheDatabase.h"
#include "QGCTileCacheWorker.h"
#include "QGCTileSet.h"
#include "SettingsManager.h"
#include "TerrainTile.h"
#include "TerrainTileCopernicus.h"
#include "TileCacheTestHelpers.h"

static const QString kTestProviderType = QStringLiteral("Bing Road");

namespace {

/// The provider that fetches from the URL template the user sets
const QString kCacheWorkerCustomUrlType = QStringLiteral("CustomURL Custom");
const QString kCacheWorkerSourceA = QStringLiteral("https://a.example/{z}/{x}/{y}.png");
const QString kCacheWorkerSourceB = QStringLiteral("https://b.example/{z}/{x}/{y}.png");

/// A cache of schema version 2, where the key of a tile did not tell its source, with a tile of the custom URL
/// provider. Version 2 has the tables of today.
bool createVersion2CacheWithCustomUrlTile(const QString& path, int x, int y, int z, const QByteArray& image)
{
    QGCTileCacheDatabase db(path);
    if (!db.init() || !db.connectDB() ||
        !db.saveTile(UrlFactory::getTileHash(kCacheWorkerCustomUrlType, x, y, z, QString()), QStringLiteral("png"),
                     image, kCacheWorkerCustomUrlType, QGCTileCacheDatabase::kInvalidTileSet)) {
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

QByteArray QGCCacheWorkerTest::_fetchTileImage(const std::function<bool(QGCMapTask*)>& addTask, const QString& hash)
{
    const auto image = std::make_shared<std::optional<QByteArray>>();
    auto* const task = new QGCFetchTileTask(hash);
    connect(
        task, &QGCFetchTileTask::tileFetched, this, [image](const QGCCacheTile& tile) { *image = tile.img; },
        Qt::QueuedConnection);
    connect(task, &QGCMapTask::error, this, [image] { *image = QByteArray(); }, Qt::QueuedConnection);
    if (!addTask(task)) {
        task->deleteLater();
        return QByteArray();
    }

    (void) UnitTest::waitForCondition([image] { return image->has_value(); }, TestTimeout::mediumMs(),
                                      QStringLiteral("answer of the tile cache"));
    return image->value_or(QByteArray());
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
    QTemporaryDir tempDir;
    QGCCacheWorker worker;
    worker.setDatabaseFile(tempDir.filePath("save_fetch.db"));
    QVERIFY(_startWorker(worker));

    auto* tile =
        new QGCCacheTile(QStringLiteral("h1"), QByteArray("tile_data"), QStringLiteral("png"), kTestProviderType);
    QVERIFY(worker.enqueueTask(new QGCSaveTileTask(tile)));

    // Fetch — FIFO guarantees save completes first
    auto* fetchTask = new QGCFetchTileTask(QStringLiteral("h1"));
    std::optional<QGCCacheTile> fetched;
    bool fetchError = false;
    // Declared after what the lambdas write: a signal still queued when this function returns is dropped with it
    const QObject fetchContext;
    connect(
        fetchTask, &QGCFetchTileTask::tileFetched, &fetchContext, [&](const QGCCacheTile& t) { fetched = t; },
        Qt::QueuedConnection);
    connect(
        fetchTask, &QGCMapTask::error, &fetchContext, [&](QGCMapTask::TaskType, const QString&) { fetchError = true; },
        Qt::QueuedConnection);

    QVERIFY(worker.enqueueTask(fetchTask));
    QTRY_VERIFY_WITH_TIMEOUT(fetched || fetchError, TestTimeout::mediumMs());

    QVERIFY2(fetched.has_value(), "Expected tile to be fetched");
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
    std::optional<QGCCacheTile> fetched;
    bool fetchError = false;
    const QObject fetchContext;
    connect(
        fetchTask, &QGCFetchTileTask::tileFetched, &fetchContext, [&](const QGCCacheTile& t) { fetched = t; },
        Qt::QueuedConnection);
    connect(
        fetchTask, &QGCMapTask::error, &fetchContext, [&](QGCMapTask::TaskType, const QString&) { fetchError = true; },
        Qt::QueuedConnection);

    QVERIFY(worker.enqueueTask(fetchTask));
    QTRY_VERIFY_WITH_TIMEOUT(fetched || fetchError, TestTimeout::mediumMs());

    QVERIFY(!fetched.has_value());
    QVERIFY(fetchError);

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
    const QString hash = UrlFactory::getTileHash(kTestProviderType, 1, 2, 3);
    auto* fetchTask = new QGCFetchTileTask(hash);
    std::optional<QGCCacheTile> fetched;
    bool fetchError = false;
    const QObject fetchContext;
    connect(
        fetchTask, &QGCFetchTileTask::tileFetched, &fetchContext, [&](const QGCCacheTile& t) { fetched = t; },
        Qt::QueuedConnection);
    connect(
        fetchTask, &QGCMapTask::error, &fetchContext, [&](QGCMapTask::TaskType, const QString&) { fetchError = true; },
        Qt::QueuedConnection);

    QVERIFY(worker.enqueueTask(fetchTask));
    QTRY_VERIFY_WITH_TIMEOUT(fetched || fetchError, TestTimeout::mediumMs());

    QVERIFY2(fetched.has_value(), "Expected fake tile on map tile cache miss under unit tests");
    QVERIFY(!fetchError);
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
    const QGeoCoordinate flatCenter = UnitTestTerrainData::flat10Region.center();
    const int x =
        static_cast<int>(std::floor((flatCenter.longitude() + 180.0) / TerrainTileCopernicus::kTileSizeDegrees));
    const int y =
        static_cast<int>(std::floor((flatCenter.latitude() + 90.0) / TerrainTileCopernicus::kTileSizeDegrees));
    const QString hash = UrlFactory::getTileHash(elevationType, x, y, 1);

    auto* fetchTask = new QGCFetchTileTask(hash);
    std::optional<QGCCacheTile> fetched;
    bool fetchError = false;
    const QObject fetchContext;
    connect(
        fetchTask, &QGCFetchTileTask::tileFetched, &fetchContext, [&](const QGCCacheTile& t) { fetched = t; },
        Qt::QueuedConnection);
    connect(
        fetchTask, &QGCMapTask::error, &fetchContext, [&](QGCMapTask::TaskType, const QString&) { fetchError = true; },
        Qt::QueuedConnection);

    QVERIFY(worker.enqueueTask(fetchTask));
    QTRY_VERIFY_WITH_TIMEOUT(fetched || fetchError, TestTimeout::mediumMs());

    QVERIFY2(fetched.has_value(), "Expected synthetic terrain tile on elevation cache miss under unit tests");
    QVERIFY(!fetchError);
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
    auto* farTask = new QGCFetchTileTask(UrlFactory::getTileHash(elevationType, farX, farY, 1));
    std::optional<QGCCacheTile> farFetched;
    bool farError = false;
    const QObject farContext;
    connect(
        farTask, &QGCFetchTileTask::tileFetched, &farContext, [&](const QGCCacheTile& t) { farFetched = t; },
        Qt::QueuedConnection);
    connect(
        farTask, &QGCMapTask::error, &farContext, [&](QGCMapTask::TaskType, const QString&) { farError = true; },
        Qt::QueuedConnection);
    QVERIFY(worker.enqueueTask(farTask));
    QTRY_VERIFY_WITH_TIMEOUT(farFetched || farError, TestTimeout::mediumMs());
    QVERIFY2(farFetched.has_value(), "Expected synthetic terrain tile for coordinate outside test regions");
    QVERIFY(!farError);

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

    // Wait for updateTotals after the fetch task completes to know all sets were emitted
    bool taskDone = false;
    const QObject totalsContext;
    auto totalsConn = connect(
        &worker, &QGCCacheWorker::updateTotals, &totalsContext, [&]() { taskDone = true; }, Qt::QueuedConnection);

    QList<QGCCachedTileSet*> sets;
    auto* fetchTask = new QGCFetchTileSetTask();
    const QObject fetchContext;
    connect(
        fetchTask, &QGCFetchTileSetTask::tileSetFetched, &fetchContext,
        [&](QGCCachedTileSet* set) { sets.append(set); }, Qt::QueuedConnection);

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
    const QObject createContext;
    connect(
        createTask, &QGCCreateTileSetTask::tileSetSaved, &createContext, [&](QGCCachedTileSet* set) { savedSet = set; },
        Qt::QueuedConnection);
    connect(
        createTask, &QGCMapTask::error, &createContext,
        [&](QGCMapTask::TaskType, const QString&) { createError = true; }, Qt::QueuedConnection);

    QVERIFY(worker.enqueueTask(createTask));
    QTRY_VERIFY_WITH_TIMEOUT(savedSet || createError, TestTimeout::mediumMs());
    QVERIFY2(savedSet != nullptr, "Expected tile set to be created");
    QVERIFY(!createError);
    QVERIFY(savedSet->id() != 0);

    // Delete it
    const quint64 setID = savedSet->id();
    auto* deleteTask = new QGCDeleteTileSetTask(setID);
    quint64 deletedID = 0;
    const QObject deleteContext;
    connect(
        deleteTask, &QGCDeleteTileSetTask::tileSetDeleted, &deleteContext, [&](quint64 id) { deletedID = id; },
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
    QTemporaryDir tempDir;
    QGCCacheWorker worker;
    worker.setDatabaseFile(tempDir.filePath("prune.db"));
    QVERIFY(_startWorker(worker));

    for (int i = 0; i < 10; i++) {
        auto* tile = new QGCCacheTile(QStringLiteral("pr_%1").arg(i), QByteArray(100, 'P'), QStringLiteral("png"),
                                      kTestProviderType);
        QVERIFY(worker.enqueueTask(new QGCSaveTileTask(tile)));
    }

    auto* pruneTask = new QGCPruneCacheTask(500);
    bool pruneDone = false;
    const QObject pruneContext;
    connect(pruneTask, &QGCPruneCacheTask::pruned, &pruneContext, [&]() { pruneDone = true; }, Qt::QueuedConnection);
    QVERIFY(worker.enqueueTask(pruneTask));
    QTRY_VERIFY_WITH_TIMEOUT(pruneDone, TestTimeout::mediumMs());

    // Trigger totals to verify count decreased
    quint32 remaining = UINT32_MAX;
    const QObject totalsContext;
    auto conn = connect(
        &worker, &QGCCacheWorker::updateTotals, &totalsContext,
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

    auto* resetTask = new QGCResetTask();
    bool resetDone = false;
    const QObject resetContext;
    connect(resetTask, &QGCResetTask::resetCompleted, &resetContext, [&]() { resetDone = true; }, Qt::QueuedConnection);
    QVERIFY(worker.enqueueTask(resetTask));
    QTRY_VERIFY_WITH_TIMEOUT(resetDone, TestTimeout::mediumMs());

    // Verify saved tile is gone
    auto* fetchTask = new QGCFetchTileTask(QStringLiteral("r1"));
    bool fetchError = false;
    const QObject fetchContext;
    connect(
        fetchTask, &QGCMapTask::error, &fetchContext, [&](QGCMapTask::TaskType, const QString&) { fetchError = true; },
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

void QGCCacheWorkerTest::_testMapEngineCloseStopsWorkerAndRefusesInit()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString databasePath = tempDir.filePath(QStringLiteral("engine.db"));

    QGCMapEngine engine;
    engine.init(databasePath);
    auto* beforeClose = new QGCFetchTileTask(QStringLiteral("hash"));
    QVERIFY(engine.addTask(beforeClose));

    engine.close();
    auto* afterClose = new QGCFetchTileTask(QStringLiteral("hash"));
    QVERIFY(!engine.addTask(afterClose));
    afterClose->deleteLater();

    // A map created during application teardown must not restart the worker.
    engine.init(databasePath);
    auto* afterReinit = new QGCFetchTileTask(QStringLiteral("hash"));
    QVERIFY(!engine.addTask(afterReinit));
    afterReinit->deleteLater();
}

// GeoMap needs the tile cache without ever creating a QtLocation map, which is what normally initializes it
void QGCCacheWorkerTest::_testMapEngineEnsureInitializedAcceptsTasks()
{
    QGCMapEngine engine;
    auto* beforeInit = new QGCFetchTileTask(QStringLiteral("hash"));
    QVERIFY(!engine.addTask(beforeInit));
    beforeInit->deleteLater();

    engine.ensureInitialized();
    auto* afterInit = new QGCFetchTileTask(QStringLiteral("hash"));
    QVERIFY(engine.addTask(afterInit));

    // Calling it again keeps the running worker
    engine.ensureInitialized();
    auto* afterSecondCall = new QGCFetchTileTask(QStringLiteral("hash"));
    QVERIFY(engine.addTask(afterSecondCall));

    engine.close();
}

void QGCCacheWorkerTest::_testCreateTileSetKeepsSourceOfItsCreation()
{
    Fact* const customUrl = SettingsManager::instance()->appSettings()->customURL();
    const QVariant savedUrl = customUrl->rawValue();
    const auto restoreUrl = qScopeGuard([customUrl, savedUrl] { customUrl->setRawValue(savedUrl); });

    QTemporaryDir tempDir;
    QGCCacheWorker worker;
    worker.setDatabaseFile(tempDir.filePath("set_source.db"));
    QVERIFY(_startWorker(worker));

    // A region of one tile
    const QGCTileSet region = UrlFactory::getTileCount(5, -122.0, 37.0, -121.0, 36.0, kCacheWorkerCustomUrlType);
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
    customUrl->setRawValue(kCacheWorkerSourceA);
    auto* const createTask = new QGCCreateTileSetTask(tileSet);
    customUrl->setRawValue(kCacheWorkerSourceB);

    QGCCachedTileSet* savedSet = nullptr;
    const QObject createContext;
    connect(
        createTask, &QGCCreateTileSetTask::tileSetSaved, &createContext, [&](QGCCachedTileSet* set) { savedSet = set; },
        Qt::QueuedConnection);
    QVERIFY(worker.enqueueTask(createTask));
    QTRY_VERIFY_WITH_TIMEOUT(savedSet, TestTimeout::mediumMs());
    const std::unique_ptr<QGCCachedTileSet> savedSetOwner(savedSet);

    auto* const listTask = new QGCGetTileDownloadListTask(savedSet->id(), 10);
    std::optional<QStringList> hashes;
    const QObject listContext;
    connect(
        listTask, &QGCGetTileDownloadListTask::tileListFetched, &listContext,
        [&](const QQueue<QGCTile*>& tiles) {
            hashes = QStringList();
            for (const QGCTile* tile : tiles) {
                hashes->append(tile->hash);
            }
            qDeleteAll(tiles);
        },
        Qt::QueuedConnection);
    QVERIFY(worker.enqueueTask(listTask));
    QTRY_VERIFY_WITH_TIMEOUT(hashes.has_value(), TestTimeout::mediumMs());
    QCOMPARE(*hashes, QStringList{UrlFactory::getTileHash(kCacheWorkerCustomUrlType, region.tileX0, region.tileY0, 5,
                                                          kCacheWorkerSourceA)});

    worker.stop();
    worker.wait(TestTimeout::mediumMs());
}

void QGCCacheWorkerTest::_testImportGivesOldTilesTheConfiguredSource()
{
    Fact* const customUrl = SettingsManager::instance()->appSettings()->customURL();
    const QVariant savedUrl = customUrl->rawValue();
    const auto restoreUrl = qScopeGuard([customUrl, savedUrl] { customUrl->setRawValue(savedUrl); });

    QTemporaryDir tempDir;
    const QString importPath = tempDir.filePath("version2.db");
    const QByteArray image("tile of the custom URL");
    QVERIFY(createVersion2CacheWithCustomUrlTile(importPath, 1, 2, 3, image));

    customUrl->setRawValue(kCacheWorkerSourceA);
    QGCCacheWorker worker;
    worker.setDatabaseFile(tempDir.filePath("import_source.db"));
    QVERIFY(_startWorker(worker));

    // The source configured when the file is imported, not the one of the time the cache was opened
    customUrl->setRawValue(kCacheWorkerSourceB);
    auto* const importTask = new QGCImportTileTask(importPath, false);
    bool imported = false;
    const QObject importContext;
    connect(
        importTask, &QGCImportTileTask::actionCompleted, &importContext, [&imported] { imported = true; },
        Qt::QueuedConnection);
    QVERIFY(worker.enqueueTask(importTask));
    QTRY_VERIFY_WITH_TIMEOUT(imported, TestTimeout::mediumMs());

    const auto addTask = [&worker](QGCMapTask* task) { return worker.enqueueTask(task); };
    QCOMPARE(_fetchTileImage(addTask, UrlFactory::getTileHash(kCacheWorkerCustomUrlType, 1, 2, 3)), image);

    worker.stop();
    worker.wait(TestTimeout::mediumMs());
}

void QGCCacheWorkerTest::_testMapEngineGivesOldTilesTheConfiguredSource()
{
    Fact* const customUrl = SettingsManager::instance()->appSettings()->customURL();
    const QVariant savedUrl = customUrl->rawValue();
    const auto restoreUrl = qScopeGuard([customUrl, savedUrl] { customUrl->setRawValue(savedUrl); });

    QTemporaryDir tempDir;
    const QString databasePath = tempDir.filePath("version2.db");
    const QByteArray image("tile of the custom URL");
    QVERIFY(createVersion2CacheWithCustomUrlTile(databasePath, 1, 2, 3, image));

    // The engine takes the source from the settings for the worker thread, which migrates the cache
    customUrl->setRawValue(kCacheWorkerSourceA);
    QGCMapEngine engine;
    engine.init(databasePath);

    const auto addTask = [&engine](QGCMapTask* task) { return engine.addTask(task); };
    QCOMPARE(_fetchTileImage(addTask, UrlFactory::getTileHash(kCacheWorkerCustomUrlType, 1, 2, 3)), image);

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
    const QVariant saved = setting->rawValue();
    const auto restore = qScopeGuard([setting, saved] { setting->setRawValue(saved); });
    setting->setRawValue(QStringLiteral("source A"));

    // A map that is open holds tiles of the source that was configured, so it has to hear of another one
    QGCMapEngine engine;
    QSignalSpy sourceSpy(&engine, &QGCMapEngine::tileSourceChanged);
    setting->setRawValue(QStringLiteral("source B"));

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
        auto* const deleteTask = new QGCDeleteTileSetTask(setID);
        connect(deleteTask, &QGCDeleteTileSetTask::tileSetDeleted, this, readPages, Qt::DirectConnection);
        mapTask = deleteTask;
    } else if (task == QStringLiteral("prune")) {
        auto* const pruneTask = new QGCPruneCacheTask(static_cast<quint64>(tileCount) * kCacheWorkerLargeTileSize);
        connect(pruneTask, &QGCPruneCacheTask::pruned, this, readPages, Qt::DirectConnection);
        mapTask = pruneTask;
    } else {
        // The file that replaces the cache has unused pages of its own
        const QString importPath = tempDir.filePath("import.db");
        quint64 importSetID = 0;
        QVERIFY(addLargeTileSet(importPath, QStringLiteral("deleted"), tileCount, &importSetID, true));
        auto* const importTask = new QGCImportTileTask(importPath, true);
        connect(importTask, &QGCImportTileTask::actionCompleted, this, readPages, Qt::DirectConnection);
        mapTask = importTask;
    }
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
    auto* const fetchTask = new QGCFetchTileTask(QStringLiteral("default_0"));
    connect(
        fetchTask, &QGCFetchTileTask::tileFetched, this,
        [&](const QGCCacheTile& tile) {
            image = tile.img;
            pagesWhenFetched = tileCacheFreePages(path);
        },
        Qt::DirectConnection);
    auto* const deleteTask = new QGCDeleteTileSetTask(setID);
    connect(
        deleteTask, &QGCDeleteTileSetTask::tileSetDeleted, this,
        [&worker, fetchTask] { (void) worker.enqueueTask(fetchTask); }, Qt::DirectConnection);
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
