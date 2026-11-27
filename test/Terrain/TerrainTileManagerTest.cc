#include "TerrainTileManagerTest.h"

#include <QtCore/QRegularExpression>
#include <QtPositioning/QGeoCoordinate>
#include <QtTest/QSignalSpy>

#include "Fact.h"
#include "Fixtures/RAIIFixtures.h"
#include "FlightMapSettings.h"
#include "MapProvider.h"
#include "MapProviderRegistry.h"
#include "SettingsManager.h"
#include "TerrainQueryInterface.h"
#include "TerrainTileManager.h"
#include "TileCache.h"
#include "TileReplyNetworkMock.h"
#include "UnitTestTileGenerator.h"

void TerrainTileManagerTest::initTestCase()
{
    UnitTest::initTestCase();
    // A test fetches a tile that the tile cache of the process must not have yet
    QVERIFY(UnitTestTileGenerator::resetTileCache());
}

// The setting can name a provider that this build lacks: the query fails, it is neither queued for ever nor a crash
void TerrainTileManagerTest::_testUnknownElevationProviderFailsTheQuery()
{
    TestFixtures::SettingsFixture saved;
    saved.setFactValue(SettingsManager::instance()->flightMapSettings()->elevationMapProvider(),
                       QStringLiteral("No Such Provider"));

    TileReplyMockNam nam;
    const TileReplyNetworkScope networkScope(&nam);
    TerrainTileManager manager;
    TerrainQueryInterface query;
    QSignalSpy heightsSpy(&query, &TerrainQueryInterface::coordinateHeightsReceived);

    expectLogMessage("MapTiles.MapProviderRegistry", QtWarningMsg,
                     QRegularExpression(QStringLiteral("type not found:")));
    expectLogMessage("Terrain.TerrainTileManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("No elevation provider is named")));
    expectLogMessage("Terrain.TerrainTileManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("signalling failure due to internal error")));
    manager.addCoordinateQuery(&query, {QGeoCoordinate(-61.235, 97.415)});
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();

    QCOMPARE(heightsSpy.count(), 1);
    QVERIFY(!heightsSpy.constFirst().at(0).toBool());
    QCOMPARE(nam.requestCount, 0);
}

// Each tile has a request of its own: a tile that cannot be fetched fails the queries that need it, and the others
// get their heights
void TerrainTileManagerTest::_testFailedTileFailsOnlyQueriesThatNeedIt()
{
    TileReplyMockNam nam;
    nam.holdReplies = true;
    const TileReplyNetworkScope networkScope(&nam);
    TerrainTileManager manager;

    // Tiles are 0.01 degrees square
    const QGeoCoordinate inFailingTile(-61.235, 97.415);
    const QGeoCoordinate inOtherTile(-61.225, 97.415);
    TerrainQueryInterface failing;
    TerrainQueryInterface answered;
    TerrainQueryInterface needsBoth;
    QSignalSpy failingSpy(&failing, &TerrainQueryInterface::coordinateHeightsReceived);
    QSignalSpy answeredSpy(&answered, &TerrainQueryInterface::coordinateHeightsReceived);
    QSignalSpy needsBothSpy(&needsBoth, &TerrainQueryInterface::coordinateHeightsReceived);

    // The tile cache looks the failing tile up first: it misses, and the other tile is served from the cache
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());
    const UnitTestTileGenerator::ForcedMisses forcedMisses(1);
    manager.addCoordinateQuery(&failing, {inFailingTile});
    manager.addCoordinateQuery(&answered, {inOtherTile});
    manager.addCoordinateQuery(&needsBoth, {inOtherTile, inFailingTile});
    QTRY_COMPARE_WITH_TIMEOUT(nam.requestCount, 1, TestTimeout::mediumMs());

    // The other tile does not wait for the failing one
    QVERIFY_SIGNAL_COUNT_WAIT(answeredSpy, 1, TestTimeout::mediumMs());
    QVERIFY(answeredSpy.constFirst().at(0).toBool());
    QCOMPARE(answeredSpy.constFirst().at(1).value<QList<double>>().size(), 1);
    QVERIFY(failingSpy.isEmpty());
    QVERIFY(needsBothSpy.isEmpty());

    expectLogMessage("Terrain.TerrainTileManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Elevation tile fetching returned error")));
    nam.replies.constFirst()->failNow(QNetworkReply::InternalServerError, 500);
    QVERIFY_SIGNAL_COUNT_WAIT(failingSpy, 1, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    QVERIFY(!failingSpy.constFirst().at(0).toBool());
    QCOMPARE(needsBothSpy.count(), 1);
    QVERIFY(!needsBothSpy.constFirst().at(0).toBool());
    QCOMPARE(answeredSpy.count(), 1);
    QCOMPARE(nam.requestCount, 1);
}

