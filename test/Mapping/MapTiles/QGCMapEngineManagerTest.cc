#include "QGCMapEngineManagerTest.h"

#include <utility>

#include <QtCore/QCoreApplication>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtTest/QSignalSpy>

#include "ElevationMapProvider.h"
#include "Fact.h"
#include "FlightMapSettings.h"
#include "FlyViewSettings.h"
#include "HeightField.h"
#include "PlanViewSettings.h"
#include "QGCCachedTileSet.h"
#include "QGCMapEngine.h"
#include "QGCMapEngineManager.h"
#include "QGCMapTasks.h"
#include "QGCMapUrlEngine.h"
#include "QmlObjectListModel.h"
#include "SettingsManager.h"
#include "TerrariumTileFetcher.h"
#include "TileMath.h"
#include "TileReplyNetworkMock.h"
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

QGCTileSet offlineRegionTiles(const QString& type, double leftLon, int minZoom, int maxZoom)
{
    QGCTileSet tiles;
    for (int zoom = minZoom; zoom <= maxZoom; zoom++) {
        tiles += UrlFactory::getTileCount(zoom, leftLon, kOfflineRegionTopLat, leftLon + kOfflineRegionLonSpan,
                                          kOfflineRegionBottomLat, type);
    }
    return tiles;
}

/// Selects the map engine of the Fly View and of the Plan View until the returned guard is gone
[[nodiscard]] auto selectGeoMapEngines(bool flyView, bool planView)
{
    Fact* const flyViewFact = SettingsManager::instance()->flyViewSettings()->useGeoMapEngine();
    Fact* const planViewFact = SettingsManager::instance()->planViewSettings()->useGeoMapEngine();
    auto restore = qScopeGuard(
        [flyViewFact, planViewFact, flyViewSaved = flyViewFact->rawValue(), planViewSaved = planViewFact->rawValue()] {
            flyViewFact->setRawValue(flyViewSaved);
            planViewFact->setRawValue(planViewSaved);
        });
    flyViewFact->setRawValue(flyView);
    planViewFact->setRawValue(planView);
    return restore;
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

/// Waits until the database worker has run the tasks queued so far, and the main thread has handled what they
/// reported: the worker runs its tasks in order
[[nodiscard]] bool offlineSetsTasksDone()
{
    bool done = false;
    // Declared after what its signal writes, so that a late signal is dropped
    const QObject sentinelContext;
    QGCFetchTileTask* const sentinelTask = new QGCFetchTileTask(QStringLiteral("no such tile"));
    (void) QObject::connect(sentinelTask, &QGCMapTask::error, &sentinelContext, [&done] { done = true; });
    if (!getQGCMapEngine()->addTask(sentinelTask)) {
        sentinelTask->deleteLater();
        return false;
    }
    return QTest::qWaitFor([&done] { return done; }, TestTimeout::mediumMs());
}

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
    const auto restoreMapEngines = selectGeoMapEngines(flyViewGeoMap, planViewGeoMap);

    QGCMapEngine* const engine = getQGCMapEngine();
    engine->ensureInitialized();

    constexpr double kLeftLon = 8.4991;
    const QString mapType = QGCMapEngineManager::mapList().constFirst();
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
    QVERIFY(offlineSetsTasksDone());

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
    QVERIFY(offlineSetsTasksDone());
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
    const auto restoreMapEngines = selectGeoMapEngines(true, false);

    getQGCMapEngine()->ensureInitialized();

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
    QVERIFY(offlineSetsTasksDone());

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
    QVERIFY(offlineSetsTasksDone());
    QCoreApplication::processEvents();
    QCOMPARE(sets.networkManager.requestCount, expectedRequestCount);
}

