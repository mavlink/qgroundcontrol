#include "QGCGeoTiledMappingManagerEngineTest.h"

#include <algorithm>
#include <memory>

#include <QtCore/QBuffer>
#include <QtCore/QDir>
#include <QtCore/QRegularExpression>
#include <QtCore/QSet>
#include <QtCore/QThread>
#include <QtGui/QPainter>
#include <QtLocation/private/qgeocameradata_p.h>
#include <QtLocation/private/qgeomap_p.h>
#include <QtLocation/private/qgeomappingmanager_p.h>
#include <QtLocation/private/qgeomaptype_p.h>
#include <QtLocation/private/qgeotilefetcher_p.h>
#include <QtLocation/private/qgeotilespec_p.h>
#include <QtNetwork/QNetworkReply>
#include <QtPositioning/QGeoCoordinate>
#include <QtTest/QSignalSpy>

#include "AppSettings.h"
#include "Fixtures/RAIIFixtures.h"
#include "MapProvider.h"
#include "MapProviderRegistry.h"
#include "QGCGeoFileTileCache.h"
#include "QGCGeoServiceProviderPlugin.h"
#include "QGCGeoTiledMapReply.h"
#include "QGCGeoTiledMappingManagerEngine.h"
#include "QtLocationMapId.h"
#include "SettingsManager.h"
#include "TileCache.h"
#include "TileReplyNetworkMock.h"
#include "UnitTestTileGenerator.h"

namespace {

/// The provider that fetches from the URL template the user sets: it has no URL for a tile while that is empty
const QString kMappingEngineCustomUrlType = QStringLiteral("CustomURL Custom");

QGeoTileSpec mappingEngineCustomUrlTileSpec(int x)
{
    const int mapId =
        QtLocationMapId::fromProviderId(MapProviderRegistry::getMapIdFromProviderType(kMappingEngineCustomUrlType));
    return QGeoTileSpec(QStringLiteral("QGroundControl"), mapId, 13, x, 2867);
}

/// Records the tiles that the maps of the engine ask for, and fetches none
class MappingEngineRequestRecorder : public QGCGeoTiledMappingManagerEngine
{
public:
    using QGCGeoTiledMappingManagerEngine::QGCGeoTiledMappingManagerEngine;
    using QGCGeoTiledMappingManagerEngine::setPrefetchStyle;

    void updateTileRequests(QGeoTiledMap* map, const QSet<QGeoTileSpec>& tilesAdded,
                            const QSet<QGeoTileSpec>& tilesRemoved) final
    {
        Q_UNUSED(map);
        Q_UNUSED(tilesRemoved);
        requested += tilesAdded;
    }

    QSet<QGeoTileSpec> requested;
};

/// Sets the view of a map, as the Map item does
struct MappingEngineMapView : QGeoMap
{
    using QGeoMap::setCameraData;
};

/// The tile of OpenStreetMap that a request of \a networkManager asks for, as "zoom/x/y"
QStringList mappingEngineRequestedTiles(const TileReplyMockNam& networkManager)
{
    QStringList tiles;
    for (const QNetworkRequest& request : networkManager.requests) {
        tiles.append(request.url().path().sliced(1).chopped(4));
    }
    return tiles;
}

/// Gives a test what the mapping engine has of its tile cache
class MappingEngineTileCacheProbe : public QGCGeoFileTileCache
{
public:
    using QGCGeoFileTileCache::init;
    using QGeoFileTileCache::directory;

    /// As QtLocation does when the textures of other tiles need the room
    void evictTextures() { textureCache_.clear(); }
};

}  // namespace

void QGCGeoTiledMappingManagerEngineTest::initTestCase()
{
    UnitTest::initTestCase();
    // The tests request tiles that the tile cache of the process must not have yet
    QVERIFY(UnitTestTileGenerator::resetTileCache());
}

void QGCGeoTiledMappingManagerEngineTest::_testElevationProvidersAreNotMapTypes()
{
    QGeoServiceProvider::Error error = QGeoServiceProvider::NotSupportedError;
    QString errorString;
    const QGCGeoTiledMappingManagerEngine engine(&error, &errorString);
    QCOMPARE(error, QGeoServiceProvider::NoError);

    const QStringList elevationTypes = MapProviderRegistry::elevationProviderTypes();
    QVERIFY(!elevationTypes.isEmpty());

    // A map showing an elevation provider would request map tiles from it, which it does not have
    const QList<QGeoMapType> mapTypes = engine.supportedMapTypes();
    QCOMPARE(mapTypes.size(), MapProviderRegistry::providerTypes().size() - elevationTypes.size());
    for (const QGeoMapType& mapType : mapTypes) {
        QVERIFY2(!elevationTypes.contains(mapType.name()), qPrintable(mapType.name()));
    }
}

