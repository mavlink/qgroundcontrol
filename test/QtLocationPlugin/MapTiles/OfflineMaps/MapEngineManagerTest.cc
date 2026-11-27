#include "MapEngineManagerTest.h"

#include <utility>

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QSet>
#include <QtCore/QTemporaryDir>
#include <QtTest/QSignalSpy>

#include "CachedTileSet.h"
#include "ElevationMapProvider.h"
#include "Fact.h"
#include "Fixtures/RAIIFixtures.h"
#include "FlightMapSettings.h"
#include "FlyViewSettings.h"
#include "HeightField.h"
#include "MapCatalog.h"
#include "MapEngine.h"
#include "MapEngineManager.h"
#include "MapProvider.h"
#include "MapProviderRegistry.h"
#include "MapProviderTable.h"
#include "PlanViewSettings.h"
#include "QmlObjectListModel.h"
#include "SettingsManager.h"
#include "TerrariumTileFetcher.h"
#include "TileCache.h"
#include "TileCacheTestHelpers.h"
#include "TileMath.h"
#include "TileReply.h"
#include "TileReplyNetworkMock.h"
#include "TileSetTasks.h"
#include "UnitTestTileGenerator.h"

namespace {

// A region within one tile of the elevation provider and of every map zoom up to 17, whatever its longitude below.
// Its Terrarium tiles are one of every zoom, and at the anchor zoom the three anchors next to that one as well.
constexpr double kOfflineRegionTopLat = 47.3906;
constexpr double kOfflineRegionBottomLat = 47.3902;
constexpr double kOfflineRegionLonSpan = 0.0004;
constexpr quint64 kOfflineRegionNeighborAnchorCount = 3;

// A set asks for this many tiles at a time. It asks for no more while the network stand-in below holds its replies.
constexpr quint64 kTilesRequestedAtOnce = 6;

TileRegion offlineRegionTiles(const QString& type, double leftLon, int minZoom, int maxZoom)
{
    const SharedMapProvider provider = MapProviderRegistry::getMapProviderFromProviderType(type);
    TileRegion tiles;
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
    OfflineSetsFixture() { networkManager.holdReplies = true; }

    /// Starts the download of a set \a name with the tiles of one zoom level of the region at \a leftLon, and
    /// returns the set once its tiles are requested. Null when that does not come about.
    CachedTileSet* startDownload(const QString& name, double leftLon)
    {
        constexpr int kZoom = 12;
        const QString mapType = MapCatalog::mapList().constFirst();
        const TileRegion tiles = offlineRegionTiles(mapType, leftLon, kZoom, kZoom);
        const int requestCount = networkManager.requestCount + static_cast<int>(tiles.tileCount);
        if ((tiles.tileCount == 0) || (tiles.tileCount > kTilesRequestedAtOnce) ||
            !manager.setProperty("fetchElevation", false)) {
            return nullptr;
        }

        manager.updateForCurrentView(leftLon, kOfflineRegionTopLat, leftLon + kOfflineRegionLonSpan,
                                     kOfflineRegionBottomLat, kZoom, kZoom, mapType);
        manager.startDownload(name, mapType);
        const bool requested = QTest::qWaitFor(
            [this, requestCount] { return networkManager.requestCount == requestCount; }, TestTimeout::mediumMs());
        return requested ? tileSet(name) : nullptr;
    }

    /// Lists the sets of the cache again, and waits for the list
    bool reload()
    {
        manager.loadTileSets();
        return UnitTestTileGenerator::waitForCacheTasks();
    }

    /// Whether a request of a set is still open
    bool hasOpenRequest() const
    {
        for (const QPointer<TileReplyCannedReply>& reply : networkManager.replies) {
            if (reply) {
                return true;
            }
        }
        return false;
    }

    CachedTileSet* tileSet(const QString& name)
    {
        for (QObject* const object : std::as_const(*manager.tileSets()->objectList())) {
            CachedTileSet* const set = qobject_cast<CachedTileSet*>(object);
            if (set->name() == name) {
                return set;
            }
        }
        return nullptr;
    }

    TileReplyMockNam networkManager;
    const TileReplyNetworkScope networkScope{&networkManager};
    MapEngineManager manager;
};

}  // namespace