void QGCMapEngineManagerTest::_testTerrainSetServedFromCache()
{
    ignoreLogMessage("API.QGCApplication.AppMessage", QtDebugMsg,
                     QRegularExpression(QStringLiteral("Restart application for changes to take effect")));
    const auto restoreMapEngines = selectGeoMapEngines(true, false);

    getQGCMapEngine()->ensureInitialized();

    // A region of its own, as its Terrarium tiles end up in the cache. The zoom leaves the terrain set with as many
    // tiles as a set asks for at a time.
    constexpr double kLeftLon = 9.4989;
    constexpr int kZoom = 12;
    constexpr int kTerrainMinZoom = TerrariumElevationProvider::kAnchorZoom;
    constexpr int kTerrainTileCount = (kZoom - kTerrainMinZoom + 1) + kOfflineRegionNeighborAnchorCount;
    constexpr int kRequestCount = 1 + 1 + kTerrainTileCount;
    const QString mapType = QGCMapEngineManager::mapList().constFirst();
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
    UnitTestTileGenerator::setForcedMissCount(kTerrainTileCount);
    const auto resetForcedMisses = qScopeGuard([] { UnitTestTileGenerator::setForcedMissCount(0); });
    HeightField heightField;
    TerrariumTileFetcher fetcher(nullptr, &sets.networkManager);
    fetcher.setHeightField(&heightField);
    for (int zoom = kTerrainMinZoom; zoom <= kZoom; zoom++) {
        const TileMath::TileKey key{UrlFactory::long2tileX(terrainType, kLeftLon, zoom),
                                    UrlFactory::lat2tileY(terrainType, kOfflineRegionTopLat, zoom), zoom};
        QVERIFY(fetcher.requestTile(key));
        QTRY_VERIFY_WITH_TIMEOUT(heightField.hasTile(key), TestTimeout::mediumMs());
    }
    QCOMPARE(sets.networkManager.requestCount, kRequestCount);
}