void QGCGeoTiledMappingManagerEngineTest::_testNightMapTypeIsFlagged()
{
    QGeoServiceProvider::Error error = QGeoServiceProvider::NotSupportedError;
    QString errorString;
    const QGCGeoTiledMappingManagerEngine engine(&error, &errorString);
    QCOMPARE(error, QGeoServiceProvider::NoError);

    // What QML reads as the night property of a map type
    QStringList nightMaps;
    const QList<QGeoMapType> mapTypes = engine.supportedMapTypes();
    for (const QGeoMapType& mapType : mapTypes) {
        if (mapType.night()) {
            nightMaps.append(mapType.name());
        }
    }
    QCOMPARE(nightMaps, QStringList{QStringLiteral("Mapbox Dark")});
}

void QGCGeoTiledMappingManagerEngineTest::_testMapTypeIdsAreSequential()
{
    QGeoServiceProvider::Error error = QGeoServiceProvider::NotSupportedError;
    QString errorString;
    const QGCGeoTiledMappingManagerEngine engine(&error, &errorString);
    QCOMPARE(error, QGeoServiceProvider::NoError);

    const QList<QGeoMapType> mapTypes = engine.supportedMapTypes();
    QVERIFY(!mapTypes.isEmpty());
    for (qsizetype i = 0; i < mapTypes.size(); i++) {
        QCOMPARE(mapTypes[i].mapId(), static_cast<int>(i) + 1);
        // A tile QtLocation requests for the map type is fetched from the provider the map type names
        QCOMPARE(MapProviderRegistry::getProviderTypeFromMapId(QtLocationMapId::toProviderId(mapTypes[i].mapId())),
                 mapTypes[i].name());
    }
}

void QGCGeoTiledMappingManagerEngineTest::_testMapIdTranslationWithGaps()
{
    // The map providers of a build without those of ids 1 to 5 and 8
    const QList<int> providerIds = {6, 7, 9, 39};

    for (qsizetype i = 0; i < providerIds.size(); i++) {
        const int qtMapId = static_cast<int>(i) + 1;
        QCOMPARE(QtLocationMapId::toProviderId(qtMapId, providerIds), providerIds[i]);
        QCOMPARE(QtLocationMapId::fromProviderId(providerIds[i], providerIds), qtMapId);
    }

    QCOMPARE(QtLocationMapId::toProviderId(0, providerIds), -1);
    QCOMPARE(QtLocationMapId::toProviderId(5, providerIds), -1);
    QCOMPARE(QtLocationMapId::fromProviderId(8, providerIds), -1);
}

void QGCGeoTiledMappingManagerEngineTest::_testSourceChangeGivesTilesNewVersion()
{
    Fact* const customUrl = SettingsManager::instance()->appSettings()->customURL();
    TestFixtures::SettingsFixture saved;
    saved.setFactValue(customUrl, QStringLiteral("https://a.example/{z}/{x}/{y}.png"));

    QGeoServiceProvider::Error error = QGeoServiceProvider::NotSupportedError;
    QString errorString;
    QGCGeoTiledMappingManagerEngine engine(&error, &errorString);
    QCOMPARE(error, QGeoServiceProvider::NoError);
    const int tileVersion = engine.tileVersion();
    QSignalSpy versionSpy(&engine, &QGeoTiledMappingManagerEngine::tileVersionChanged);

    // The maps and QtLocation's tile cache hold tiles of the source that was configured. On a new tile version
    // QtLocation's maps request their tiles again, and drop those of the old version.
    customUrl->setRawValue(QStringLiteral("https://b.example/{z}/{x}/{y}.png"));

    QCOMPARE(versionSpy.count(), 1);
    QCOMPARE_NE(engine.tileVersion(), tileVersion);
}

void QGCGeoTiledMappingManagerEngineTest::_testPluginLoadsThroughQtLocation()
{
    // As the Plugin of a QML map does: by the provider name of qgc_maps_plugin.json
    const QString providerName = QStringLiteral("QGroundControl");
    QVERIFY(QGeoServiceProvider::availableServiceProviders().contains(providerName));
    const QGeoServiceProvider provider(providerName);
    const QGeoMappingManager* const mappingManager = provider.mappingManager();
    QCOMPARE(provider.mappingError(), QGeoServiceProvider::NoError);
    QVERIFY2(mappingManager, qPrintable(provider.mappingErrorString()));
    QCOMPARE(mappingManager->managerName(), providerName);

    // A QML map takes a map type as its active one only when the type names the plugin of the map
    const QList<QGeoMapType> mapTypes = mappingManager->supportedMapTypes();
    QVERIFY(!mapTypes.isEmpty());
    for (const QGeoMapType& mapType : mapTypes) {
        QCOMPARE(mapType.pluginName(), providerName.toLatin1());
    }
}

