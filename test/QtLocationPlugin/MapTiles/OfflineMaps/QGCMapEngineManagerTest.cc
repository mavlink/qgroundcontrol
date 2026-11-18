#include "QGCMapEngineManagerTest.h"

#include <utility>

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QRegularExpression>
#include <QtCore/QTemporaryDir>
#include <QtTest/QSignalSpy>

#include "ElevationMapProvider.h"
#include "Fact.h"
#include "Fixtures/RAIIFixtures.h"
#include "FlightMapSettings.h"
#include "FlyViewSettings.h"
#include "HeightField.h"
#include "MapProvider.h"
#include "PlanViewSettings.h"
#include "QGCCachedTileSet.h"
#include "QGCMapCatalog.h"
#include "QGCMapEngine.h"
#include "QGCMapEngineManager.h"
#include "QmlObjectListModel.h"
#include "SettingsManager.h"
#include "TerrariumTileFetcher.h"
#include "TileMath.h"
#include "TileReplyNetworkMock.h"
#include "UnitTestTileGenerator.h"
#include "UrlFactory.h"

namespace {

// A region within one tile of the elevation provider and of every map zoom up to 17, whatever its longitude below.
// Its Terrarium tiles are one of every zoom, and at the anchor zoom the three anchors next to that one as well.
constexpr double kOfflineRegionTopLat = 47.3906;
constexpr double kOfflineRegionBottomLat = 47.3902;
constexpr double kOfflineRegionLonSpan = 0.0004;
constexpr quint64 kOfflineRegionNeighborAnchorCount = 3;

// A set asks for this many tiles at a time. It asks for no more while the network stand-in below holds its replies.
constexpr quint64 kTilesRequestedAtOnce = 6;

QGCTileSet offlineRegionTiles(const QString& type, double leftLon, int minZoom, int maxZoom)
{
    const SharedMapProvider provider = UrlFactory::getMapProviderFromProviderType(type);
    QGCTileSet tiles;
    for (int zoom = minZoom; zoom <= maxZoom; zoom++) {
        tiles += provider->getTileCount(zoom, leftLon, kOfflineRegionTopLat, leftLon + kOfflineRegionLonSpan,
                                        kOfflineRegionBottomLat);
    }
    return tiles;
}

/// Selects the map engine of the Fly View and of the Plan View until \a saved is gone
void selectGeoMapEngines(TestFixtures::SettingsFixture& saved, bool flyView, bool planView)
{
    saved.setFactValue(SettingsManager::instance()->flyViewSettings()->useGeoMapEngine(), flyView);
    saved.setFactValue(SettingsManager::instance()->planViewSettings()->useGeoMapEngine(), planView);
}

/// A manager whose offline sets send their requests to a network stand-in that holds its replies, so that a test
/// decides which of them are answered
class OfflineSetsFixture
{
public:
    OfflineSetsFixture()
    {
        networkManager.holdReplies = true;
        (void) QObject::connect(&manager, &QGCMapEngineManager::tileSetsChanged, &manager, [this] {
            for (QObject* const set : std::as_const(*manager.tileSets()->objectList())) {
                qobject_cast<QGCCachedTileSet*>(set)->setNetworkManager(&networkManager);
            }
        });
    }

    ~OfflineSetsFixture()
    {
        // The manager does not own its sets, and they must not outlive the network manager they download with
        while (manager.tileSets()->count() > 0) {
            delete manager.tileSets()->removeAt(0);
        }
    }

    QGCCachedTileSet* tileSet(const QString& name)
    {
        for (QObject* const object : std::as_const(*manager.tileSets()->objectList())) {
            QGCCachedTileSet* const set = qobject_cast<QGCCachedTileSet*>(object);
            if (set->name() == name) {
                return set;
            }
        }
        return nullptr;
    }

    TileReplyMockNam networkManager;
    QGCMapEngineManager manager;
};

}  // namespace

void QGCMapEngineManagerTest::initTestCase()
{
    UnitTest::initTestCase();
    // The tests fetch tiles that the tile cache of the process must not have yet
    QVERIFY(UnitTestTileGenerator::resetTileCache());
}

