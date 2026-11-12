#include "QGeoTiledMappingManagerEngineQGCTest.h"

#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtLocation/private/qgeomaptype_p.h>
#include <QtLocation/private/qgeotilefetcher_p.h>
#include <QtLocation/private/qgeotilespec_p.h>
#include <QtNetwork/QNetworkAccessManager>
#include <QtTest/QSignalSpy>

#include "AppSettings.h"
#include "Fixtures/RAIIFixtures.h"
#include "QGCMapEngine.h"
#include "QGCMapUrlEngine.h"
#include "QGeoMapIdQGC.h"
#include "QGeoServiceProviderPluginQGC.h"
#include "QGeoTiledMappingManagerEngineQGC.h"
#include "SettingsManager.h"
#include "TileReplyNetworkMock.h"
#include "UnitTestTileGenerator.h"

namespace {

/// The provider that fetches from the URL template the user sets: it has no URL for a tile while that is empty
const QString kMappingEngineCustomUrlType = QStringLiteral("CustomURL Custom");

QGeoTileSpec mappingEngineCustomUrlTileSpec(int x)
{
    const int mapId = QGeoMapIdQGC::fromProviderId(UrlFactory::getMapIdFromProviderType(kMappingEngineCustomUrlType));
    return QGeoTileSpec(QStringLiteral("QGroundControl"), mapId, 13, x, 2867);
}

}  // namespace

void QGeoTiledMappingManagerEngineQGCTest::initTestCase()
{
    UnitTest::initTestCase();
    // _testUncachedTileFailsWithoutProviderUrl requests a tile that the tile cache of the process must not have
    QVERIFY(UnitTestTileGenerator::resetTileCache());
}

void QGeoTiledMappingManagerEngineQGCTest::_testElevationProvidersAreNotMapTypes()
{
    QNetworkAccessManager networkManager;
    QGeoServiceProvider::Error error = QGeoServiceProvider::NotSupportedError;
    QString errorString;
    const QGeoTiledMappingManagerEngineQGC engine(&error, &errorString, &networkManager);
    QCOMPARE(error, QGeoServiceProvider::NoError);

    const QStringList elevationTypes = UrlFactory::getElevationProviderTypes();
    QVERIFY(!elevationTypes.isEmpty());

    // A map showing an elevation provider would request map tiles from it, which it does not have
    const QList<QGeoMapType> mapTypes = engine.supportedMapTypes();
    QCOMPARE(mapTypes.size(), UrlFactory::getProviderTypes().size() - elevationTypes.size());
    for (const QGeoMapType& mapType : mapTypes) {
        QVERIFY2(!elevationTypes.contains(mapType.name()), qPrintable(mapType.name()));
    }
}

void QGeoTiledMappingManagerEngineQGCTest::_testMapTypeIdsAreSequential()
{
    QNetworkAccessManager networkManager;
    QGeoServiceProvider::Error error = QGeoServiceProvider::NotSupportedError;
    QString errorString;
    const QGeoTiledMappingManagerEngineQGC engine(&error, &errorString, &networkManager);
    QCOMPARE(error, QGeoServiceProvider::NoError);

    const QList<QGeoMapType> mapTypes = engine.supportedMapTypes();
    QVERIFY(!mapTypes.isEmpty());
    for (qsizetype i = 0; i < mapTypes.size(); i++) {
        QCOMPARE(mapTypes[i].mapId(), static_cast<int>(i) + 1);
        // A tile QtLocation requests for the map type is fetched from the provider the map type names
        QCOMPARE(UrlFactory::getProviderTypeFromMapId(QGeoMapIdQGC::toProviderId(mapTypes[i].mapId())),
                 mapTypes[i].name());
    }
}

void QGeoTiledMappingManagerEngineQGCTest::_testMapIdTranslationWithGaps()
{
    // The map providers of a build without those of ids 1 to 5 and 8
    const QList<int> providerIds = {6, 7, 9, 39};

    for (qsizetype i = 0; i < providerIds.size(); i++) {
        const int qtMapId = static_cast<int>(i) + 1;
        QCOMPARE(QGeoMapIdQGC::toProviderId(qtMapId, providerIds), providerIds[i]);
        QCOMPARE(QGeoMapIdQGC::fromProviderId(providerIds[i], providerIds), qtMapId);
    }

    QCOMPARE(QGeoMapIdQGC::toProviderId(0, providerIds), -1);
    QCOMPARE(QGeoMapIdQGC::toProviderId(5, providerIds), -1);
    QCOMPARE(QGeoMapIdQGC::fromProviderId(8, providerIds), -1);
}