void QGCGeoTiledMappingManagerEngineTest::_testFactoryNeedsMainThread()
{
    const QGCGeoServiceProviderFactory factory;
    QGeoServiceProvider::Error error = QGeoServiceProvider::NoError;
    QString errorString;

    // The mapping engine fetches its tiles on the main thread
    QGeoMappingManagerEngine* offThreadEngine = nullptr;
    expectLogMessage("QtLocationPlugin.QGCGeoServiceProviderFactory", QtCriticalMsg,
                     QRegularExpression(QStringLiteral("main thread")));
    const std::unique_ptr<QThread> thread(
        QThread::create([&] { offThreadEngine = factory.createMappingManagerEngine({}, &error, &errorString); }));
    thread->start();
    QVERIFY(thread->wait(TestTimeout::mediumMs()));
    verifyExpectedLogMessage();
    QCOMPARE(offThreadEngine, nullptr);
    QCOMPARE(error, QGeoServiceProvider::LoaderError);
    QVERIFY(!errorString.isEmpty());

    const std::unique_ptr<QGeoMappingManagerEngine> engine(
        factory.createMappingManagerEngine({}, &error, &errorString));
    QVERIFY(engine);
    QCOMPARE(error, QGeoServiceProvider::NoError);
}

void QGCGeoTiledMappingManagerEngineTest::_testCachedTileServedWithoutProviderUrl()
{
    Fact* const customUrl = SettingsManager::instance()->appSettings()->customURL();
    TestFixtures::SettingsFixture saved;
    saved.setFactValue(customUrl, QString());

    TileReplyMockNam networkManager;
    const TileReplyNetworkScope networkScope(&networkManager);
    QGeoServiceProvider::Error error = QGeoServiceProvider::NotSupportedError;
    QString errorString;
    QGCGeoTiledMappingManagerEngine engine(&error, &errorString);
    QCOMPARE(error, QGeoServiceProvider::NoError);
    QGeoTileFetcher* const fetcher = engine.tileFetcher();
    QSignalSpy finishedSpy(fetcher, &QGeoTileFetcher::tileFinished);
    QSignalSpy errorSpy(fetcher, &QGeoTileFetcher::tileError);

    const QGeoTileSpec spec = mappingEngineCustomUrlTileSpec(4410);
    const QByteArray tile = tileReplyPngBody();
    QVERIFY(!tile.isEmpty());
    // The cache worker runs tasks in order: the tile is stored before the fetcher asks for it
    const SharedMapProvider provider = MapProviderRegistry::getMapProviderFromProviderType(kMappingEngineCustomUrlType);
    TileCache::instance()->cacheTile(provider->mapId(), provider->getTileHash(spec.x(), spec.y(), spec.zoom()), tile,
                                     QStringLiteral("png"));

    fetcher->updateTileRequests({spec}, {});

    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(finishedSpy.constFirst().at(0).value<QGeoTileSpec>(), spec);
    QCOMPARE(finishedSpy.constFirst().at(1).toByteArray(), tile);
    QVERIFY(errorSpy.isEmpty());
    QCOMPARE(networkManager.requestCount, 0);
}

void QGCGeoTiledMappingManagerEngineTest::_testTileThatCannotBeAskedForEndsWithoutTile_data()
{
    QTest::addColumn<bool>("customUrlMap");
    QTest::addColumn<QString>("warning");

    // The map of the URL that the user sets has no URL for a tile while that is empty
    QTest::newRow("tile of a map without a URL") << true << QStringLiteral("has no URL");
    QTest::newRow("tile of no map type") << false << QStringLiteral("no map provider");
}

void QGCGeoTiledMappingManagerEngineTest::_testTileThatCannotBeAskedForEndsWithoutTile()
{
    QFETCH(bool, customUrlMap);
    QFETCH(QString, warning);

    Fact* const customUrl = SettingsManager::instance()->appSettings()->customURL();
    TestFixtures::SettingsFixture saved;
    saved.setFactValue(customUrl, QString());

    // Under unit tests UnitTestTileGenerator serves a tile the cache lacks, unless a miss is forced
    const UnitTestTileGenerator::ForcedMisses forcedMisses(1);

    TileReplyMockNam networkManager;
    const TileReplyNetworkScope networkScope(&networkManager);
    QGeoServiceProvider::Error error = QGeoServiceProvider::NotSupportedError;
    QString errorString;
    QGCGeoTiledMappingManagerEngine engine(&error, &errorString);
    QCOMPARE(error, QGeoServiceProvider::NoError);
    QGeoTileFetcher* const fetcher = engine.tileFetcher();
    QSignalSpy finishedSpy(fetcher, &QGeoTileFetcher::tileFinished);
    QSignalSpy errorSpy(fetcher, &QGeoTileFetcher::tileError);

    const int noMapTypeId = static_cast<int>(engine.supportedMapTypes().size()) + 1;
    const QGeoTileSpec spec = customUrlMap
                                  ? mappingEngineCustomUrlTileSpec(4411)
                                  : QGeoTileSpec(QStringLiteral("QGroundControl"), noMapTypeId, 13, 4412, 2867);
    expectLogMessage("QtLocationPlugin.QGCGeoTiledMapReply", QtWarningMsg, QRegularExpression(warning));
    fetcher->updateTileRequests({spec}, {});

    // A request that ends, which a map does not wait for ever for: without a tile, and without an error, for which
    // QtLocation asks five more times
    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 1, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    QCOMPARE(finishedSpy.constFirst().at(0).value<QGeoTileSpec>(), spec);
    QVERIFY(finishedSpy.constFirst().at(1).toByteArray().isEmpty());
    QVERIFY(errorSpy.isEmpty());
    QCOMPARE(networkManager.requestCount, 0);
}