void QGCMapEngineManagerTest::_testTerrainSet_data()
{
    QTest::addColumn<bool>("flyViewGeoMap");
    QTest::addColumn<bool>("planViewGeoMap");
    QTest::addColumn<bool>("fetchElevation");
    QTest::addColumn<int>("minZoom");
    QTest::addColumn<int>("maxZoom");
    QTest::addColumn<int>("terrainMinZoom");
    QTest::addColumn<int>("terrainMaxZoom");  // 0 without a terrain set

    constexpr int kAnchorZoom = TerrariumElevationProvider::kAnchorZoom;
    constexpr int kTerrariumMaxZoom = TerrariumElevationProvider::kMaxZoom;

    // The terrain set has the zooms of the imagery that Terrarium serves, and the anchor zoom below the finer ones
    QTest::newRow("zooms up to the anchor zoom") << true << false << true << 7 << kAnchorZoom << 7 << kAnchorZoom;
    QTest::newRow("zooms across the anchor zoom") << true << false << true << 9 << 12 << 9 << 12;
    QTest::newRow("zooms finer than the anchor zoom") << true << false << true << 12 << 14 << kAnchorZoom << 14;
    QTest::newRow("zooms finer than Terrarium serves")
        << true << false << true << 16 << 17 << kAnchorZoom << kTerrariumMaxZoom;

    QTest::newRow("GeoMap in the Plan View only") << false << true << true << 12 << 14 << kAnchorZoom << 14;
    QTest::newRow("no GeoMap") << false << false << true << 12 << 14 << 0 << 0;
    QTest::newRow("elevation not fetched") << true << true << false << 12 << 14 << 0 << 0;
}

void QGCMapEngineManagerTest::_testTerrainSet()
{
    QFETCH(bool, flyViewGeoMap);
    QFETCH(bool, planViewGeoMap);
    QFETCH(bool, fetchElevation);
    QFETCH(int, minZoom);
    QFETCH(int, maxZoom);
    QFETCH(int, terrainMinZoom);
    QFETCH(int, terrainMaxZoom);
    const bool terrainExpected = terrainMaxZoom > 0;

    // The setting requires a restart, which the app announces on every change
    ignoreLogMessage("API.QGCApplication.AppMessage", QtDebugMsg,
                     QRegularExpression(QStringLiteral("Restart application for changes to take effect")));
    TestFixtures::SettingsFixture saved;
    selectGeoMapEngines(saved, flyViewGeoMap, planViewGeoMap);

    QGCMapEngine* const engine = QGCMapEngine::instance();
    engine->ensureInitialized();

    constexpr double kLeftLon = 8.4991;
    const QString mapType = QGCMapCatalog::mapList().constFirst();
    const QString elevationType =
        SettingsManager::instance()->flightMapSettings()->elevationMapProvider()->rawValue().toString();
    const QString terrainType = QString::fromLatin1(TerrariumElevationProvider::kProviderKey);
    const QGCTileSet imageTiles = offlineRegionTiles(mapType, kLeftLon, minZoom, maxZoom);
    const QGCTileSet elevationTiles = fetchElevation ? offlineRegionTiles(elevationType, kLeftLon, 1, 1) : QGCTileSet();
    const QGCTileSet terrainTiles =
        terrainExpected ? offlineRegionTiles(terrainType, kLeftLon, terrainMinZoom, terrainMaxZoom) : QGCTileSet();
    QCOMPARE(terrainTiles.tileCount,
             terrainExpected ? quint64(terrainMaxZoom - terrainMinZoom + 1) + kOfflineRegionNeighborAnchorCount
                             : quint64(0));

    OfflineSetsFixture sets;
    QVERIFY(sets.manager.setProperty("fetchElevation", fetchElevation));
    sets.manager.updateForCurrentView(kLeftLon, kOfflineRegionTopLat, kLeftLon + kOfflineRegionLonSpan,
                                      kOfflineRegionBottomLat, minZoom, maxZoom, mapType);
    QCOMPARE(sets.manager.tileCount(), imageTiles.tileCount + elevationTiles.tileCount + terrainTiles.tileCount);
    QCOMPARE(sets.manager.tileSize(), imageTiles.tileSize + elevationTiles.tileSize + terrainTiles.tileSize);

    const QString name = QStringLiteral("terrain set test: %1").arg(QString::fromLatin1(QTest::currentDataTag()));
    sets.manager.startDownload(name, mapType);
    // The sets are all saved
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());

    QVERIFY(sets.tileSet(name));
    QCOMPARE(sets.tileSet(name + QStringLiteral(" Elevation")) != nullptr, fetchElevation);
    const QGCCachedTileSet* const terrainSet = sets.tileSet(name + QStringLiteral(" Terrain"));
    QCOMPARE(terrainSet != nullptr, terrainExpected);
    if (terrainSet) {
        QCOMPARE(terrainSet->type(), terrainType);
        QCOMPARE(terrainSet->minZoom(), terrainMinZoom);
        QCOMPARE(terrainSet->maxZoom(), terrainMaxZoom);
        QCOMPARE(terrainSet->totalTileCount(), static_cast<quint32>(terrainTiles.tileCount));
    }

    const auto requestedAtOnce = [](const QGCTileSet& tiles) { return qMin(tiles.tileCount, kTilesRequestedAtOnce); };
    const int expectedRequestCount =
        static_cast<int>(requestedAtOnce(imageTiles) + requestedAtOnce(elevationTiles) + requestedAtOnce(terrainTiles));
    QTRY_COMPARE_WITH_TIMEOUT(sets.networkManager.requestCount, expectedRequestCount, TestTimeout::mediumMs());
    // No more than these: a set asks for further tiles when a reply ends, which none does here, or when a download
    // list arrives. The lists have all arrived once the worker has run its tasks, and a set has asked for the tiles of
    // a list once the events that the list queued are handled.
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());
    QCoreApplication::processEvents();
    QCOMPARE(sets.networkManager.requestCount, expectedRequestCount);
}

