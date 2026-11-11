/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "Viewer3DTerrainTextureTest.h"

#include <QtCore/QScopeGuard>
#include <QtPositioning/QGeoCoordinate>
#include <QtTest/QSignalSpy>

#include "AppSettings.h"
#include "FlightMapSettings.h"
#include "SettingsManager.h"
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

void Viewer3DTerrainTextureTest::_testSourceChangeReloadsTexture()
{
    // The texture is made of tiles of the map type the user selected: here the provider whose URL the user sets
    FlightMapSettings* const flightMapSettings = SettingsManager::instance()->flightMapSettings();
    Fact* const customUrl = SettingsManager::instance()->appSettings()->customURL();
    const QVariant savedProvider = flightMapSettings->mapProvider()->rawValue();
    const QVariant savedType = flightMapSettings->mapType()->rawValue();
    const QVariant savedUrl = customUrl->rawValue();
    const auto restore = qScopeGuard([=] {
        flightMapSettings->mapProvider()->setRawValue(savedProvider);
        flightMapSettings->mapType()->setRawValue(savedType);
        customUrl->setRawValue(savedUrl);
    });
    flightMapSettings->mapProvider()->setRawValue(QStringLiteral("CustomURL"));
    flightMapSettings->mapType()->setRawValue(QStringLiteral("Custom"));
    customUrl->setRawValue(QStringLiteral("https://a.example/{z}/{x}/{y}.png"));

    Viewer3DTerrainTexture texture;
    texture.setFallbackCenter(QGeoCoordinate(47.397742, 8.545594));
    QTRY_VERIFY_WITH_TIMEOUT(texture.textureLoaded(), TestTimeout::mediumMs());

    // The user sets another URL: the texture is one of the previous source, and is loaded again
    customUrl->setRawValue(QStringLiteral("https://b.example/{z}/{x}/{y}.png"));
    QVERIFY(!texture.textureLoaded());
    QTRY_VERIFY_WITH_TIMEOUT(texture.textureLoaded(), TestTimeout::mediumMs());
}

UT_REGISTER_TEST(Viewer3DTerrainTextureTest, TestLabel::Unit)