void QGCGeoTiledMappingManagerEngineTest::_testTileWithoutPictureIsNotAskedForAgain_data()
{
    QTest::addColumn<QString>("providerType");
    QTest::addColumn<int>("x");
    QTest::addColumn<QNetworkReply::NetworkError>("networkError");
    QTest::addColumn<int>("httpStatus");
    QTest::addColumn<int>("askedTiles");
    QTest::addColumn<QString>("warning");

    // The server of a provider with a fixed source has no tile here: it is asked for the tile, and for the tiles
    // above it that could be shown in its place
    QTest::newRow("tile that the provider lacks")
        << QStringLiteral("Bing Hybrid") << 4413 << QNetworkReply::ContentNotFoundError << 404
        << (1 + QGCGeoTiledMapReply::kMaxAncestorLevels) << QString();
    // As for a key of the provider that is not valid
    QTest::newRow("tile that the server refuses")
        << QStringLiteral("Street Map") << 6449 << QNetworkReply::ContentAccessDenied << 403 << 1
        << QStringLiteral("http 403");
}

// A map asks for a tile again whenever its view changes: the maps of the engine share what is known of the tiles
// that have no picture, so that the server is not asked again, or not again soon
void QGCGeoTiledMappingManagerEngineTest::_testTileWithoutPictureIsNotAskedForAgain()
{
    QFETCH(QString, providerType);
    QFETCH(int, x);
    QFETCH(QNetworkReply::NetworkError, networkError);
    QFETCH(int, httpStatus);
    QFETCH(int, askedTiles);
    QFETCH(QString, warning);

    const UnitTestTileGenerator::ForcedMisses forcedMisses(UnitTestTileGenerator::ForcedMisses::kEveryLookup);

    TileReplyMockNam networkManager;
    const TileReplyNetworkScope networkScope(&networkManager);
    networkManager.error = networkError;
    networkManager.httpStatus = httpStatus;
    QGeoServiceProvider::Error error = QGeoServiceProvider::NotSupportedError;
    QString errorString;
    QGCGeoTiledMappingManagerEngine engine(&error, &errorString);
    QCOMPARE(error, QGeoServiceProvider::NoError);
    QGeoTileFetcher* const fetcher = engine.tileFetcher();
    QSignalSpy finishedSpy(fetcher, &QGeoTileFetcher::tileFinished);
    const QSignalSpy errorSpy(fetcher, &QGeoTileFetcher::tileError);

    const int mapId = QtLocationMapId::fromProviderId(MapProviderRegistry::getMapIdFromProviderType(providerType));
    const QGeoTileSpec spec(QStringLiteral("QGroundControl"), mapId, 13, x, 2867, engine.tileVersion());

    // No error, for which QtLocation asks five more times
    if (!warning.isEmpty()) {
        expectLogMessage("QtLocationPlugin.QGCGeoTiledMapReply", QtWarningMsg, QRegularExpression(warning));
    }
    fetcher->updateTileRequests({spec}, {});
    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 1, TestTimeout::mediumMs());
    if (!warning.isEmpty()) {
        verifyExpectedLogMessage();
    }
    QCOMPARE(finishedSpy.constFirst().at(0).value<QGeoTileSpec>(), spec);
    QVERIFY(finishedSpy.constFirst().at(1).toByteArray().isEmpty());
    QCOMPARE(networkManager.requestCount, askedTiles);

    fetcher->updateTileRequests({spec}, {});
    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 2, TestTimeout::mediumMs());
    QVERIFY(finishedSpy.constLast().at(1).toByteArray().isEmpty());
    QCOMPARE(networkManager.requestCount, askedTiles);
    QVERIFY(errorSpy.isEmpty());
}