void QGCMapEngineManagerTest::_testElevationTypeSet_data()
{
    QTest::addColumn<QString>("elevationType");
    QTest::addColumn<int>("minZoom");
    QTest::addColumn<int>("maxZoom");
    QTest::addColumn<int>("setMinZoom");
    QTest::addColumn<int>("setMaxZoom");

    // The tiles have no zoom levels: the set has each once, with the zoom that the terrain queries ask the cache for
    QTest::newRow("tiles without zoom levels")
        << QString::fromLatin1(CopernicusElevationProvider::kProviderKey) << 12 << 14 << 1 << 1;
    // The set that an imagery set of these zooms gets for GeoMap: Terrarium serves no finer zoom than its last
    QTest::newRow("tiles of the zooms that the provider serves")
        << QString::fromLatin1(TerrariumElevationProvider::kProviderKey) << 16 << 17
        << TerrariumElevationProvider::kAnchorZoom << TerrariumElevationProvider::kMaxZoom;
}

// "Add New Set" offers the elevation providers as types of a set
void QGCMapEngineManagerTest::_testElevationTypeSet()
{
    QFETCH(QString, elevationType);
    QFETCH(int, minZoom);
    QFETCH(int, maxZoom);
    QFETCH(int, setMinZoom);
    QFETCH(int, setMaxZoom);

    // A set of imagery gets both sets of elevation tiles beside it: GeoMap is used, and elevation is fetched
    ignoreLogMessage("API.QGCApplication.AppMessage", QtDebugMsg,
                     QRegularExpression(QStringLiteral("Restart application for changes to take effect")));
    TestFixtures::SettingsFixture saved;
    selectGeoMapEngines(saved, true, false);

    QGCMapEngine::instance()->ensureInitialized();

    constexpr double kLeftLon = 8.4991;
    const QGCTileSet tiles = offlineRegionTiles(elevationType, kLeftLon, setMinZoom, setMaxZoom);
    QCOMPARE_GE(tiles.tileCount, quint64(setMaxZoom - setMinZoom + 1));

    OfflineSetsFixture sets;
    QVERIFY(sets.manager.property("fetchElevation").toBool());
    sets.manager.updateForCurrentView(kLeftLon, kOfflineRegionTopLat, kLeftLon + kOfflineRegionLonSpan,
                                      kOfflineRegionBottomLat, minZoom, maxZoom, elevationType);
    QCOMPARE(sets.manager.tileCount(), tiles.tileCount);
    QCOMPARE(sets.manager.tileSize(), tiles.tileSize);

    const QString name = QStringLiteral("elevation type test: %1").arg(QString::fromLatin1(QTest::currentDataTag()));
    sets.manager.startDownload(name, elevationType);
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());

    const QGCCachedTileSet* const set = sets.tileSet(name);
    QVERIFY(set);
    QCOMPARE(sets.manager.tileSets()->count(), 1);
    QCOMPARE(set->type(), elevationType);
    QCOMPARE(set->minZoom(), setMinZoom);
    QCOMPARE(set->maxZoom(), setMaxZoom);
    QCOMPARE(set->totalTileCount(), static_cast<quint32>(tiles.tileCount));

    // The set requests each of its tiles once, as many as a set requests at a time: see _testTerrainSet for the wait
    const int expectedRequestCount = static_cast<int>(qMin(tiles.tileCount, kTilesRequestedAtOnce));
    QTRY_COMPARE_WITH_TIMEOUT(sets.networkManager.requestCount, expectedRequestCount, TestTimeout::mediumMs());
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());
    QCoreApplication::processEvents();
    QCOMPARE(sets.networkManager.requestCount, expectedRequestCount);
}

