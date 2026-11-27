#include "Viewer3DTileQueryTest.h"

#include <chrono>

#include <QtCore/QRegularExpression>
#include <QtGui/QImage>
#include <QtPositioning/QGeoCoordinate>
#include <QtTest/QSignalSpy>

#include "ManualScheduler.h"
#include "MapProvider.h"
#include "MapProviderRegistry.h"
#include "TileReplyNetworkMock.h"
#include "TileServerPauses.h"
#include "UnitTestTileGenerator.h"
#include "Viewer3DTileQuery.h"

namespace {

/// The texture of \a query before a tile is drawn on it
QByteArray viewer3DBlankTexture(const Viewer3DTileQuery& query)
{
    QImage image(query.mapSize(), QImage::Format_RGBA32FPx4);
    image.fill(Qt::gray);
    return QByteArray(reinterpret_cast<const char*>(image.constBits()), image.sizeInBytes());
}

}  // namespace

void Viewer3DTileQueryTest::initTestCase()
{
    UnitTest::initTestCase();
    // A test fetches a tile that the tile cache of the process must not have yet
    QVERIFY(UnitTestTileGenerator::resetTileCache());
}

void Viewer3DTileQueryTest::_testMaxTileCount()
{
    Viewer3DTileQuery query;

    const auto& providers = MapProviderRegistry::providers();
    QVERIFY(!providers.isEmpty());
    query._mapId = providers.first()->mapId();

    QGeoCoordinate min(47.0, 8.0);
    QGeoCoordinate max(47.1, 8.1);

    int count10 = query.maxTileCount(10, min, max);
    int count15 = query.maxTileCount(15, min, max);

    QVERIFY(count10 >= 1);
    QVERIFY(count15 > count10);
}

void Viewer3DTileQueryTest::_testTileCoordinateRoundTrip()
{
    const auto& providers = MapProviderRegistry::providers();
    QVERIFY(!providers.isEmpty());
    const SharedMapProvider provider = providers.first();

    struct TestCase
    {
        double coord;
        int zoom;
    };

    // X: forward → inverse → point slightly inside tile → forward gives same tile
    const TestCase lonCases[] = {
        {-179.5, 1}, {0.0, 1}, {179.5, 1}, {-122.4, 10}, {0.0, 10}, {8.5, 10}, {-122.4, 18}, {37.8, 18},
    };
    for (const auto& tc : lonCases) {
        const int tileX = provider->long2tileX(tc.coord, tc.zoom);
        const double lon = provider->tileX2long(tileX, tc.zoom);
        // tileX2long gives the west edge; a point slightly east is inside the tile
        const int roundTrip = provider->long2tileX(lon + 0.0001, tc.zoom);
        QCOMPARE(roundTrip, tileX);
    }

    // Y: forward → inverse → point slightly inside tile → forward gives same tile
    const TestCase latCases[] = {
        {-80.0, 1}, {0.0, 1}, {80.0, 1}, {-33.9, 10}, {0.0, 10}, {47.5, 10}, {-33.9, 18}, {47.5, 18},
    };
    for (const auto& tc : latCases) {
        const int tileY = provider->lat2tileY(tc.coord, tc.zoom);
        const double lat = provider->tileY2lat(tileY, tc.zoom);
        // tileY2lat gives the north edge; a point slightly south is inside the tile
        const int roundTrip = provider->lat2tileY(lat - 0.0001, tc.zoom);
        QCOMPARE(roundTrip, tileY);
    }
}

void Viewer3DTileQueryTest::_testCoarserZoomWhereServerHasNoTile_data()
{
    QTest::addColumn<bool>("tileAtNextZoom");

    QTest::newRow("tile at the next zoom") << true;
    // The map is then done without a tile: it must not wait for one for ever
    QTest::newRow("no tile at any zoom") << false;
}