void MapEngineManagerTest::initTestCase()
{
    UnitTest::initTestCase();
    // The tests fetch tiles that the tile cache of the process must not have yet
    QVERIFY(UnitTestTileGenerator::resetTileCache());
}

void MapEngineManagerTest::_testTerrainSet_data()
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

void MapEngineManagerTest::_testTerrainSet()
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

    TileCache::instance()->ensureInitialized();

    constexpr double kLeftLon = 8.4991;
    const QString mapType = MapCatalog::mapList().constFirst();
    const QString elevationType =
        SettingsManager::instance()->flightMapSettings()->elevationMapProvider()->rawValue().toString();
    const QString terrainType = QString::fromLatin1(TerrariumElevationProvider::kProviderKey);
    const TileRegion imageTiles = offlineRegionTiles(mapType, kLeftLon, minZoom, maxZoom);
    const TileRegion elevationTiles = fetchElevation ? offlineRegionTiles(elevationType, kLeftLon, 1, 1) : TileRegion();
    const TileRegion terrainTiles =
        terrainExpected ? offlineRegionTiles(terrainType, kLeftLon, terrainMinZoom, terrainMaxZoom) : TileRegion();
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
    const CachedTileSet* const terrainSet = sets.tileSet(name + QStringLiteral(" Terrain"));
    QCOMPARE(terrainSet != nullptr, terrainExpected);
    if (terrainSet) {
        QCOMPARE(terrainSet->mapTypeStr(), terrainType);
        QCOMPARE(terrainSet->minZoom(), terrainMinZoom);
        QCOMPARE(terrainSet->maxZoom(), terrainMaxZoom);
        QCOMPARE(terrainSet->totalTileCount(), static_cast<quint32>(terrainTiles.tileCount));
    }

    const auto requestedAtOnce = [](const TileRegion& tiles) { return qMin(tiles.tileCount, kTilesRequestedAtOnce); };
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

void MapEngineManagerTest::_testElevationTypeSet_data()
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
void MapEngineManagerTest::_testElevationTypeSet()
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

    TileCache::instance()->ensureInitialized();

    constexpr double kLeftLon = 8.4991;
    const TileRegion tiles = offlineRegionTiles(elevationType, kLeftLon, setMinZoom, setMaxZoom);
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

    const CachedTileSet* const set = sets.tileSet(name);
    QVERIFY(set);
    QCOMPARE(sets.manager.tileSets()->count(), 1);
    QCOMPARE(set->mapTypeStr(), elevationType);
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

void MapEngineManagerTest::_testSetStopsAtDeepestZoomOfProvider_data()
{
    QTest::addColumn<int>("minZoomOffset");
    QTest::addColumn<int>("maxZoomOffset");
    QTest::addColumn<int>("setMinZoomOffset");

    // Offsets from the deepest zoom level of the provider
    QTest::newRow("zooms that end below the deepest tiles") << -1 << 1 << -1;
    QTest::newRow("zooms that are all below the deepest tiles") << 1 << 1 << 0;
}

