/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "Viewer3DTerrainTextureTest.h"

#include <QtPositioning/QGeoCoordinate>
#include <QtTest/QSignalSpy>

#include "Viewer3DTerrainTexture.h"

void Viewer3DTerrainTextureTest::_testFallbackBoundingBox()
{
    const QGeoCoordinate center(47.397742, 8.545594);

    const auto [bbMin, bbMax] = Viewer3DTerrainTexture::_fallbackBoundingBox(center);

    QVERIFY(bbMin.isValid());
    QVERIFY(bbMax.isValid());
    QVERIFY(bbMin.latitude() < center.latitude());
    QVERIFY(bbMax.latitude() > center.latitude());
    QVERIFY(bbMin.longitude() < center.longitude());
    QVERIFY(bbMax.longitude() > center.longitude());

    // Each edge is 500m from the center
    const QGeoCoordinate south(bbMin.latitude(), center.longitude());
    const QGeoCoordinate north(bbMax.latitude(), center.longitude());
    const QGeoCoordinate west(center.latitude(), bbMin.longitude());
    const QGeoCoordinate east(center.latitude(), bbMax.longitude());
    QVERIFY(qAbs(center.distanceTo(south) - 500.0) < 1.0);
    QVERIFY(qAbs(center.distanceTo(north) - 500.0) < 1.0);
    QVERIFY(qAbs(center.distanceTo(west) - 500.0) < 1.0);
    QVERIFY(qAbs(center.distanceTo(east) - 500.0) < 1.0);
}

void Viewer3DTerrainTextureTest::_testInvalidFallbackCenterNoLoad()
{
    Viewer3DTerrainTexture texture;

    QSignalSpy centerSpy(&texture, &Viewer3DTerrainTexture::fallbackCenterChanged);

    texture.setFallbackCenter(QGeoCoordinate());

    QCOMPARE(centerSpy.count(), 0);
    QVERIFY(!texture._terrainTileLoader);
}

void Viewer3DTerrainTextureTest::_testFallbackCenterTriggersLoad()
{
    Viewer3DTerrainTexture texture;

    QSignalSpy centerSpy(&texture, &Viewer3DTerrainTexture::fallbackCenterChanged);

    // No map provider: a valid fallback center must kick off tile loading
    texture.setFallbackCenter(QGeoCoordinate(47.397742, 8.545594));

    QCOMPARE(centerSpy.count(), 1);
    QVERIFY(texture._terrainTileLoader);
}

UT_REGISTER_TEST(Viewer3DTerrainTextureTest, TestLabel::Unit)