void QGCMapEngineManagerTest::_testTerrainSetServedFromCache()
{
    ignoreLogMessage("API.QGCApplication.AppMessage", QtDebugMsg,
                     QRegularExpression(QStringLiteral("Restart application for changes to take effect")));
    TestFixtures::SettingsFixture saved;
    selectGeoMapEngines(saved, true, false);

    QGCMapEngine::instance()->ensureInitialized();

    // A region of its own, as its Terrarium tiles end up in the cache. The zoom leaves the terrain set with as many
    // tiles as a set asks for at a time.
    constexpr double kLeftLon = 9.4989;
    constexpr int kZoom = 12;
    constexpr int kTerrainMinZoom = TerrariumElevationProvider::kAnchorZoom;
    constexpr int kTerrainTileCount = (kZoom - kTerrainMinZoom + 1) + kOfflineRegionNeighborAnchorCount;
    constexpr int kRequestCount = 1 + 1 + kTerrainTileCount;
    const QString mapType = QGCMapCatalog::mapList().constFirst();
    const QString terrainType = QString::fromLatin1(TerrariumElevationProvider::kProviderKey);
    QCOMPARE(offlineRegionTiles(terrainType, kLeftLon, kTerrainMinZoom, kZoom).tileCount, quint64(kTerrainTileCount));
    QCOMPARE_LE(quint64(kTerrainTileCount), kTilesRequestedAtOnce);

    OfflineSetsFixture sets;
    sets.networkManager.body = UnitTestTileGenerator::syntheticTerrariumTileData(0, 0, 0);
    sets.manager.updateForCurrentView(kLeftLon, kOfflineRegionTopLat, kLeftLon + kOfflineRegionLonSpan,
                                      kOfflineRegionBottomLat, kZoom, kZoom, mapType);
    const QString name = QStringLiteral("terrain download test");
    sets.manager.startDownload(name, mapType);
    QTRY_COMPARE_WITH_TIMEOUT(sets.networkManager.requestCount, kRequestCount, TestTimeout::mediumMs());

    // Only the terrain set is downloaded
    const QString terrariumUrl = QString::fromLatin1(TerrariumElevationProvider::kProviderURL);
    for (TileReplyCannedReply* const reply : std::as_const(sets.networkManager.replies)) {
        if (reply->request().url().toString().startsWith(terrariumUrl)) {
            reply->finishNow();
        }
    }
    const QGCCachedTileSet* const terrainSet = sets.tileSet(name + QStringLiteral(" Terrain"));
    QVERIFY(terrainSet);
    QTRY_COMPARE_WITH_TIMEOUT(terrainSet->savedTileCount(), static_cast<quint32>(kTerrainTileCount),
                              TestTimeout::mediumMs());
    QCOMPARE(terrainSet->errorCount(), 0U);

    // GeoMap fetches the tiles of the region, and with a tile finer than the anchor zoom the anchors of that tile and
    // of its neighbors, which it waits for. A tile the cache lacks would be asked of the network, which holds its
    // replies, as the forced misses keep the tile generator of the tests from answering for the cache.
    const UnitTestTileGenerator::ForcedMisses forcedMisses(UnitTestTileGenerator::ForcedMisses::kEveryLookup);
    HeightField heightField;
    TerrariumTileFetcher fetcher(nullptr, &sets.networkManager);
    fetcher.setHeightField(&heightField);
    const SharedMapProvider terrainProvider = UrlFactory::getMapProviderFromProviderType(terrainType);
    for (int zoom = kTerrainMinZoom; zoom <= kZoom; zoom++) {
        const TileMath::TileKey key{terrainProvider->long2tileX(kLeftLon, zoom),
                                    terrainProvider->lat2tileY(kOfflineRegionTopLat, zoom), zoom};
        QVERIFY(fetcher.requestTile(key));
        QTRY_VERIFY_WITH_TIMEOUT(heightField.hasTile(key), TestTimeout::mediumMs());
    }
    QCOMPARE(sets.networkManager.requestCount, kRequestCount);
}