// The server of a provider has no tile below its deepest zoom level: a set that asked for one would only get errors
void MapEngineManagerTest::_testSetStopsAtDeepestZoomOfProvider()
{
    QFETCH(int, minZoomOffset);
    QFETCH(int, maxZoomOffset);
    QFETCH(int, setMinZoomOffset);

    TileCache::instance()->ensureInitialized();

    const QString mapType = QStringLiteral("Street Map");
    const SharedMapProvider provider = MapProviderRegistry::getMapProviderFromProviderType(mapType);
    QVERIFY(provider);
    const int deepestZoom = provider->maxZoom();
    QCOMPARE_LT(deepestZoom, QGC_MAX_MAP_ZOOM);

    constexpr double kLeftLon = 10.4993;
    const int setMinZoom = deepestZoom + setMinZoomOffset;
    const TileRegion tiles = offlineRegionTiles(mapType, kLeftLon, setMinZoom, deepestZoom);
    QCOMPARE_GT(tiles.tileCount, quint64(0));

    // The set of the map only, without sets of elevation tiles beside it
    OfflineSetsFixture sets;
    QVERIFY(sets.manager.setProperty("fetchElevation", false));
    sets.manager.updateForCurrentView(kLeftLon, kOfflineRegionTopLat, kLeftLon + kOfflineRegionLonSpan,
                                      kOfflineRegionBottomLat, deepestZoom + minZoomOffset, deepestZoom + maxZoomOffset,
                                      mapType);
    QCOMPARE(sets.manager.tileCount(), tiles.tileCount);
    QCOMPARE(sets.manager.tileSize(), tiles.tileSize);

    const QString name = QStringLiteral("deepest zoom test: %1").arg(QString::fromLatin1(QTest::currentDataTag()));
    sets.manager.startDownload(name, mapType);
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());

    const CachedTileSet* const set = sets.tileSet(name);
    QVERIFY(set);
    QCOMPARE(set->minZoom(), setMinZoom);
    QCOMPARE(set->maxZoom(), deepestZoom);
    QCOMPARE(set->totalTileCount(), static_cast<quint32>(tiles.tileCount));
}