void QGCGeoTiledMappingManagerEngineTest::_testTileBelowDeepestZoomShownFromAbove()
{
    const UnitTestTileGenerator::ForcedMisses forcedMisses(UnitTestTileGenerator::ForcedMisses::kEveryLookup);

    TileReplyMockNam networkManager;
    const TileReplyNetworkScope networkScope(&networkManager);
    networkManager.body = tileReplyPngBody(Qt::darkCyan);
    QGeoServiceProvider::Error error = QGeoServiceProvider::NotSupportedError;
    QString errorString;
    QGCGeoTiledMappingManagerEngine engine(&error, &errorString);
    QCOMPARE(error, QGeoServiceProvider::NoError);
    QGeoTileFetcher* const fetcher = engine.tileFetcher();
    QSignalSpy finishedSpy(fetcher, &QGeoTileFetcher::tileFinished);
    const QSignalSpy errorSpy(fetcher, &QGeoTileFetcher::tileError);

    // A tile one zoom level below the deepest ones of OpenStreetMap
    const SharedMapProvider provider = MapProviderRegistry::findProvider(QStringLiteral("Street Map"));
    QVERIFY(provider);
    const QGeoTileSpec spec(QStringLiteral("QGroundControl"), QtLocationMapId::fromProviderId(provider->mapId()),
                            provider->maxZoom() + 1, 600002, 400002, engine.tileVersion());

    // The tile above is fetched, and given to the tile cache of the engine: no bytes to store, nor to decode
    fetcher->updateTileRequests({spec}, {});
    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(finishedSpy.constFirst().at(0).value<QGeoTileSpec>(), spec);
    QVERIFY(finishedSpy.constFirst().at(1).toByteArray().isEmpty());
    QVERIFY(errorSpy.isEmpty());
    QCOMPARE(networkManager.requestCount, 1);

    // QtLocation looks the tile up once it has finished: a texture of the tile itself, which a map shows whole
    const QSharedPointer<QGeoTileTexture> texture = engine.tileCache()->get(spec);
    QVERIFY(texture);
    QCOMPARE(texture->spec, spec);
    QCOMPARE(texture->image.pixelColor(texture->image.width() / 2, texture->image.height() / 2), QColor(Qt::darkCyan));
}

void QGCGeoTiledMappingManagerEngineTest::_testMapCreditsProviderOfItsMapType()
{
    QGeoServiceProvider::Error error = QGeoServiceProvider::NotSupportedError;
    QString errorString;
    QGCGeoTiledMappingManagerEngine engine(&error, &errorString);
    QCOMPARE(error, QGeoServiceProvider::NoError);

    // Esri lists the sources of a map by area: the list of this server has one source, of the whole world
    TileReplyMockNam listServer;
    listServer.holdReplies = true;
    listServer.body = QByteArrayLiteral(R"({"contributors":[{"attribution":"Survey & Sons","coverageAreas":)"
                                        R"([{"zoomMax":23,"zoomMin":0,"score":50,"bbox":[-85,-180,85,180]}]}]})");
    const TileReplyNetworkScope networkScope(&listServer);

    const QList<QGeoMapType> mapTypes = engine.supportedMapTypes();
    const auto mapType = [&mapTypes](const QString& name) {
        const auto found = std::find_if(mapTypes.cbegin(), mapTypes.cend(),
                                        [&name](const QGeoMapType& type) { return type.name() == name; });
        return (found != mapTypes.cend()) ? *found : QGeoMapType();
    };

    const std::unique_ptr<QGeoMap> map(engine.createMap());
    map->setViewportSize(QSize(512, 512));
    QSignalSpy copyrightsSpy(map.get(), &QGeoMap::copyrightsChanged);

    // The copyright notice of a Map item shows the HTML that the map tells it
    map->setActiveMapType(mapType(QStringLiteral("Street Map")));
    QCOMPARE(copyrightsSpy.count(), 1);
    QCOMPARE(copyrightsSpy.constLast().at(0).toString(), QStringLiteral("© OpenStreetMap contributors"));

    // A source that the user sets has no credit: the notice hides for no HTML
    map->setActiveMapType(mapType(kMappingEngineCustomUrlType));
    QCOMPARE(copyrightsSpy.count(), 2);
    QVERIFY(copyrightsSpy.constLast().at(0).toString().isEmpty());

    // The sources of what the view shows follow the credit of the provider once their list has arrived
    map->setActiveMapType(mapType(QStringLiteral("Esri World Satellite")));
    QCOMPARE(copyrightsSpy.count(), 3);
    QCOMPARE(copyrightsSpy.constLast().at(0).toString(), QStringLiteral("Powered by Esri"));
    QCOMPARE(listServer.replies.size(), 1);
    QVERIFY(listServer.replies.constFirst());
    listServer.replies.constFirst()->finishNow();
    QCOMPARE(copyrightsSpy.count(), 4);
    QCOMPARE(copyrightsSpy.constLast().at(0).toString(), QStringLiteral("Powered by Esri, Survey &amp; Sons"));

    // A notice that is shown later is told the same
    map->setCopyrightVisible(false);
    map->setCopyrightVisible(true);
    QCOMPARE(copyrightsSpy.count(), 5);
    QCOMPARE(copyrightsSpy.constLast().at(0).toString(), QStringLiteral("Powered by Esri, Survey &amp; Sons"));
}