void QGeoTiledMappingManagerEngineQGCTest::_testSourceChangeGivesTilesNewVersion()
{
    Fact* const customUrl = SettingsManager::instance()->appSettings()->customURL();
    TestFixtures::SettingsFixture saved;
    saved.setFactValue(customUrl, QStringLiteral("https://a.example/{z}/{x}/{y}.png"));

    QNetworkAccessManager networkManager;
    QGeoServiceProvider::Error error = QGeoServiceProvider::NotSupportedError;
    QString errorString;
    QGeoTiledMappingManagerEngineQGC engine(&error, &errorString, &networkManager);
    QCOMPARE(error, QGeoServiceProvider::NoError);
    const int tileVersion = engine.tileVersion();
    QSignalSpy versionSpy(&engine, &QGeoTiledMappingManagerEngine::tileVersionChanged);

    // The maps and QtLocation's tile cache hold tiles of the source that was configured. On a new tile version
    // QtLocation's maps request their tiles again, and drop those of the old version.
    customUrl->setRawValue(QStringLiteral("https://b.example/{z}/{x}/{y}.png"));

    QCOMPARE(versionSpy.count(), 1);
    QCOMPARE_NE(engine.tileVersion(), tileVersion);
}

void QGeoTiledMappingManagerEngineQGCTest::_testFactoryNeedsQmlEngine()
{
    // QtLocation gives the factory the QML engine before it asks for a mapping engine, which fetches its tiles with
    // the network access manager of that engine
    const QGeoServiceProviderFactoryQGC factory;
    QGeoServiceProvider::Error error = QGeoServiceProvider::NoError;
    QString errorString;

    expectLogMessage("QtLocationPlugin.QGeoServiceProviderFactoryQGC", QtWarningMsg,
                     QRegularExpression(QStringLiteral("QML engine")));
    QCOMPARE(factory.createMappingManagerEngine({}, &error, &errorString), nullptr);
    verifyExpectedLogMessage();
    QCOMPARE_NE(error, QGeoServiceProvider::NoError);
    QVERIFY(!errorString.isEmpty());
}

void QGeoTiledMappingManagerEngineQGCTest::_testCachedTileServedWithoutProviderUrl()
{
    Fact* const customUrl = SettingsManager::instance()->appSettings()->customURL();
    TestFixtures::SettingsFixture saved;
    saved.setFactValue(customUrl, QString());

    TileReplyMockNam networkManager;
    QGeoServiceProvider::Error error = QGeoServiceProvider::NotSupportedError;
    QString errorString;
    QGeoTiledMappingManagerEngineQGC engine(&error, &errorString, &networkManager);
    QCOMPARE(error, QGeoServiceProvider::NoError);
    QGeoTileFetcher* const fetcher = engine.tileFetcher();
    QSignalSpy finishedSpy(fetcher, &QGeoTileFetcher::tileFinished);
    QSignalSpy errorSpy(fetcher, &QGeoTileFetcher::tileError);

    const QGeoTileSpec spec = mappingEngineCustomUrlTileSpec(4410);
    const QByteArray tile = tileReplyPngBody();
    QVERIFY(!tile.isEmpty());
    // The cache worker runs tasks in order: the tile is stored before the fetcher asks for it
    getQGCMapEngine()->cacheTile(kMappingEngineCustomUrlType,
                                 UrlFactory::getTileHash(kMappingEngineCustomUrlType, spec.x(), spec.y(), spec.zoom()),
                                 tile, QStringLiteral("png"));

    fetcher->updateTileRequests({spec}, {});

    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(finishedSpy.constFirst().at(0).value<QGeoTileSpec>(), spec);
    QCOMPARE(finishedSpy.constFirst().at(1).toByteArray(), tile);
    QVERIFY(errorSpy.isEmpty());
    QCOMPARE(networkManager.requestCount, 0);
}

void QGeoTiledMappingManagerEngineQGCTest::_testUncachedTileFailsWithoutProviderUrl()
{
    Fact* const customUrl = SettingsManager::instance()->appSettings()->customURL();
    TestFixtures::SettingsFixture saved;
    saved.setFactValue(customUrl, QString());

    // Under unit tests UnitTestTileGenerator serves a tile the cache lacks, unless a miss is forced
    const UnitTestTileGenerator::ForcedMisses forcedMisses(1);

    TileReplyMockNam networkManager;
    QGeoServiceProvider::Error error = QGeoServiceProvider::NotSupportedError;
    QString errorString;
    QGeoTiledMappingManagerEngineQGC engine(&error, &errorString, &networkManager);
    QCOMPARE(error, QGeoServiceProvider::NoError);
    QGeoTileFetcher* const fetcher = engine.tileFetcher();
    QSignalSpy finishedSpy(fetcher, &QGeoTileFetcher::tileFinished);
    QSignalSpy errorSpy(fetcher, &QGeoTileFetcher::tileError);

    const QGeoTileSpec spec = mappingEngineCustomUrlTileSpec(4411);
    expectLogMessage("QtLocationPlugin.QGeoTiledMapReplyQGC", QtWarningMsg,
                     QRegularExpression(QStringLiteral("has no URL")));
    fetcher->updateTileRequests({spec}, {});

    // An error, as QtLocation's maps wait for ever for a tile that ends with neither a tile nor an error
    QVERIFY_SIGNAL_COUNT_WAIT(errorSpy, 1, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    QCOMPARE(errorSpy.constFirst().at(0).value<QGeoTileSpec>(), spec);
    QVERIFY(!errorSpy.constFirst().at(1).toString().isEmpty());
    QVERIFY(finishedSpy.isEmpty());
    QCOMPARE(networkManager.requestCount, 0);
}

UT_REGISTER_TEST(QGeoTiledMappingManagerEngineQGCTest, TestLabel::Unit)
