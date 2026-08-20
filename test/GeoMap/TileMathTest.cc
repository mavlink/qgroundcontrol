#include "TileMathTest.h"

#include <QtCore/QtMath>

#include "TileMath.h"

using namespace TileMath;

void TileMathTest::_geoWorldOrigin()
{
    const QPointF world = geoToWorld(QGeoCoordinate(0, 0));
    QCOMPARE_LT(qAbs(world.x()), 1e-6);
    QCOMPARE_LT(qAbs(world.y()), 1e-6);

    // Antimeridian maps to the world edge
    const double half = worldSize() / 2.0;
    QCOMPARE_LT(qAbs(geoToWorld(QGeoCoordinate(0, 180)).x() - half), 1e-6);
    QCOMPARE_LT(qAbs(geoToWorld(QGeoCoordinate(0, -180)).x() + half), 1e-6);
}

void TileMathTest::_geoWorldRoundTrip()
{
    const QList<QGeoCoordinate> coords = {
        QGeoCoordinate(47.3977419, 8.5455938),  // Zurich
        QGeoCoordinate(-33.8688, 151.2093),     // Sydney
        QGeoCoordinate(64.15, -21.95),          // Reykjavik
        QGeoCoordinate(0.0, 0.0),
        QGeoCoordinate(-85.0, 179.9),
    };
    for (const QGeoCoordinate& coord : coords) {
        const QGeoCoordinate back = worldToGeo(geoToWorld(coord));
        QCOMPARE_LT(qAbs(back.latitude() - coord.latitude()), 1e-9);
        QCOMPARE_LT(qAbs(back.longitude() - coord.longitude()), 1e-9);
    }
}

void TileMathTest::_latitudeClamp()
{
    const QPointF clamped = geoToWorld(QGeoCoordinate(89.0, 10.0));
    const QPointF atLimit = geoToWorld(QGeoCoordinate(kMaxLatitude, 10.0));
    QCOMPARE(clamped, atLimit);

    // Mercator world is square: y at the latitude limit equals half the world size
    const double half = worldSize() / 2.0;
    QCOMPARE_LT(qAbs(atLimit.y() - half), 1e-3);
}

void TileMathTest::_metersPerPixel()
{
    // Canonical Web Mercator zoom 0 resolution at 256px tiles
    QCOMPARE_LT(qAbs(metersPerPixelAtZoom(0) - 156543.03392804097), 1e-6);
    QCOMPARE_LT(qAbs(metersPerPixelAtZoom(1) - (156543.03392804097 / 2.0)), 1e-6);
}

void TileMathTest::_tileZoom0()
{
    const TileKey key = tileForWorld(QPointF(0, 0), 0);
    QCOMPARE(key, (TileKey{0, 0, 0}));

    const double half = worldSize() / 2.0;
    const QPointF minCorner = tileMinCorner(key);
    QCOMPARE_LT(qAbs(minCorner.x() + half), 1e-6);
    QCOMPARE_LT(qAbs(minCorner.y() + half), 1e-6);
    QCOMPARE_LT(qAbs(tileSpanAtZoom(0) - worldSize()), 1e-6);
}

void TileMathTest::_tileZoom1Quadrants()
{
    const double quarter = worldSize() / 4.0;
    // Slippy scheme: (0,0) is north-west
    QCOMPARE(tileForWorld(QPointF(-quarter, quarter), 1), (TileKey{0, 0, 1}));   // NW
    QCOMPARE(tileForWorld(QPointF(quarter, quarter), 1), (TileKey{1, 0, 1}));    // NE
    QCOMPARE(tileForWorld(QPointF(-quarter, -quarter), 1), (TileKey{0, 1, 1}));  // SW
    QCOMPARE(tileForWorld(QPointF(quarter, -quarter), 1), (TileKey{1, 1, 1}));   // SE
}

void TileMathTest::_tileWorldRoundTrip()
{
    for (int zoom : {2, 8, 15}) {
        const TileKey key{(1 << zoom) / 3, (1 << zoom) / 5, zoom};
        const double span = tileSpanAtZoom(zoom);
        const QPointF tileCenter = tileMinCorner(key) + QPointF(span / 2.0, span / 2.0);
        QCOMPARE(tileForWorld(tileCenter, zoom), key);
    }
}

void TileMathTest::_tileEdgeClamp()
{
    const double half = worldSize() / 2.0;
    const int last = (1 << 5) - 1;
    QCOMPARE(tileForWorld(QPointF(half, -half), 5), (TileKey{last, last, 5}));
    QCOMPARE(tileForWorld(QPointF(-half - 1000.0, half + 1000.0), 5), (TileKey{0, 0, 5}));
}

void TileMathTest::_zoomForMetersPerPixel()
{
    QCOMPARE(zoomForMetersPerPixel(metersPerPixelAtZoom(5)), 5);
    // Slightly finer resolution requires the next zoom level
    QCOMPARE(zoomForMetersPerPixel(metersPerPixelAtZoom(5) * 0.99), 6);
    QCOMPARE(zoomForMetersPerPixel(1e9), kMinZoom);
    QCOMPARE(zoomForMetersPerPixel(0.0), kMaxZoom);
    QCOMPARE(zoomForMetersPerPixel(1e-9), kMaxZoom);
}

void TileMathTest::_mercatorScale()
{
    // 1 at the equator, 1/cos(lat) elsewhere, symmetric in hemisphere
    QCOMPARE_LT(qAbs(mercatorScale(0.0) - 1.0), 1e-12);
    QCOMPARE_LT(qAbs(mercatorScale(60.0) - 2.0), 1e-12);
    QCOMPARE_LT(qAbs(mercatorScale(-60.0) - 2.0), 1e-12);

    // Consistent with the projection: d(world.y)/d(ground meters) at 60 deg,
    // where ground meters are the meridian arc on the projection's own sphere
    const double dLatDeg = 0.0001;
    const double worldDy =
        geoToWorld(QGeoCoordinate(60.0 + dLatDeg, 0.0)).y() - geoToWorld(QGeoCoordinate(60.0, 0.0)).y();
    const double groundDy = qDegreesToRadians(dLatDeg) * kEarthRadius;
    QCOMPARE_LT(qAbs((worldDy / groundDy) - mercatorScale(60.0)), 1e-3);

    // Clamped at the mercator latitude limit: finite beyond it
    QCOMPARE(mercatorScale(89.0), mercatorScale(kMaxLatitude));
}

void TileMathTest::_isValidKey()
{
    QVERIFY(isValidKey(TileKey{0, 0, 0}));
    QVERIFY(isValidKey(TileKey{7, 0, 3}));
    QVERIFY(isValidKey(TileKey{(1 << kMaxZoom) - 1, (1 << kMaxZoom) - 1, kMaxZoom}));

    QVERIFY(!isValidKey(TileKey{0, 0, -1}));
    QVERIFY(!isValidKey(TileKey{0, 0, kMaxZoom + 1}));
    QVERIFY(!isValidKey(TileKey{-1, 0, 3}));
    QVERIFY(!isValidKey(TileKey{8, 0, 3}));
    QVERIFY(!isValidKey(TileKey{0, -1, 3}));
    QVERIFY(!isValidKey(TileKey{0, 8, 3}));
}

UT_REGISTER_TEST_LIGHTWEIGHT(TileMathTest, TestLabel::Unit)