// A set tells its manager that it was selected: also a set that the manager created for a download, not only one that
// it loaded from the cache database
void QGCMapEngineManagerTest::_testNewSetTellsManagerOfItsSelection()
{
    QGCMapEngine::instance()->ensureInitialized();

    constexpr double kLeftLon = 8.4991;
    constexpr int kZoom = 12;
    const QString mapType = QGCMapCatalog::mapList().constFirst();

    OfflineSetsFixture sets;
    sets.manager.updateForCurrentView(kLeftLon, kOfflineRegionTopLat, kLeftLon + kOfflineRegionLonSpan,
                                      kOfflineRegionBottomLat, kZoom, kZoom, mapType);
    const QString name = QStringLiteral("selection test");
    sets.manager.startDownload(name, mapType);
    QTRY_VERIFY_WITH_TIMEOUT(sets.tileSet(name), TestTimeout::mediumMs());

    QSignalSpy selectedCountSpy(&sets.manager, &QGCMapEngineManager::selectedCountChanged);
    sets.tileSet(name)->setSelected(true);
    QCOMPARE(selectedCountSpy.count(), 1);
}

// An import that fails must not leave the manager importing, which the user sees as an import without end
void QGCMapEngineManagerTest::_testFailedImportEndsTheImport()
{
    QTemporaryDir tempDir;
    const QString path = tempDir.filePath(QStringLiteral("import.db"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QVERIFY(file.write("not a tile cache") > 0);
    file.close();

    OfflineSetsFixture sets;
    expectLogMessage("MapTiles.QGCMapEngineManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Error in task: Import Tile Sets")));
    QVERIFY(sets.manager.importSets(path));
    QCOMPARE(sets.manager.importAction(), QGCMapEngineManager::ImportAction::ActionImporting);

    QTRY_COMPARE_WITH_TIMEOUT(sets.manager.importAction(), QGCMapEngineManager::ImportAction::ActionDone,
                              TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    QVERIFY(sets.manager.errorMessage().contains(QStringLiteral("Import Tile Sets")));
    // The sets that the manager lists again are of the cache as it is
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());
}

UT_REGISTER_TEST(QGCMapEngineManagerTest, TestLabel::Unit)
