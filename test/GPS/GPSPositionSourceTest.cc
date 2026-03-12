#include "GPSPositionSourceTest.h"
#include "GPSPositionSource.h"
#include "UnitTest.h"

#include <QtPositioning/QGeoPositionInfo>
#include <QtTest/QSignalSpy>

#include <cmath>

static sensor_gps_s _makeGpsData(uint8_t fixType, double lat, double lon, double alt)
{
    sensor_gps_s gps{};
    gps.fix_type = fixType;
    gps.latitude_deg = lat;
    gps.longitude_deg = lon;
    gps.altitude_msl_m = alt;
    gps.time_utc_usec = 1700000000000000ULL; // 2023-11-14 ~22:13 UTC
    gps.eph = 1.5f;
    gps.epv = 2.0f;
    gps.vel_m_s = 0.5f;
    gps.cog_rad = static_cast<float>(M_PI / 4.0); // 45 degrees
    gps.heading_accuracy = 5.0f;
    return gps;
}

void GPSPositionSourceTest::testDefaults()
{
    GPSPositionSource source;

    QVERIFY(!source.lastKnownPosition().isValid());
    QCOMPARE(source.supportedPositioningMethods(), QGeoPositionInfoSource::SatellitePositioningMethods);
    QCOMPARE(source.minimumUpdateInterval(), 100);
    QCOMPARE(source.error(), QGeoPositionInfoSource::NoError);
}

void GPSPositionSourceTest::testStartStop()
{
    GPSPositionSource source;

    auto gps = _makeGpsData(sensor_gps_s::FIX_TYPE_3D, 47.0, 8.0, 500.0);
    QSignalSpy spy(&source, &QGeoPositionInfoSource::positionUpdated);

    // Not started — should not emit
    source.updateFromSensorGps(gps);
    QCOMPARE(spy.count(), 0);

    source.startUpdates();
    source.updateFromSensorGps(gps);
    QCOMPARE(spy.count(), 1);

    source.stopUpdates();
    source.updateFromSensorGps(gps);
    QCOMPARE(spy.count(), 1);
}

void GPSPositionSourceTest::testUpdateFromSensorGps_noFix()
{
    GPSPositionSource source;
    source.startUpdates();

    QSignalSpy spy(&source, &QGeoPositionInfoSource::positionUpdated);

    auto gps = _makeGpsData(sensor_gps_s::FIX_TYPE_NONE, 47.0, 8.0, 500.0);
    source.updateFromSensorGps(gps);

    QCOMPARE(spy.count(), 0);
    QVERIFY(!source.lastKnownPosition().isValid());
}

void GPSPositionSourceTest::testUpdateFromSensorGps_2dFix()
{
    GPSPositionSource source;
    source.startUpdates();

    QSignalSpy spy(&source, &QGeoPositionInfoSource::positionUpdated);

    auto gps = _makeGpsData(sensor_gps_s::FIX_TYPE_2D, 47.123, 8.456, 500.0);
    source.updateFromSensorGps(gps);

    QCOMPARE(spy.count(), 1);

    const auto info = spy.at(0).at(0).value<QGeoPositionInfo>();
    QVERIFY(info.isValid());
    QCOMPARE(info.coordinate().latitude(), 47.123);
    QCOMPARE(info.coordinate().longitude(), 8.456);
    // 2D fix should not set altitude
    QVERIFY(qIsNaN(info.coordinate().altitude()));
}

void GPSPositionSourceTest::testUpdateFromSensorGps_3dFix()
{
    GPSPositionSource source;
    source.startUpdates();

    QSignalSpy spy(&source, &QGeoPositionInfoSource::positionUpdated);

    auto gps = _makeGpsData(sensor_gps_s::FIX_TYPE_3D, 47.123, 8.456, 550.0);
    source.updateFromSensorGps(gps);

    QCOMPARE(spy.count(), 1);

    const auto info = spy.at(0).at(0).value<QGeoPositionInfo>();
    QCOMPARE(info.coordinate().altitude(), 550.0);
}

void GPSPositionSourceTest::testUpdateFromSensorGps_attributes()
{
    GPSPositionSource source;
    source.startUpdates();

    QSignalSpy spy(&source, &QGeoPositionInfoSource::positionUpdated);

    auto gps = _makeGpsData(sensor_gps_s::FIX_TYPE_3D, 47.0, 8.0, 500.0);
    source.updateFromSensorGps(gps);

    QCOMPARE(spy.count(), 1);
    const auto info = spy.at(0).at(0).value<QGeoPositionInfo>();

    QVERIFY(info.hasAttribute(QGeoPositionInfo::HorizontalAccuracy));
    QCOMPARE(info.attribute(QGeoPositionInfo::HorizontalAccuracy), static_cast<qreal>(1.5));

    QVERIFY(info.hasAttribute(QGeoPositionInfo::VerticalAccuracy));
    QCOMPARE(info.attribute(QGeoPositionInfo::VerticalAccuracy), static_cast<qreal>(2.0));

    QVERIFY(info.hasAttribute(QGeoPositionInfo::GroundSpeed));
    QCOMPARE(info.attribute(QGeoPositionInfo::GroundSpeed), static_cast<qreal>(0.5));

    QVERIFY(info.hasAttribute(QGeoPositionInfo::Direction));
    QVERIFY(qAbs(info.attribute(QGeoPositionInfo::Direction) - 45.0) < 0.1);

    QVERIFY(info.hasAttribute(QGeoPositionInfo::DirectionAccuracy));
    QCOMPARE(info.attribute(QGeoPositionInfo::DirectionAccuracy), static_cast<qreal>(5.0));
}

void GPSPositionSourceTest::testUpdateWhileStopped()
{
    GPSPositionSource source;
    QSignalSpy spy(&source, &QGeoPositionInfoSource::positionUpdated);

    auto gps = _makeGpsData(sensor_gps_s::FIX_TYPE_3D, 47.0, 8.0, 500.0);
    source.updateFromSensorGps(gps);

    QCOMPARE(spy.count(), 0);
    QVERIFY(!source.lastKnownPosition().isValid());
}

void GPSPositionSourceTest::testRequestUpdate()
{
    GPSPositionSource source;
    source.startUpdates();

    QSignalSpy spy(&source, &QGeoPositionInfoSource::positionUpdated);

    // No prior data — requestUpdate should not emit
    source.requestUpdate();
    QCOMPARE(spy.count(), 0);

    // Feed data then request
    auto gps = _makeGpsData(sensor_gps_s::FIX_TYPE_3D, 47.0, 8.0, 500.0);
    source.updateFromSensorGps(gps);
    QCOMPARE(spy.count(), 1);

    source.requestUpdate();
    QCOMPARE(spy.count(), 2);
}

UT_REGISTER_TEST(GPSPositionSourceTest, TestLabel::Unit)