void MapEngineManagerTest::_testTerrainSetServedFromCache()
{
    ignoreLogMessage("API.QGCApplication.AppMessage", QtDebugMsg,
                     QRegularExpression(QStringLiteral("Restart application for changes to take effect")));
    TestFixtures::SettingsFixture saved;
    selectGeoMapEngines(saved, true, false);

    TileCache::instance()->ensureInitialized();

    // A region of its own, as its Terrarium tiles end up in the cache. The zoom leaves the terrain set with as many
    // tiles as a set asks for at a time.
    constexpr double kLeftLon = 9.4989;
    constexpr int kZoom = 12;
    constexpr int kTerrainMinZoom = TerrariumElevationProvider::kAnchorZoom;
    constexpr int kTerrainTileCount = (kZoom - kTerrainMinZoom + 1) + kOfflineRegionNeighborAnchorCount;
    constexpr int kRequestCount = 1 + 1 + kTerrainTileCount;
    const QString mapType = MapCatalog::mapList().constFirst();
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
    const CachedTileSet* const terrainSet = sets.tileSet(name + QStringLiteral(" Terrain"));
    QVERIFY(terrainSet);
    QTRY_COMPARE_WITH_TIMEOUT(terrainSet->savedTileCount(), static_cast<quint32>(kTerrainTileCount),
                              TestTimeout::mediumMs());
    QCOMPARE(terrainSet->errorCount(), 0U);

    // GeoMap fetches the tiles of the region, and with a tile finer than the anchor zoom the anchors of that tile and
    // of its neighbors, which it waits for. A tile the cache lacks would be asked of the network, which holds its
    // replies, as the forced misses keep the tile generator of the tests from answering for the cache.
    const UnitTestTileGenerator::ForcedMisses forcedMisses(UnitTestTileGenerator::ForcedMisses::kEveryLookup);
    HeightField heightField;
    TerrariumTileFetcher fetcher;
    fetcher.setHeightField(&heightField);
    const SharedMapProvider terrainProvider = MapProviderRegistry::getMapProviderFromProviderType(terrainType);
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
void MapEngineManagerTest::_testNewSetTellsManagerOfItsSelection()
{
    TileCache::instance()->ensureInitialized();

    constexpr double kLeftLon = 8.4991;
    constexpr int kZoom = 12;
    const QString mapType = MapCatalog::mapList().constFirst();

    OfflineSetsFixture sets;
    sets.manager.updateForCurrentView(kLeftLon, kOfflineRegionTopLat, kLeftLon + kOfflineRegionLonSpan,
                                      kOfflineRegionBottomLat, kZoom, kZoom, mapType);
    const QString name = QStringLiteral("selection test");
    sets.manager.startDownload(name, mapType);
    QTRY_VERIFY_WITH_TIMEOUT(sets.tileSet(name), TestTimeout::mediumMs());

    QSignalSpy selectedCountSpy(&sets.manager, &MapEngineManager::selectedCountChanged);
    sets.tileSet(name)->setSelected(true);
    QCOMPARE(selectedCountSpy.count(), 1);
}

// The Maps page and the editor of a set list the sets again when they open. A set that is listed stays the object it
// is: one that took its place would not be the one that downloads, and the UI would show a download that stands still.
void MapEngineManagerTest::_testReloadKeepsSetsAndTheirDownloads()
{
    TileCache::instance()->ensureInitialized();

    QPointer<CachedTileSet> set;
    {
        OfflineSetsFixture sets;
        const QString name = QStringLiteral("reload test");
        set = sets.startDownload(name, 11.4991);
        QVERIFY(set);
        QVERIFY(set->downloading());
        set->setSelected(true);
        const int requestCount = sets.networkManager.requestCount;

        // Twice at once, as when the page opens the editor of a set
        sets.manager.loadTileSets();
        QVERIFY(sets.reload());

        QCOMPARE(sets.tileSet(name), set.data());
        QVERIFY(set->downloading());
        QVERIFY(set->selected());
        QVERIFY(sets.hasOpenRequest());
        QCOMPARE(sets.networkManager.requestCount, requestCount);

        // Each set of the cache once, in the order of the cache
        const QObjectList listed = *sets.manager.tileSets()->objectList();
        QCOMPARE_GE(listed.count(), 2);
        QVERIFY(qobject_cast<CachedTileSet*>(listed.constFirst())->defaultSet());
        QSet<quint64> ids;
        for (QObject* const object : listed) {
            ids.insert(qobject_cast<CachedTileSet*>(object)->id());
        }
        QCOMPARE(ids.count(), listed.count());
    }
    // The sets are of their manager
    QVERIFY(!set);
}

// A cache that was replaced by an imported one gives the ids of its sets again, and a merge can delete a set
void MapEngineManagerTest::_testReloadDropsSetsThatTheCacheLacks()
{
    TileCache::instance()->ensureInitialized();

    OfflineSetsFixture sets;
    const QString name = QStringLiteral("replaced set test");
    const QPointer<CachedTileSet> set = sets.startDownload(name, 12.4991);
    QVERIFY(set);
    const quint64 setID = set->id();
    const int maxZoom = set->maxZoom();

    // Another set has the id: it covers another zoom level
    QVERIFY(tileCacheWorkerRows(this, tileCacheProcessAddTask,
                                QStringLiteral("UPDATE TileSets SET maxZoom = maxZoom + 1 WHERE setID = %1").arg(setID))
                .has_value());
    QVERIFY(sets.reload());

    QVERIFY(!set);
    const QPointer<CachedTileSet> otherSet = sets.tileSet(name);
    QVERIFY(otherSet);
    QCOMPARE(otherSet->id(), setID);
    QCOMPARE(otherSet->maxZoom(), maxZoom + 1);
    QVERIFY(!otherSet->downloading());
    // The requests of the set that is gone are dropped
    QTRY_VERIFY_WITH_TIMEOUT(!sets.hasOpenRequest(), TestTimeout::mediumMs());

    QCOMPARE(tileCacheRunTask(this, tileCacheProcessAddTask, TileSetTasks::deleteTileSet(setID)), QString());
    QVERIFY(sets.reload());

    QVERIFY(!otherSet);
    QVERIFY(!sets.tileSet(name));
}

// "Clear Cache" deletes the tiles that the maps saved as they were looked at, and no tile of an offline set
void MapEngineManagerTest::_testClearCachedTilesKeepsOfflineSets()
{
    TileCache::instance()->ensureInitialized();

    OfflineSetsFixture sets;
    sets.networkManager.body = tileReplyPngBody();
    const QString name = QStringLiteral("clear cache test");
    CachedTileSet* const set = sets.startDownload(name, 15.4991);
    QVERIFY(set);
    for (TileReplyCannedReply* const reply : std::as_const(sets.networkManager.replies)) {
        reply->finishNow();
    }
    QTRY_VERIFY_WITH_TIMEOUT(!set->downloading(), TestTimeout::mediumMs());
    const quint32 setTileCount = set->savedTileCount();
    QCOMPARE_GT(setTileCount, 0U);
    QVERIFY(set->complete());

    // A tile that a map saved, of a place that no set has
    const SharedMapProvider provider = MapProviderRegistry::getMapProviderFromProviderType(set->mapTypeStr());
    TileCache::instance()->cacheTile(provider->mapId(), provider->getTileHash(1, 1, 2), tileReplyPngBody(),
                                     QStringLiteral("png"));
    QVERIFY(sets.reload());
    CachedTileSet* const defaultSet = qobject_cast<CachedTileSet*>(sets.manager.tileSets()->get(0));
    QVERIFY(defaultSet->defaultSet());
    // The default set tells of every tile of the cache: those of earlier tests are among them
    const quint32 cachedBefore = defaultSet->savedTileCount();
    QCOMPARE_GE(cachedBefore, setTileCount + 1);

    sets.manager.clearCachedTiles();
    QVERIFY(defaultSet->deleting());
    QTRY_VERIFY_WITH_TIMEOUT(!defaultSet->deleting(), TestTimeout::mediumMs());
    QVERIFY2(sets.manager.errorMessage().isEmpty(), qPrintable(sets.manager.errorMessage()));

    QVERIFY(sets.reload());
    QCOMPARE(sets.tileSet(name), set);
    QCOMPARE(set->savedTileCount(), setTileCount);
    QVERIFY(set->complete());
    QCOMPARE_LT(defaultSet->savedTileCount(), cachedBefore);
    // What is left is of the offline sets
    QCOMPARE(defaultSet->uniqueTileCount(), 0U);
}

// The manager shows the user what the cache fails to do for the download of a set, as for its own tasks
void MapEngineManagerTest::_testShowsErrorOfDownloadOfSet()
{
    TileCache::instance()->ensureInitialized();
    const auto cacheExec = [this](const QString& sql) {
        return tileCacheWorkerRows(this, tileCacheProcessAddTask, sql).has_value();
    };

    OfflineSetsFixture sets;
    CachedTileSet* const set = sets.startDownload(QStringLiteral("download error test"), 14.4991);
    QVERIFY(set);
    set->cancelDownloadTask();
    QVERIFY(sets.manager.errorMessage().isEmpty());

    // The database refuses to mark the tiles of the set as pending again, which a resumed download asks for first
    const auto dropTrigger =
        qScopeGuard([&cacheExec] { (void) cacheExec(QStringLiteral("DROP TRIGGER refuse_pending")); });
    QVERIFY(
        cacheExec(QStringLiteral("CREATE TRIGGER refuse_pending BEFORE UPDATE ON TilesDownload "
                                 "WHEN NEW.state = 0 BEGIN SELECT RAISE(ABORT, 'refused'); END")));
    expectLogMessage("MapTiles.TileCacheDatabase", QtWarningMsg,
                     QRegularExpression(QStringLiteral("set TilesDownload states of set")));
    expectLogMessage("MapTiles.MapEngineManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Error in task: Reset Tile Downloads")));
    expectLogMessage("MapTiles.TileSetDownloader", QtWarningMsg,
                     QRegularExpression(QStringLiteral("could not be reset to download them again")));
    expectLogMessage("MapTiles.TileSetDownloader", QtWarningMsg,
                     QRegularExpression(QStringLiteral("No list of the tiles to download")));
    set->resumeDownloadTask();
    QTRY_VERIFY_WITH_TIMEOUT(!set->downloading(), TestTimeout::mediumMs());
    for (int i = 0; i < 4; i++) {
        verifyExpectedLogMessage();
    }

    QVERIFY(sets.manager.errorMessage().contains(QStringLiteral("Reset Tile Downloads")));
    QCOMPARE(set->errorCount(), 1U);
}

