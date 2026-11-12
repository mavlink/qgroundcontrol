#include "Viewer3DTileQueryTest.h"

#include <QtCore/QScopeGuard>
#include <QtPositioning/QGeoCoordinate>
#include <QtTest/QSignalSpy>

#include "MapProvider.h"
#include "QGCMapUrlEngine.h"
#include "TileReplyNetworkMock.h"
#include "UnitTestTileGenerator.h"
#include "Viewer3DTileQuery.h"

void Viewer3DTileQueryTest::initTestCase()
{
    UnitTest::initTestCase();
    // A test fetches a tile that the tile cache of the process must not have yet
    QVERIFY(UnitTestTileGenerator::resetTileCache());
}

void Viewer3DTileQueryTest::_testMaxTileCount()
{
    Viewer3DTileQuery query;

    const auto& providers = UrlFactory::getProviders();
    QVERIFY(!providers.isEmpty());
    query._mapId = providers.first()->getMapId();

    QGeoCoordinate min(47.0, 8.0);
    QGeoCoordinate max(47.1, 8.1);

    int count10 = query.maxTileCount(10, min, max);
    int count15 = query.maxTileCount(15, min, max);

    QVERIFY(count10 >= 1);
    QVERIFY(count15 > count10);
}

void Viewer3DTileQueryTest::_testTileCoordinateRoundTrip()
{
    const auto& providers = UrlFactory::getProviders();
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
    nam.error = QNetworkReply::ContentNotFoundError;  // as Qt reports that status
    nam.httpStatus = 404;

    // Under unit tests UnitTestTileGenerator serves a tile that the cache lacks, unless a miss is forced: the server
    // is asked for the tile of the finest zoom only, or for the tile of every zoom
    constexpr int kMoreMissesThanZooms = 100;
    const UnitTestTileGenerator::ForcedMisses forcedMisses(tileAtNextZoom ? 1 : kMoreMissesThanZooms);

    QList<int> zoomLevels;
    Viewer3DTileQuery query;
    query._networkManager = &nam;
    (void) connect(&query, &Viewer3DTileQuery::textureGeometryReady, &query,
                   [&zoomLevels](const Viewer3DTileStatistics& geometry) { zoomLevels.append(geometry.zoomLevel); });
    QSignalSpy completedSpy(&query, &Viewer3DTileQuery::loadingMapCompleted);

    // A region within one tile at every zoom
    const QGeoCoordinate position(47.3977, 8.5456);
    query.adaptiveMapTilesLoader(UrlFactory::getMapIdFromProviderType(QStringLiteral("Street Map")), position,
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

UT_REGISTER_TEST(Viewer3DTileQueryTest, TestLabel::Unit)