// As a query for coordinates, a query for a path or for a carpet fails when a tile of it cannot be fetched: it is not
// left waiting
void TerrainTileManagerTest::_testFailedTileFailsPathAndCarpetQueries()
{
    TileReplyMockNam nam;
    nam.error = QNetworkReply::InternalServerError;
    nam.httpStatus = 500;
    nam.holdReplies = true;
    const TileReplyNetworkScope networkScope(&nam);
    TerrainTileManager manager;

    // Within one tile, with the elevations of the carpet that lie beyond its north east corner
    const QGeoCoordinate southWest(-61.278, 97.472);
    const QGeoCoordinate northEast(-61.2775, 97.4725);
    TerrainQueryInterface path;
    TerrainQueryInterface carpet;
    QSignalSpy pathSpy(&path, &TerrainQueryInterface::pathHeightsReceived);
    QSignalSpy carpetSpy(&carpet, &TerrainQueryInterface::carpetHeightsReceived);

    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());
    const UnitTestTileGenerator::ForcedMisses forcedMisses(1);
    manager.addPathQuery(&path, southWest, northEast);
    manager.addCarpetQuery(&carpet, southWest, northEast, false);
    QTRY_COMPARE_WITH_TIMEOUT(nam.requestCount, 1, TestTimeout::mediumMs());
    QVERIFY(pathSpy.isEmpty());
    QVERIFY(carpetSpy.isEmpty());

    expectLogMessage("Terrain.TerrainTileManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Elevation tile fetching returned error")));
    nam.replies.constFirst()->finishNow();
    QVERIFY_SIGNAL_COUNT_WAIT(pathSpy, 1, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    QVERIFY(!pathSpy.constFirst().at(0).toBool());
    QVERIFY(pathSpy.constFirst().at(3).value<QList<double>>().isEmpty());
    QCOMPARE(carpetSpy.count(), 1);
    QVERIFY(!carpetSpy.constFirst().at(0).toBool());
    QCOMPARE(nam.requestCount, 1);
}

// The vehicle asks for the terrain under it again and again (TERRAIN_REQUEST), and is answered at once with what the
// tiles in memory give: a tile that is not there is fetched for the next time it asks
void TerrainTileManagerTest::_testAltitudesAreGivenOnceTheirTileIsInMemory()
{
    TileReplyMockNam nam;
    nam.body = QByteArrayLiteral(R"({"status":"success","data":{"bounds":{"sw":[-61.26,97.45],"ne":[-61.25,97.46]},)"
                                 R"("stats":{"min":123,"max":123,"avg":123.0},"carpet":[[123,123],[123,123]]}})");
    nam.holdReplies = true;
    const TileReplyNetworkScope networkScope(&nam);
    TerrainTileManager manager;

    QList<double> altitudes;
    bool error = true;
    const auto altitudesKnown = [&](const QGeoCoordinate& coordinate) {
        altitudes.clear();
        return manager.getAltitudesForCoordinates({coordinate}, altitudes, error);
    };

    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());
    UnitTestTileGenerator::ForcedMisses forcedMisses(1);
    const QGeoCoordinate coordinate(-61.255, 97.455);
    QVERIFY(!altitudesKnown(coordinate));
    QVERIFY(!error);
    QTRY_COMPARE_WITH_TIMEOUT(nam.requestCount, 1, TestTimeout::mediumMs());
    // Asked again while its tile is fetched, the server is not
    QVERIFY(!altitudesKnown(coordinate));
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());
    QCOMPARE(nam.requestCount, 1);

    nam.replies.constFirst()->finishNow();
    QTRY_VERIFY_WITH_TIMEOUT(altitudesKnown(coordinate), TestTimeout::mediumMs());
    QVERIFY(!error);
    QCOMPARE(altitudes, QList<double>{123.});

    // A tile that could not be fetched is not fetched again at once: its altitudes are an error until then, and
    // nothing to wait for
    const QGeoCoordinate inFailingTile(-61.265, 97.465);
    forcedMisses.setCount(1);
    nam.error = QNetworkReply::InternalServerError;
    nam.httpStatus = 500;
    nam.holdReplies = false;
    expectLogMessage("Terrain.TerrainTileManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Elevation tile fetching returned error")));
    QVERIFY(!altitudesKnown(inFailingTile));
    QTRY_VERIFY_WITH_TIMEOUT(altitudesKnown(inFailingTile), TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    QVERIFY(error);
    QCOMPARE(altitudes.size(), 1);
    QVERIFY(qIsNaN(altitudes.constFirst()));
    QCOMPARE(nam.requestCount, 2);
}