void MapEngineManagerTest::_testDeletedSetStopsDownloading_data()
{
    QTest::addColumn<bool>("deleteDefaultSet");

    QTest::newRow("the set that downloads") << false;
    // Deletes every set, with what the cache has
    QTest::newRow("the default set") << true;
}

// "Delete" is offered for a set that downloads
void MapEngineManagerTest::_testDeletedSetStopsDownloading()
{
    QFETCH(bool, deleteDefaultSet);

    TileCache::instance()->ensureInitialized();

    OfflineSetsFixture sets;
    const QString name = QStringLiteral("delete test: %1").arg(QString::fromLatin1(QTest::currentDataTag()));
    const QPointer<CachedTileSet> set = sets.startDownload(name, 13.4991);
    QVERIFY(set);
    QVERIFY(set->downloading());
    QVERIFY(sets.reload());
    CachedTileSet* const defaultSet = qobject_cast<CachedTileSet*>(sets.manager.tileSets()->get(0));
    QVERIFY(defaultSet->defaultSet());

    sets.manager.deleteTileSet(deleteDefaultSet ? defaultSet : set.data());
    QVERIFY(set);
    QVERIFY(!set->downloading());
    QVERIFY(set->deleting());

    QTRY_VERIFY_WITH_TIMEOUT(!set, TestTimeout::mediumMs());
    QVERIFY(!sets.tileSet(name));
    QTRY_VERIFY_WITH_TIMEOUT(!sets.hasOpenRequest(), TestTimeout::mediumMs());

    // The default set is there to be used, alone when every set went with it
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());
    const QObjectList listed = *sets.manager.tileSets()->objectList();
    QVERIFY(!listed.isEmpty());
    const CachedTileSet* const listedDefaultSet = qobject_cast<CachedTileSet*>(listed.constFirst());
    QVERIFY(listedDefaultSet->defaultSet());
    QVERIFY(!listedDefaultSet->deleting());
    if (deleteDefaultSet) {
        QCOMPARE(listed.count(), 1);
    }
}

