#include "Viewer3DTileQueryTest.h"

#include <QtPositioning/QGeoCoordinate>

#include "MapProvider.h"
#include "QGCMapUrlEngine.h"
#include "Viewer3DTileQuery.h"

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

UT_REGISTER_TEST(Viewer3DTileQueryTest, TestLabel::Unit)