// A tile of the cache that cannot be read is not the end of the heights of its place: the server is asked, and its
// tile takes the place of the one in the cache
void TerrainTileManagerTest::_testUnreadableCachedTileIsFetchedAgain()
{
    const QGeoCoordinate coordinate(-61.245, 97.445);
    const SharedMapProvider provider = MapProviderRegistry::getMapProviderFromProviderType(
        SettingsManager::instance()->flightMapSettings()->elevationMapProvider()->rawValue().toString());
    QVERIFY(provider);
    const QString hash = provider->getTileHash(provider->long2tileX(coordinate.longitude(), 1),
                                               provider->lat2tileY(coordinate.latitude(), 1), 1);
    TileCache::instance()->ensureInitialized();
    TileCache::instance()->cacheTile(provider->mapId(), hash, QByteArrayLiteral("unreadable"), QStringLiteral("bin"));
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());

    // What the server of the elevation provider answers for the tile of the coordinate
    TileReplyMockNam nam;
    nam.body = QByteArrayLiteral(R"({"status":"success","data":{"bounds":{"sw":[-61.25,97.44],"ne":[-61.24,97.45]},)"
                                 R"("stats":{"min":123,"max":123,"avg":123.0},"carpet":[[123,123],[123,123]]}})");
    const TileReplyNetworkScope networkScope(&nam);

    {
        TerrainTileManager manager;
        TerrainQueryInterface query;
        QSignalSpy heightsSpy(&query, &TerrainQueryInterface::coordinateHeightsReceived);
        expectLogMessage("Terrain.terraintile", QtWarningMsg,
                         QRegularExpression(QStringLiteral("binary data too small")));
        manager.addCoordinateQuery(&query, {coordinate});
        QVERIFY_SIGNAL_COUNT_WAIT(heightsSpy, 1, TestTimeout::mediumMs());
        verifyExpectedLogMessage();
        QVERIFY(heightsSpy.constFirst().at(0).toBool());
        QCOMPARE(heightsSpy.constFirst().at(1).value<QList<double>>(), QList<double>{123.});
        QCOMPARE(nam.requestCount, 1);
    }

    // A manager without the tile in memory gets the one of the server from the cache
    TerrainTileManager manager;
    TerrainQueryInterface query;
    QSignalSpy heightsSpy(&query, &TerrainQueryInterface::coordinateHeightsReceived);
    manager.addCoordinateQuery(&query, {coordinate});
    QVERIFY_SIGNAL_COUNT_WAIT(heightsSpy, 1, TestTimeout::mediumMs());
    QVERIFY(heightsSpy.constFirst().at(0).toBool());
    QCOMPARE(heightsSpy.constFirst().at(1).value<QList<double>>(), QList<double>{123.});
    QCOMPARE(nam.requestCount, 1);
}

UT_REGISTER_TEST(TerrainTileManagerTest, TestLabel::Unit, TestLabel::Terrain)