// The sources of a map are listed by areas with north up. A turned view shows another such area than its size tells:
// a wide one that looks east has more to its north and south than to its east and west.
void QGCGeoTiledMappingManagerEngineTest::_testMapCreditsAreaOfTurnedView()
{
    QGeoServiceProvider::Error error = QGeoServiceProvider::NotSupportedError;
    QString errorString;
    QGCGeoTiledMappingManagerEngine engine(&error, &errorString);
    QCOMPARE(error, QGeoServiceProvider::NoError);

    // One source north of the equator and one east of the prime meridian, as [south, west, north, east]
    TileReplyMockNam listServer;
    listServer.holdReplies = true;
    listServer.body = QByteArrayLiteral(
        R"({"contributors":[)"
        R"({"attribution":"North","coverageAreas":[{"zoomMax":23,"zoomMin":0,"score":50,"bbox":[20,-1,30,1]}]},)"
        R"({"attribution":"East","coverageAreas":[{"zoomMax":23,"zoomMin":0,"score":50,"bbox":[-1,30,1,40]}]}]})");
    const TileReplyNetworkScope networkScope(&listServer);

    // A map whose list no other test here has asked for: the lists that arrived are kept
    const QList<QGeoMapType> mapTypes = engine.supportedMapTypes();
    const auto esri = std::find_if(mapTypes.cbegin(), mapTypes.cend(), [](const QGeoMapType& type) {
        return type.name() == QStringLiteral("Esri World Street");
    });
    QVERIFY(esri != mapTypes.cend());

    // At zoom level 4 the world is 4096 pixels wide: the view has 45 degrees to each side, and less than 3 above
    // and below
    const std::unique_ptr<QGeoMap> map(engine.createMap());
    map->setViewportSize(QSize(1024, 64));
    QGeoCameraData camera = map->cameraData();
    camera.setCenter(QGeoCoordinate(0., 0.));
    camera.setZoomLevel(4.);
    (map.get()->*(&MappingEngineMapView::setCameraData))(camera);
    QSignalSpy copyrightsSpy(map.get(), &QGeoMap::copyrightsChanged);
    map->setActiveMapType(*esri);
    QCOMPARE(listServer.replies.size(), 1);
    QVERIFY(listServer.replies.constFirst());
    listServer.replies.constFirst()->finishNow();
    QCOMPARE(copyrightsSpy.constLast().at(0).toString(), QStringLiteral("Powered by Esri, East"));

    // Turned to the east, the view has these 45 degrees above and below
    camera.setBearing(90.);
    (map.get()->*(&MappingEngineMapView::setCameraData))(camera);
    QCOMPARE(copyrightsSpy.constLast().at(0).toString(), QStringLiteral("Powered by Esri, North"));
}

void QGCGeoTiledMappingManagerEngineTest::_testPrefetchStyleReachesExistingMaps()
{
    QGeoServiceProvider::Error error = QGeoServiceProvider::NotSupportedError;
    QString errorString;
    MappingEngineRequestRecorder engine(&error, &errorString);
    QCOMPARE(error, QGeoServiceProvider::NoError);

    // The engine starts with the style for the connection of the machine
    engine.setPrefetchStyle(QGeoTiledMap::NoPrefetching);
    const std::unique_ptr<QGeoMap> map(engine.createMap());
    map->setViewportSize(QSize(512, 512));
    QVERIFY(!engine.requested.isEmpty());
    const int viewZoom = engine.requested.constBegin()->zoom();
    engine.requested.clear();

    map->prefetchData();
    QVERIFY(engine.requested.isEmpty());

    // As when the connection to the internet comes back: the map that exists prefetches the next zoom level
    engine.setPrefetchStyle(QGeoTiledMap::PrefetchTwoNeighbourLayers);
    map->prefetchData();
    QVERIFY(std::any_of(engine.requested.cbegin(), engine.requested.cend(),
                        [viewZoom](const QGeoTileSpec& spec) { return spec.zoom() == viewZoom + 1; }));
}