void MapEngineManagerTest::_testFailedDeleteLeavesNothingDeleting_data()
{
    QTest::addColumn<bool>("clearCache");
    QTest::addColumn<double>("longitude");  // a region of its own for every row
    QTest::addColumn<QString>("refusedTable");
    QTest::addColumn<QString>("taskName");

    QTest::newRow("delete of a set") << false << 19.4991 << QStringLiteral("TileSets")
                                     << QStringLiteral("Delete Tile Set");
    QTest::newRow("clear of the cached tiles")
        << true << 20.4991 << QStringLiteral("Tiles") << QStringLiteral("Clear Tile Cache");
}

// What the cache fails to delete is not shown as being deleted for ever, and the user is told why it is still there
void MapEngineManagerTest::_testFailedDeleteLeavesNothingDeleting()
{
    QFETCH(bool, clearCache);
    QFETCH(double, longitude);
    QFETCH(QString, refusedTable);
    QFETCH(QString, taskName);

    TileCache::instance()->ensureInitialized();
    const auto cacheExec = [this](const QString& sql) {
        return tileCacheWorkerRows(this, tileCacheProcessAddTask, sql).has_value();
    };

    OfflineSetsFixture sets;
    const QString name = QStringLiteral("failed delete test: %1").arg(QString::fromLatin1(QTest::currentDataTag()));
    CachedTileSet* const set = sets.startDownload(name, longitude);
    QVERIFY(set);
    // A tile that a map saved, of a place that no set has: one that "Clear Cache" deletes
    const SharedMapProvider provider = MapProviderRegistry::getMapProviderFromProviderType(set->mapTypeStr());
    TileCache::instance()->cacheTile(provider->mapId(), provider->getTileHash(2, 1, 2), tileReplyPngBody(),
                                     QStringLiteral("png"));
    QVERIFY(sets.reload());
    CachedTileSet* const defaultSet = qobject_cast<CachedTileSet*>(sets.manager.tileSets()->get(0));
    QVERIFY(defaultSet->defaultSet());
    const CachedTileSet* const busySet = clearCache ? defaultSet : set;

    const auto dropTrigger =
        qScopeGuard([&cacheExec] { (void) cacheExec(QStringLiteral("DROP TRIGGER refuse_delete")); });
    QVERIFY(cacheExec(QStringLiteral("CREATE TRIGGER refuse_delete BEFORE DELETE ON %1 "
                                     "BEGIN SELECT RAISE(ABORT, 'refused'); END")
                          .arg(refusedTable)));
    expectLogMessage("MapTiles.TileCacheDatabase", QtWarningMsg, QRegularExpression(QStringLiteral("refused")));
    expectLogMessage("MapTiles.MapEngineManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Error in task: %1").arg(taskName)));
    if (clearCache) {
        sets.manager.clearCachedTiles();
    } else {
        sets.manager.deleteTileSet(set);
    }
    QVERIFY(busySet->deleting());
    QTRY_VERIFY_WITH_TIMEOUT(!busySet->deleting(), TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();

    QVERIFY2(sets.manager.errorMessage().contains(taskName), qPrintable(sets.manager.errorMessage()));
    QCOMPARE(sets.tileSet(name), set);
}