// A server answers 404 where it has no tile: the map is then loaded one zoom level out, as it is for the picture
// that Bing sends there
void Viewer3DTileQueryTest::_testCoarserZoomWhereServerHasNoTile()
{
    QFETCH(bool, tileAtNextZoom);

    TileReplyMockNam nam;
    const TileReplyNetworkScope networkScope(&nam);
    nam.error = QNetworkReply::ContentNotFoundError;  // as Qt reports that status
    nam.httpStatus = 404;

    // Under unit tests UnitTestTileGenerator serves a tile that the cache lacks, unless a miss is forced: the server
    // is asked for the tile of the finest zoom only, or for the tile of every zoom
    const UnitTestTileGenerator::ForcedMisses forcedMisses(
        tileAtNextZoom ? 1 : UnitTestTileGenerator::ForcedMisses::kEveryLookup);

    QList<int> zoomLevels;
    Viewer3DTileQuery query;
    (void) connect(&query, &Viewer3DTileQuery::textureGeometryReady, &query,
                   [&zoomLevels](const Viewer3DTileStatistics& geometry) { zoomLevels.append(geometry.zoomLevel); });
    QSignalSpy completedSpy(&query, &Viewer3DTileQuery::loadingMapCompleted);

    // A region within one tile at every zoom
    const QGeoCoordinate position(47.3977, 8.5456);
    query.adaptiveMapTilesLoader(MapProviderRegistry::getMapIdFromProviderType(QStringLiteral("Street Map")), position,
                                 position);

    QVERIFY_SIGNAL_COUNT_WAIT(completedSpy, 1, TestTimeout::mediumMs());
    QVERIFY(!zoomLevels.isEmpty());
    const int lastZoom = tileAtNextZoom ? (zoomLevels.constFirst() - 1) : 0;
    QList<int> expectedZoomLevels;
    for (int zoom = zoomLevels.constFirst(); zoom >= lastZoom; zoom--) {
        expectedZoomLevels.append(zoom);
    }
    QCOMPARE(zoomLevels, expectedZoomLevels);
    QCOMPARE(nam.requestCount, tileAtNextZoom ? 1 : static_cast<int>(expectedZoomLevels.count()));
}

void Viewer3DTileQueryTest::_testTileOutcome_data()
{
    QTest::addColumn<double>("longitude");  // a tile of its own for every row
    QTest::addColumn<bool>("cached");
    QTest::addColumn<QByteArray>("serverBody");
    QTest::addColumn<bool>("drawn");

    QTest::newRow("cached tile") << 7.10 << true << QByteArray() << true;
    QTest::newRow("server tile") << 7.20 << false << tileReplyPngBody() << true;
    // The map is then done without the tile
    QTest::newRow("server answer that is no tile") << 7.30 << false << QByteArray() << false;
}