void QGCGeoTiledMappingManagerEngineTest::_testTilesOfViewAskedForNearestCentreFirst()
{
    const UnitTestTileGenerator::ForcedMisses forcedMisses(UnitTestTileGenerator::ForcedMisses::kEveryLookup);

    TileReplyMockNam networkManager;
    const TileReplyNetworkScope networkScope(&networkManager);
    networkManager.body = tileReplyPngBody();
    QGeoServiceProvider::Error error = QGeoServiceProvider::NotSupportedError;
    QString errorString;
    QGCGeoTiledMappingManagerEngine engine(&error, &errorString);
    QCOMPARE(error, QGeoServiceProvider::NoError);
    const QList<QGeoMapType> mapTypes = engine.supportedMapTypes();
    const auto streetMap = std::find_if(mapTypes.cbegin(), mapTypes.cend(), [](const QGeoMapType& type) {
        return type.name() == QStringLiteral("Street Map");
    });
    QVERIFY(streetMap != mapTypes.cend());

    // A view of five tiles by five, centred on the middle of a tile
    constexpr int kZoom = 13;
    constexpr int kX = 4430;
    constexpr int kY = 2867;
    const double tilesPerEdge = 1 << kZoom;
    const QPointF centre((kX + 0.5) / tilesPerEdge, (kY + 0.5) / tilesPerEdge);
    QGeoCameraData camera;
    camera.setZoomLevel(kZoom);
    camera.setCenter(QGeoCoordinate(qRadiansToDegrees(std::atan(std::sinh(M_PI * (1.0 - (2.0 * centre.y()))))),
                                    (centre.x() * 360.0) - 180.0));
    const auto showView = [&](QGeoMap& map) {
        map.setActiveMapType(*streetMap);
        (map.*(&MappingEngineMapView::setCameraData))(camera);
        map.setViewportSize(QSize(5 * 256, 5 * 256));
    };
    // How far from the centre of the view a tile that the server was asked for is, in tiles of the zoom level of the
    // view
    const auto distance = [&centre, tilesPerEdge](const QString& tile) {
        const QStringList zxy = tile.split(QLatin1Char('/'));
        const double scale = tilesPerEdge / (1 << zxy.at(0).toInt());
        return std::hypot(((zxy.at(1).toInt() + 0.5) * scale) - (centre.x() * tilesPerEdge),
                          ((zxy.at(2).toInt() + 0.5) * scale) - (centre.y() * tilesPerEdge));
    };

    // Until the fetcher has asked for all the tiles that it was given: it asks for one at each turn of the event loop
    const auto waitForFetcher = [&networkManager] {
        for (int asked = -1; asked != networkManager.requestCount;) {
            asked = networkManager.requestCount;
            QCoreApplication::processEvents();
            if (!UnitTestTileGenerator::waitForCacheTasks()) {
                return false;
            }
        }
        return true;
    };

    // The nearest tiles to the centre first: those that the user looks at
    const std::unique_ptr<QGeoMap> map(engine.createMap());
    showView(*map);
    constexpr int kViewTiles = 7 * 7;  // With those that the edges of the view touch
    QTRY_COMPARE_WITH_TIMEOUT(networkManager.requestCount, kViewTiles, TestTimeout::mediumMs());
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());
    QCOMPARE(networkManager.requestCount, kViewTiles);
    const QStringList viewTiles = mappingEngineRequestedTiles(networkManager);
    QCOMPARE(viewTiles.constFirst(), QStringLiteral("%1/%2/%3").arg(kZoom).arg(kX).arg(kY));
    for (qsizetype i = 1; i < viewTiles.size(); i++) {
        QVERIFY2(distance(viewTiles.at(i - 1)) <= distance(viewTiles.at(i)), qPrintable(viewTiles.join(u' ')));
    }

    // Another map of the same view asks for none of them again
    const std::unique_ptr<QGeoMap> otherMap(engine.createMap());
    showView(*otherMap);
    QVERIFY(waitForFetcher());
    QCOMPARE(networkManager.requestCount, viewTiles.size());

    // The tiles that the map prefetches: those of the zoom level of the view before those of the levels beside it
    auto* const tiledMap = qobject_cast<QGeoTiledMap*>(map.get());
    QVERIFY(tiledMap);
    tiledMap->setPrefetchStyle(QGeoTiledMap::PrefetchTwoNeighbourLayers);
    tiledMap->prefetchData();
    QTRY_VERIFY_WITH_TIMEOUT(networkManager.requestCount > viewTiles.size(), TestTimeout::mediumMs());
    QVERIFY(waitForFetcher());
    const QStringList prefetched = mappingEngineRequestedTiles(networkManager).sliced(viewTiles.size());
    const auto firstOtherLevel = std::find_if(prefetched.cbegin(), prefetched.cend(), [](const QString& tile) {
        return !tile.startsWith(QStringLiteral("%1/").arg(kZoom));
    });
    QVERIFY2((firstOtherLevel != prefetched.cbegin()) && (firstOtherLevel != prefetched.cend()),
             qPrintable(prefetched.join(u' ')));
    QVERIFY2(std::none_of(firstOtherLevel, prefetched.cend(),
                          [](const QString& tile) { return tile.startsWith(QStringLiteral("%1/").arg(kZoom)); }),
             qPrintable(prefetched.join(u' ')));
}