// The editor of a set renames it to what the user typed, which can be the name of another set: the set gets a name
// that tells the two apart, in the cache as well
void MapEngineManagerTest::_testRenamedSetGetsNameThatNoOtherSetHas()
{
    TileCache::instance()->ensureInitialized();

    OfflineSetsFixture sets;
    const QString takenName = QStringLiteral("rename test: taken");
    QVERIFY(sets.startDownload(takenName, 16.4991));
    CachedTileSet* const renamed = sets.startDownload(QStringLiteral("rename test: other"), 17.4991);
    QVERIFY(renamed);
    QVERIFY(sets.manager.findName(takenName));

    sets.manager.renameTileSet(renamed, takenName);
    QCOMPARE(renamed->name(), takenName + QStringLiteral(" (1)"));
    const QString nameSql = QStringLiteral("SELECT name FROM TileSets WHERE setID = %1").arg(renamed->id());
    QCOMPARE(tileCacheWorkerRows(this, tileCacheProcessAddTask, nameSql), QStringList{renamed->name()});

    // The name that "Add New Set" proposes is one that no set has
    const QString proposedName = sets.manager.getUniqueName();
    QVERIFY(!sets.manager.findName(proposedName));
    sets.manager.renameTileSet(renamed, proposedName);
    QCOMPARE(renamed->name(), proposedName);
    QCOMPARE(tileCacheWorkerRows(this, tileCacheProcessAddTask, nameSql), QStringList{proposedName});
    QVERIFY(sets.manager.getUniqueName() != proposedName);
    QVERIFY2(sets.manager.errorMessage().isEmpty(), qPrintable(sets.manager.errorMessage()));
}

// "Export" writes the sets that the user selected to a file
void MapEngineManagerTest::_testExportsSelectedSets()
{
    TileCache::instance()->ensureInitialized();
    QTemporaryDir tempDir;
    const QString path = tempDir.filePath(QStringLiteral("export.qgctiledb"));

    OfflineSetsFixture sets;
    sets.networkManager.body = tileReplyPngBody();
    const QString name = QStringLiteral("export test");
    CachedTileSet* const set = sets.startDownload(name, 18.4991);
    QVERIFY(set);
    for (TileReplyCannedReply* const reply : std::as_const(sets.networkManager.replies)) {
        reply->finishNow();
    }
    QTRY_VERIFY_WITH_TIMEOUT(!set->downloading(), TestTimeout::mediumMs());
    QCOMPARE_GT(set->savedTileCount(), 0U);
    QVERIFY(sets.reload());
    QCOMPARE_GE(sets.manager.tileSets()->count(), 2);

    // Without a selection there is nothing to export, and no export that would go on
    QCOMPARE(sets.manager.selectedCount(), 0);
    QVERIFY(!sets.manager.exportSets(path));
    QCOMPARE(sets.manager.importAction(), MapEngineManager::ImportAction::ActionNone);
    QVERIFY(!QFileInfo::exists(path));

    set->setSelected(true);
    QCOMPARE(sets.manager.selectedCount(), 1);
    QVERIFY(sets.manager.exportSets(path));
    QCOMPARE(sets.manager.importAction(), MapEngineManager::ImportAction::ActionExporting);
    QTRY_COMPARE_WITH_TIMEOUT(sets.manager.importAction(), MapEngineManager::ImportAction::ActionDone,
                              TestTimeout::mediumMs());
    QVERIFY2(sets.manager.errorMessage().isEmpty(), qPrintable(sets.manager.errorMessage()));

    // The file has the selected set with its tiles, and no other
    QCOMPARE(tileCacheRows(path, QStringLiteral("SELECT S.name, (SELECT COUNT(*) FROM SetTiles L WHERE L.setID = "
                                                "S.setID) FROM TileSets S")),
             QStringList{QStringLiteral("%1|%2").arg(name).arg(set->savedTileCount())});

    sets.manager.selectAll();
    QCOMPARE(sets.manager.selectedCount(), static_cast<int>(sets.manager.tileSets()->count()));
}