void Viewer3DTileQueryTest::_testTileOutcome()
{
    QFETCH(double, longitude);
    QFETCH(bool, cached);
    QFETCH(QByteArray, serverBody);
    QFETCH(bool, drawn);

    TileReplyMockNam nam;
    const TileReplyNetworkScope networkScope(&nam);
    nam.body = serverBody;
    nam.httpStatus = 200;
    const UnitTestTileGenerator::ForcedMisses forcedMisses(cached ? 0
                                                                  : UnitTestTileGenerator::ForcedMisses::kEveryLookup);
    if (!drawn) {
        expectLogMessage("Viewer3d.Viewer3DTileQuery", QtWarningMsg,
                         QRegularExpression(QStringLiteral("Request error for tile")));
    }

    Viewer3DTileQuery query;
    QSignalSpy completedSpy(&query, &Viewer3DTileQuery::loadingMapCompleted);
    const QGeoCoordinate position(46.50, longitude);
    query.adaptiveMapTilesLoader(
        MapProviderRegistry::getMapIdFromProviderType(UnitTestTileGenerator::mapProviderType()), position, position);

    QVERIFY_SIGNAL_COUNT_WAIT(completedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(query.mapData() != viewer3DBlankTexture(query), drawn);
    QCOMPARE(nam.requestCount, cached ? 0 : 1);
    if (!drawn) {
        verifyExpectedLogMessage();
    }
}

// A load replaces the one before it: the requests for the tiles of that one are aborted
void Viewer3DTileQueryTest::_testNewLoadAbortsEarlierLoad()
{
    TileReplyMockNam nam;
    const TileReplyNetworkScope networkScope(&nam);
    nam.body = tileReplyPngBody();
    nam.httpStatus = 200;
    nam.holdReplies = true;
    const UnitTestTileGenerator::ForcedMisses forcedMisses(UnitTestTileGenerator::ForcedMisses::kEveryLookup);

    Viewer3DTileQuery query;
    QSignalSpy completedSpy(&query, &Viewer3DTileQuery::loadingMapCompleted);
    const int mapId = MapProviderRegistry::getMapIdFromProviderType(UnitTestTileGenerator::mapProviderType());
    const QGeoCoordinate earlier(46.60, 7.40);
    query.adaptiveMapTilesLoader(mapId, earlier, earlier);
    QTRY_COMPARE_WITH_TIMEOUT(nam.requestCount, 1, TestTimeout::mediumMs());

    const QGeoCoordinate later(46.70, 7.50);
    query.adaptiveMapTilesLoader(mapId, later, later);
    QTRY_COMPARE_WITH_TIMEOUT(nam.requestCount, 2, TestTimeout::mediumMs());
    // The request of the earlier load let go of its network reply, which never finished
    QTRY_VERIFY_WITH_TIMEOUT(nam.replies.constFirst().isNull(), TestTimeout::mediumMs());

    nam.replies.constLast()->finishNow();
    QVERIFY_SIGNAL_COUNT_WAIT(completedSpy, 1, TestTimeout::mediumMs());
    QVERIFY(query.mapData() != viewer3DBlankTexture(query));
    QCOMPARE(nam.requestCount, 2);
}

void Viewer3DTileQueryTest::_testTileThatFailsForNowIsAskedForAgain_data()
{
    QTest::addColumn<double>("longitude");  // a tile of its own for every row
    QTest::addColumn<QNetworkReply::NetworkError>("networkError");
    QTest::addColumn<int>("httpStatus");
    QTest::addColumn<int>("delayMs");
    QTest::addColumn<bool>("serverPaused");

    const int retryDelay = static_cast<int>(Viewer3DTileQuery::kRetryDelays.front().count());
    QTest::newRow("server that does not answer") << 7.60 << QNetworkReply::TimeoutError << 0 << retryDelay << false;
    QTest::newRow("server that cannot serve the tile now")
        << 7.70 << QNetworkReply::ServiceUnavailableError << 503 << retryDelay << false;
    // The request that is made again waits for the pause of the server, as every request for a tile of it does
    const int pause = static_cast<int>(
        std::chrono::duration_cast<std::chrono::milliseconds>(TileServerPauses::kDefaultPause).count());
    QTest::newRow("server that was asked too often")
        << 7.80 << QNetworkReply::UnknownContentError << 429 << pause << true;
}

// The connection of a ground station in the field drops a request now and then: the map is not left with a hole for
// a tile that the server has a moment later
void Viewer3DTileQueryTest::_testTileThatFailsForNowIsAskedForAgain()
{
    QFETCH(double, longitude);
    QFETCH(QNetworkReply::NetworkError, networkError);
    QFETCH(int, httpStatus);
    QFETCH(int, delayMs);
    QFETCH(bool, serverPaused);

    ManualScheduler scheduler;
    const TileReplyServerPauses serverPauses(&scheduler);
    TileReplyMockNam nam;
    const TileReplyNetworkScope networkScope(&nam);
    nam.error = networkError;
    nam.httpStatus = httpStatus;
    const UnitTestTileGenerator::ForcedMisses forcedMisses(UnitTestTileGenerator::ForcedMisses::kEveryLookup);

    Viewer3DTileQuery query(nullptr, &scheduler);
    QSignalSpy completedSpy(&query, &Viewer3DTileQuery::loadingMapCompleted);
    const QGeoCoordinate position(46.80, longitude);
    if (serverPaused) {
        expectLogMessage("MapTiles.TileServerPauses", QtWarningMsg,
                         QRegularExpression(QStringLiteral("asked too often")));
    }
    query.adaptiveMapTilesLoader(
        MapProviderRegistry::getMapIdFromProviderType(UnitTestTileGenerator::mapProviderType()), position, position);
    QTRY_COMPARE_WITH_TIMEOUT(scheduler.pendingCount(), 1, TestTimeout::mediumMs());
    if (serverPaused) {
        verifyExpectedLogMessage();
    }
    QCOMPARE(nam.requestCount, 1);

    // The server has the tile from here on. It is not asked before the delay has passed, and the map waits for it.
    nam.body = tileReplyPngBody();
    nam.error = QNetworkReply::NoError;
    nam.httpStatus = 200;
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(delayMs - 1)));
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());
    QCOMPARE(nam.requestCount, 1);
    QVERIFY(completedSpy.isEmpty());

    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
    QVERIFY_SIGNAL_COUNT_WAIT(completedSpy, 1, TestTimeout::mediumMs());
    QVERIFY(query.mapData() != viewer3DBlankTexture(query));
    QCOMPARE(nam.requestCount, 2);
}