void QGCGeoTiledMappingManagerEngineTest::_testTileCacheStaysInMemory()
{
    MappingEngineTileCacheProbe cache;
    // QtLocation's own cache creates its directory, lists it, and deletes tiles from it. This one is of the tile
    // cache of the test run.
    const QString directory = cache.directory();
    QVERIFY(!TileCache::cachePath().isEmpty());
    QVERIFY2(directory.startsWith(TileCache::cachePath()), qPrintable(directory));
    QVERIFY(QDir(directory).removeRecursively());

    cache.init();
    QVERIFY(!QDir(directory).exists());

    const QGeoTileSpec spec(QStringLiteral("QGroundControl"), 1, 13, 4420, 2867);
    cache.insert(spec, tileReplyPngBody(), QStringLiteral("png"), QAbstractGeoTileCache::MemoryCache);
    const QSharedPointer<QGeoTileTexture> texture = cache.get(spec);
    QVERIFY(texture);
    QVERIFY(!texture->image.isNull());

    cache.clearAll();
    QVERIFY(!cache.get(spec));
    QVERIFY(!QDir(directory).exists());
}

void QGCGeoTiledMappingManagerEngineTest::_testTileCacheReportsBadTile()
{
    MappingEngineTileCacheProbe cache;
    cache.init();

    const QGeoTileSpec spec(QStringLiteral("QGroundControl"), 1, 13, 4421, 2867);
    cache.insert(spec, QByteArrayLiteral("not a picture"), QStringLiteral("png"), QAbstractGeoTileCache::MemoryCache);

    // In the log category of the cache, with the tile: QtLocation's own cache names neither
    expectLogMessage("QtLocationPlugin.QGCGeoFileTileCache", QtWarningMsg,
                     QRegularExpression(QStringLiteral("tile 4421 2867 of zoom level 13")));
    QVERIFY(!cache.get(spec));
    verifyExpectedLogMessage();
}

void QGCGeoTiledMappingManagerEngineTest::_testTileFromAboveMadeAgainAfterEviction()
{
    MappingEngineTileCacheProbe cache;
    cache.init();

    // A tile above, red over its left half and blue over its right one, of which the right tile below is shown
    QImage abovePicture(64, 64, QImage::Format_RGB32);
    abovePicture.fill(Qt::red);
    QPainter painter(&abovePicture);
    painter.fillRect(32, 0, 32, 64, Qt::blue);
    painter.end();
    QByteArray aboveData;
    QBuffer buffer(&aboveData);
    QVERIFY(buffer.open(QIODevice::WriteOnly));
    QVERIFY(abovePicture.save(&buffer, "PNG"));
    const QGeoTileSpec above(QStringLiteral("QGroundControl"), 1, 13, 4422, 2867);
    const QGeoTileSpec right(QStringLiteral("QGroundControl"), 1, 14, (2 * 4422) + 1, 2 * 2867);
    cache.insertFromAbove(right, above, abovePicture, aboveData, QStringLiteral("png"));

    const QSharedPointer<QGeoTileTexture> shown = cache.get(right);
    QVERIFY(shown);
    QCOMPARE(shown->spec, right);
    QCOMPARE(shown->image.size(), abovePicture.size());
    QCOMPARE(shown->image.pixelColor(32, 32), QColor(Qt::blue));

    // The tile has no bytes of its own to decode again: it is made again from those of the tile above
    cache.evictTextures();
    const QSharedPointer<QGeoTileTexture> again = cache.get(right);
    QVERIFY(again);
    QCOMPARE(again->image, shown->image);

    // A tile above given without its bytes is gone with its texture: the map asks for the tile again
    const QGeoTileSpec otherAbove(QStringLiteral("QGroundControl"), 1, 13, 4423, 2867);
    const QGeoTileSpec otherLeft(QStringLiteral("QGroundControl"), 1, 14, 2 * 4423, 2 * 2867);
    cache.insertFromAbove(otherLeft, otherAbove, abovePicture, QByteArray(), QString());
    QVERIFY(cache.get(otherLeft));
    cache.evictTextures();
    QVERIFY(!cache.get(otherLeft));

    cache.clearAll();
    QVERIFY(!cache.get(right));
}

UT_REGISTER_TEST(QGCGeoTiledMappingManagerEngineTest, TestLabel::Unit)