// An import that fails must not leave the manager importing, which the user sees as an import without end
void MapEngineManagerTest::_testFailedImportEndsTheImport()
{
    QTemporaryDir tempDir;
    const QString path = tempDir.filePath(QStringLiteral("import.db"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QVERIFY(file.write("not a tile cache") > 0);
    file.close();

    OfflineSetsFixture sets;
    expectLogMessage("MapTiles.MapEngineManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Error in task: Import Tile Sets")));
    QVERIFY(sets.manager.importSets(path));
    QCOMPARE(sets.manager.importAction(), MapEngineManager::ImportAction::ActionImporting);

    QTRY_COMPARE_WITH_TIMEOUT(sets.manager.importAction(), MapEngineManager::ImportAction::ActionDone,
                              TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    QVERIFY(sets.manager.errorMessage().contains(QStringLiteral("Import Tile Sets")));
    // The sets that the manager lists again are of the cache as it is
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());
}

// The Import button takes an MBTiles file as well: its tiles become an offline set of the map of the imported tiles,
// which shows them at once
void MapEngineManagerTest::_testImportedMBTilesFileIsAMapOfItsTiles()
{
    // The tile of the row 5 from the south of the 8 of zoom level 3 is the third from the north
    constexpr int kZoom = 3;
    constexpr int kColumn = 4;
    constexpr int kRowFromSouth = 5;
    constexpr int kRowFromNorth = 2;
    const QByteArray picture = QByteArray::fromHex("89504E470D0A1A0A") + QByteArrayLiteral("imported");

    QTemporaryDir tempDir;
    const QString path = tempDir.filePath(QStringLiteral("Imported Field.mbtiles"));
    QVERIFY(tileCacheCreateMBTiles(path, QString(),
                                   QStringLiteral("(%1, %2, %3, x'%4')")
                                       .arg(kZoom)
                                       .arg(kColumn)
                                       .arg(kRowFromSouth)
                                       .arg(QString::fromLatin1(picture.toHex()))));

    OfflineSetsFixture sets;
    QSignalSpy sourceSpy(MapEngine::instance(), &MapEngine::tileSourceChanged);
    QVERIFY(sets.manager.importSets(path));
    QTRY_COMPARE_WITH_TIMEOUT(sets.manager.importAction(), MapEngineManager::ImportAction::ActionDone,
                              TestTimeout::mediumMs());
    QVERIFY2(sets.manager.errorMessage().isEmpty(), qPrintable(sets.manager.errorMessage()));

    // The maps that showed the map of the imported tiles ask for its tiles again
    QCOMPARE(sourceSpy.count(), 1);
    QCOMPARE(sourceSpy.constFirst().constFirst().toInt(), MapProviderTable::kImportedMapId);

    QTRY_VERIFY_WITH_TIMEOUT(sets.tileSet(QStringLiteral("Imported Field")), TestTimeout::mediumMs());
    const CachedTileSet* const set = sets.tileSet(QStringLiteral("Imported Field"));
    QCOMPARE(set->mapTypeStr(), QStringLiteral("MBTiles Imported"));
    QCOMPARE(set->savedTileCount(), 1U);
    QVERIFY(set->complete());

    TileReply reply(MapProviderTable::kImportedMapId, kColumn, kRowFromNorth, kZoom);
    QSignalSpy finishedSpy(&reply, &TileReply::finished);
    reply.start();
    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(reply.error(), TileReply::Error::NoError);
    QCOMPARE(reply.data(), picture);
    QCOMPARE(reply.format(), QStringLiteral("png"));
    QCOMPARE(sets.networkManager.requestCount, 0);
}

UT_REGISTER_TEST(MapEngineManagerTest, TestLabel::Unit)