// A server that stays away is not asked for ever: the map is done without the tile, as before, and says so once
void Viewer3DTileQueryTest::_testTileThatKeepsFailingIsLeftOut()
{
    ManualScheduler scheduler;
    const TileReplyServerPauses serverPauses(&scheduler);
    TileReplyMockNam nam;
    const TileReplyNetworkScope networkScope(&nam);
    nam.error = QNetworkReply::TimeoutError;
    const UnitTestTileGenerator::ForcedMisses forcedMisses(UnitTestTileGenerator::ForcedMisses::kEveryLookup);

    Viewer3DTileQuery query(nullptr, &scheduler);
    QSignalSpy completedSpy(&query, &Viewer3DTileQuery::loadingMapCompleted);
    const QGeoCoordinate position(46.80, 7.90);
    expectLogMessage("Viewer3d.Viewer3DTileQuery", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Request error for tile")));
    query.adaptiveMapTilesLoader(
        MapProviderRegistry::getMapIdFromProviderType(UnitTestTileGenerator::mapProviderType()), position, position);
    for (const std::chrono::milliseconds delay : Viewer3DTileQuery::kRetryDelays) {
        QTRY_COMPARE_WITH_TIMEOUT(scheduler.pendingCount(), 1, TestTimeout::mediumMs());
        QVERIFY(completedSpy.isEmpty());
        QVERIFY(scheduler.advanceBy(delay));
    }
    QVERIFY_SIGNAL_COUNT_WAIT(completedSpy, 1, TestTimeout::mediumMs());
    verifyExpectedLogMessage();

    QCOMPARE(query.mapData(), viewer3DBlankTexture(query));
    QCOMPARE(nam.requestCount, 1 + static_cast<int>(Viewer3DTileQuery::kRetryDelays.size()));
    QCOMPARE(scheduler.pendingCount(), 0);
}

// The tile of a load that another one replaced is not shown: it is not asked for again either
void Viewer3DTileQueryTest::_testNewLoadDropsRetryOfEarlierLoad()
{
    ManualScheduler scheduler;
    const TileReplyServerPauses serverPauses(&scheduler);
    TileReplyMockNam nam;
    const TileReplyNetworkScope networkScope(&nam);
    nam.error = QNetworkReply::TimeoutError;
    const UnitTestTileGenerator::ForcedMisses forcedMisses(UnitTestTileGenerator::ForcedMisses::kEveryLookup);

    Viewer3DTileQuery query(nullptr, &scheduler);
    QSignalSpy completedSpy(&query, &Viewer3DTileQuery::loadingMapCompleted);
    const int mapId = MapProviderRegistry::getMapIdFromProviderType(UnitTestTileGenerator::mapProviderType());
    const QGeoCoordinate earlier(46.80, 8.00);
    query.adaptiveMapTilesLoader(mapId, earlier, earlier);
    QTRY_COMPARE_WITH_TIMEOUT(scheduler.pendingCount(), 1, TestTimeout::mediumMs());

    nam.body = tileReplyPngBody();
    nam.error = QNetworkReply::NoError;
    nam.httpStatus = 200;
    const QGeoCoordinate later(46.80, 8.10);
    query.adaptiveMapTilesLoader(mapId, later, later);
    QCOMPARE(scheduler.pendingCount(), 0);
    QVERIFY_SIGNAL_COUNT_WAIT(completedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(nam.requestCount, 2);

    QVERIFY(scheduler.advanceBy(std::chrono::hours(1)));
    QVERIFY(UnitTestTileGenerator::waitForCacheTasks());
    QCOMPARE(nam.requestCount, 2);
    QCOMPARE(completedSpy.count(), 1);
}

UT_REGISTER_TEST(Viewer3DTileQueryTest, TestLabel::Unit)
