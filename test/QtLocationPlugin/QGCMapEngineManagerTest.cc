#include "QGCMapEngineManagerTest.h"

#include <utility>

#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtTest/QSignalSpy>

#include "ElevationMapProvider.h"
#include "Fact.h"
#include "FlightMapSettings.h"
#include "FlyViewSettings.h"
#include "PlanViewSettings.h"
#include "QGCCachedTileSet.h"
#include "QGCMapEngine.h"
#include "QGCMapEngineManager.h"
#include "QGCMapTasks.h"
#include "QGCMapUrlEngine.h"
#include "QGCTileReply.h"
#include "QmlObjectListModel.h"
#include "SettingsManager.h"
#include "TileReplyNetworkMock.h"
#include "UnitTestTileGenerator.h"

namespace {

// A region within one tile of the elevation provider and of every map zoom up to 17, whatever its longitude below.
// A set of it has no more tiles than a set downloads in parallel, so it asks for all of them at once.
constexpr double kOfflineRegionTopLat = 47.3906;
constexpr double kOfflineRegionBottomLat = 47.3902;
constexpr double kOfflineRegionLonSpan = 0.0004;

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

}  // namespace

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
    QCOMPARE(terrainTiles.tileCount, terrainExpected ? quint64(terrainMaxZoom - terrainMinZoom + 1) : quint64(0));

    OfflineSetsFixture sets;
    QVERIFY(sets.manager.setProperty("fetchElevation", fetchElevation));
    sets.manager.updateForCurrentView(kLeftLon, kOfflineRegionTopLat, kLeftLon + kOfflineRegionLonSpan,
                                      kOfflineRegionBottomLat, minZoom, maxZoom, mapType);
    QCOMPARE(sets.manager.tileCount(), imageTiles.tileCount + elevationTiles.tileCount + terrainTiles.tileCount);
    QCOMPARE(sets.manager.tileSize(), imageTiles.tileSize + elevationTiles.tileSize + terrainTiles.tileSize);

    const QString name = QStringLiteral("terrain set test: %1").arg(QString::fromLatin1(QTest::currentDataTag()));
    if (!fetchElevation) {
        expectLogMessage("QtLocationPlugin.QGCMapEngineManager", QtWarningMsg,
                         QRegularExpression(QStringLiteral("No Tiles to save")));
    }
    sets.manager.startDownload(name, mapType);
    if (!fetchElevation) {
        verifyExpectedLogMessage();
    }

    // The database worker runs tasks in order, so the sets are all saved once the task queued after them is done
    bool setsSaved = false;
    const QObject sentinelContext;
    QGCFetchTileTask* const sentinelTask = new QGCFetchTileTask(QStringLiteral("no such tile"));
    connect(sentinelTask, &QGCMapTask::error, &sentinelContext, [&setsSaved] { setsSaved = true; });
    QVERIFY(engine->addTask(sentinelTask));
    QTRY_VERIFY_WITH_TIMEOUT(setsSaved, TestTimeout::mediumMs());

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

    QTRY_COMPARE_WITH_TIMEOUT(
        sets.networkManager.requestCount,
        static_cast<int>(imageTiles.tileCount + elevationTiles.tileCount + terrainTiles.tileCount),
        TestTimeout::mediumMs());
}

void QGCMapEngineManagerTest::_testTerrainSetServedFromCache()
{
    ignoreLogMessage("API.QGCApplication.AppMessage", QtDebugMsg,
                     QRegularExpression(QStringLiteral("Restart application for changes to take effect")));
    const auto restoreMapEngines = selectGeoMapEngines(true, false);

    getQGCMapEngine()->ensureInitialized();

    // A region of its own, as its Terrarium tiles end up in the cache
    constexpr double kLeftLon = 9.4989;
    constexpr int kMinZoom = 12;
    constexpr int kMaxZoom = 13;
    constexpr int kTerrainMinZoom = TerrariumElevationProvider::kAnchorZoom;
    constexpr int kTerrainTileCount = kMaxZoom - kTerrainMinZoom + 1;
    constexpr int kRequestCount = (kMaxZoom - kMinZoom + 1) + 1 + kTerrainTileCount;
    const QString mapType = QGCMapEngineManager::mapList().constFirst();
    const QString terrainType = QString::fromLatin1(TerrariumElevationProvider::kProviderKey);

    OfflineSetsFixture sets;
    sets.networkManager.body = UnitTestTileGenerator::syntheticTerrariumTileData(0, 0, 0);
    sets.manager.updateForCurrentView(kLeftLon, kOfflineRegionTopLat, kLeftLon + kOfflineRegionLonSpan,
                                      kOfflineRegionBottomLat, kMinZoom, kMaxZoom, mapType);
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

    // GeoMap asks for these tiles as below. A tile the cache lacks would be asked of the network, which holds its
    // replies, as the forced misses keep the tile generator of the tests from answering for the cache.
    UnitTestTileGenerator::setForcedMissCount(kTerrainTileCount);
    const auto resetForcedMisses = qScopeGuard([] { UnitTestTileGenerator::setForcedMissCount(0); });
    const int terrainMapId = UrlFactory::getQtMapIdFromProviderType(terrainType);
    for (int zoom = kTerrainMinZoom; zoom <= kMaxZoom; zoom++) {
        QGCTileReply reply(&sets.networkManager, terrainMapId, UrlFactory::long2tileX(terrainType, kLeftLon, zoom),
                           UrlFactory::lat2tileY(terrainType, kOfflineRegionTopLat, zoom), zoom);
        QSignalSpy finishedSpy(&reply, &QGCTileReply::finished);
        QVERIFY(reply.start());
        QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 1, TestTimeout::mediumMs());
        QCOMPARE(reply.error(), QGCTileReply::Error::NoError);
        QVERIFY(reply.isCached());
    }
    QCOMPARE(sets.networkManager.requestCount, kRequestCount);
}

UT_REGISTER_TEST(QGCMapEngineManagerTest, TestLabel::Unit)