// A set tells its manager that it was selected: also a set that the manager created for a download, not only one that
// it loaded from the cache database
void QGCMapEngineManagerTest::_testNewSetTellsManagerOfItsSelection()
{
    getQGCMapEngine()->ensureInitialized();

    constexpr double kLeftLon = 8.4991;
    constexpr int kZoom = 12;
    const QString mapType = QGCMapEngineManager::mapList().constFirst();

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

void QGCMapEngineManagerTest::_testMapAttribution_data()
{
    QTest::addColumn<QString>("mapType");
    QTest::addColumn<QString>("attribution");

    // The wording that the terms of these providers prescribe
    QTest::newRow("OpenStreetMap") << QStringLiteral("Street Map") << QStringLiteral("© OpenStreetMap contributors");
    QTest::newRow("Mapbox") << QStringLiteral("Mapbox Streets") << QStringLiteral("© Mapbox © OpenStreetMap");
    QTest::newRow("Mapbox satellite") << QStringLiteral("Mapbox Hybrid")
                                      << QStringLiteral("© Mapbox © OpenStreetMap © Maxar");
    QTest::newRow("Esri") << QStringLiteral("Esri World Satellite") << QStringLiteral("Powered by Esri");
    QTest::newRow("Kartverket") << QStringLiteral("Statkart Topo") << QStringLiteral("© Kartverket");
    QTest::newRow("Norwegian Polar Institute")
        << QStringLiteral("Svalbard Topo") << QStringLiteral("© Norwegian Polar Institute");

    QTest::newRow("GeoMap terrain") << QString::fromLatin1(TerrariumElevationProvider::kProviderKey)
                                    << QStringLiteral("Terrain: Mapzen, USGS, NOAA and other sources");
    QTest::newRow("source that the user sets") << QStringLiteral("CustomURL Custom") << QString();
    QTest::newRow("not a map") << QStringLiteral("No Such Map") << QString();
}

void QGCMapEngineManagerTest::_testMapAttribution()
{
    QFETCH(QString, mapType);
    QFETCH(QString, attribution);

    QCOMPARE(QGCMapEngineManager::mapAttribution(mapType), attribution);
}

void QGCMapEngineManagerTest::_testTerrainAttribution()
{
    QCOMPARE(QGCMapEngineManager::terrainAttribution(),
             QStringLiteral("Terrain: Mapzen, USGS, NOAA and other sources"));
}

void QGCMapEngineManagerTest::_testIsSatelliteMap_data()
{
    QTest::addColumn<QString>("mapType");
    QTest::addColumn<bool>("satellite");

    // Satellite or aerial pictures whose names say neither "Satellite" nor "Hybrid"
    QTest::newRow("MapQuest Sat") << QStringLiteral("MapQuest Sat") << true;
    QTest::newRow("LINZ Basemap") << QStringLiteral("LINZ Basemap") << true;
    QTest::newRow("Japan-GSI Seamless") << QStringLiteral("Japan-GSI Seamless") << true;

    QTest::newRow("hybrid map") << QStringLiteral("Bing Hybrid") << true;
    QTest::newRow("street map") << QStringLiteral("Bing Road") << false;
    QTest::newRow("elevation provider") << QString::fromLatin1(CopernicusElevationProvider::kProviderKey) << false;
    QTest::newRow("not a map") << QStringLiteral("No Such Map Satellite") << false;
}

void QGCMapEngineManagerTest::_testIsSatelliteMap()
{
    QFETCH(QString, mapType);
    QFETCH(bool, satellite);

    QCOMPARE(QGCMapEngineManager::isSatelliteMap(mapType), satellite);
}

void QGCMapEngineManagerTest::_testIsImageryMap_data()
{
    QTest::addColumn<QString>("mapType");
    QTest::addColumn<bool>("imagery");

    QTest::newRow("map") << QStringLiteral("Bing Hybrid") << true;
    QTest::newRow("map without a type") << QStringLiteral("OpenAIP") << true;
    QTest::newRow("elevation provider") << QString::fromLatin1(CopernicusElevationProvider::kProviderKey) << false;
    QTest::newRow("elevation provider that the user cannot select")
        << QString::fromLatin1(TerrariumElevationProvider::kProviderKey) << false;
    // A set of an earlier version can name a map that its provider has no more
    QTest::newRow("no map of a provider that has maps") << QStringLiteral("Bing Removed") << false;
    QTest::newRow("no name") << QString() << false;
}

void QGCMapEngineManagerTest::_testIsImageryMap()
{
    QFETCH(QString, mapType);
    QFETCH(bool, imagery);

    QCOMPARE(QGCMapEngineManager::isImageryMap(mapType), imagery);
}

// The settings select a map by provider and type: the providers and their types must lead to every map, and to
// nothing else. The provider of OpenStreetMap, "Street", is also a word of other providers' map names, and the map
// of OpenAIP has no type.
void QGCMapEngineManagerTest::_testProviderAndTypeSelectEveryMap()
{
    QCOMPARE(QGCMapEngineManager::mapTypeList(QStringLiteral("Street")), QStringList{QStringLiteral("Map")});

    QStringList selectableMaps;
    for (const QString& provider : QGCMapEngineManager::mapProviderList()) {
        const QStringList types = QGCMapEngineManager::mapTypeList(provider);
        if (types.isEmpty()) {
            selectableMaps.append(QGCMapEngineManager::mapName(provider, QStringLiteral("type of another provider")));
        }
        for (const QString& type : types) {
            selectableMaps.append(QGCMapEngineManager::mapName(provider, type));
        }
    }

    QStringList maps = QGCMapEngineManager::mapList();
    for (const QString& elevationProvider : UrlFactory::getElevationProviderTypes()) {
        (void) maps.removeAll(elevationProvider);
    }
    selectableMaps.sort();
    maps.sort();
    QCOMPARE(selectableMaps, maps);
}

void QGCMapEngineManagerTest::_testChosenMap_data()
{
    // The settings that the manager is created with
    QTest::addColumn<QString>("savedProvider");
    QTest::addColumn<QString>("savedType");
    // The settings and the chosen map once it is created
    QTest::addColumn<QString>("provider");
    QTest::addColumn<QString>("type");
    QTest::addColumn<QString>("map");
    // What the settings page sets then, the provider first, and the chosen map after each. Empty: it is not set.
    QTest::addColumn<QString>("newProvider");
    QTest::addColumn<QString>("mapOfNewProvider");
    QTest::addColumn<QString>("newType");
    QTest::addColumn<QString>("mapOfNewType");

    const QString bing = QStringLiteral("Bing");
    const QString hybrid = QStringLiteral("Hybrid");
    const QString bingHybrid = QStringLiteral("Bing Hybrid");
    const QString notSet;

    // The provider and the type name no map in between: the map chosen before stays
    QTest::newRow("provider, then type") << bing << hybrid << bing << hybrid << bingHybrid
                                         << QStringLiteral("CustomURL") << bingHybrid << QStringLiteral("Custom")
                                         << QStringLiteral("CustomURL Custom");
    // The map of OpenAIP has no type: its provider selects it, whatever type is left of the provider before
    QTest::newRow("provider without types")
        << bing << hybrid << bing << hybrid << bingHybrid << QStringLiteral("OpenAIP") << QStringLiteral("OpenAIP")
        << notSet << notSet;

    // Saved settings that name no map are repaired. Earlier versions saved this pair for OpenStreetMap: it gets the
    // type of that map.
    QTest::newRow("saved type that its provider lacks")
        << QStringLiteral("Street") << QStringLiteral("Street Map") << QStringLiteral("Street") << QStringLiteral("Map")
        << QStringLiteral("Street Map") << notSet << notSet << notSet << notSet;
    // No type makes a map of this provider: the settings get the default map
    QTest::newRow("saved provider that no map has")
        << QStringLiteral("No Such Provider") << QStringLiteral("Custom") << bing << hybrid << bingHybrid << notSet
        << notSet << notSet << notSet;

    // An elevation provider is in the map list, and is no map that a map engine can show
    QTest::newRow("saved provider that is an elevation provider")
        << QString::fromLatin1(CopernicusElevationProvider::kProviderKey) << hybrid << bing << hybrid << bingHybrid
        << notSet << notSet << notSet << notSet;
    QTest::newRow("elevation provider set as the provider")
        << bing << hybrid << bing << hybrid << bingHybrid
        << QString::fromLatin1(TerrariumElevationProvider::kProviderKey) << bingHybrid << notSet << notSet;
}

void QGCMapEngineManagerTest::_testChosenMap()
{
    QFETCH(QString, savedProvider);
    QFETCH(QString, savedType);
    QFETCH(QString, provider);
    QFETCH(QString, type);
    QFETCH(QString, map);
    QFETCH(QString, newProvider);
    QFETCH(QString, mapOfNewProvider);
    QFETCH(QString, newType);
    QFETCH(QString, mapOfNewType);

    Fact* const providerFact = SettingsManager::instance()->flightMapSettings()->mapProvider();
    Fact* const typeFact = SettingsManager::instance()->flightMapSettings()->mapType();
    const auto restoreSettings = qScopeGuard(
        [providerFact, typeFact, providerBefore = providerFact->rawValue(), typeBefore = typeFact->rawValue()] {
            providerFact->setRawValue(providerBefore);
            typeFact->setRawValue(typeBefore);
        });
    providerFact->setRawValue(savedProvider);
    typeFact->setRawValue(savedType);

    const QGCMapEngineManager manager;
    QCOMPARE(providerFact->rawValue().toString(), provider);
    QCOMPARE(typeFact->rawValue().toString(), type);
    QCOMPARE(manager.chosenMap(), map);

    // The signal tells of a change of the map, and of nothing else
    QSignalSpy chosenMapSpy(&manager, &QGCMapEngineManager::chosenMapChanged);
    if (!newProvider.isEmpty()) {
        providerFact->setRawValue(newProvider);
        QCOMPARE(manager.chosenMap(), mapOfNewProvider);
        QCOMPARE(chosenMapSpy.count(), (mapOfNewProvider == map) ? 0 : 1);
    }
    if (!newType.isEmpty()) {
        chosenMapSpy.clear();
        typeFact->setRawValue(newType);
        QCOMPARE(manager.chosenMap(), mapOfNewType);
        QCOMPARE(chosenMapSpy.count(), (mapOfNewType == mapOfNewProvider) ? 0 : 1);
    }
}

UT_REGISTER_TEST(QGCMapEngineManagerTest, TestLabel::Unit)
