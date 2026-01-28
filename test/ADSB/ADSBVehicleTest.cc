#include "ADSBVehicleTest.h"
#include "TestHelpers.h"
#include "ADSBVehicle.h"

#include <QtTest/QTest>
#include <QtTest/QSignalSpy>

// ============================================================================
// Helper Methods
// ============================================================================

ADSB::VehicleInfo_t ADSBVehicleTest::_createVehicleInfo(
    uint32_t icaoAddress,
    const QString &callsign,
    const QGeoCoordinate &location,
    ADSB::AvailableInfoTypes flags)
{
    ADSB::VehicleInfo_t info{};
    info.icaoAddress = icaoAddress;
    info.callsign = callsign.isEmpty() ? QString::number(icaoAddress, 16).toUpper() : callsign;
    info.location = location.isValid() ? location : QGeoCoordinate(1.0, 1.0, 100.0);
    info.heading = 90.0;
    info.velocity = 250.0;
    info.alert = false;
    info.availableFlags = flags;
    return info;
}

// ============================================================================
// Constructor Tests
// ============================================================================

void ADSBVehicleTest::_constructorTest()
{
    const auto info = _createVehicleInfo(
        0x123456, "TEST1", QGeoCoordinate(10.0, 20.0, 500.0),
        ADSB::CallsignAvailable | ADSB::LocationAvailable | ADSB::AltitudeAvailable);

    ADSBVehicle vehicle(info, this);

    QCOMPARE_EQ(vehicle.icaoAddress(), info.icaoAddress);
    QCOMPARE_EQ(vehicle.callsign(), info.callsign);
    QCOMPARE_EQ(vehicle.coordinate(), info.location);
    QCOMPARE_EQ(vehicle.altitude(), info.location.altitude());
    QVERIFY(!vehicle.expired());
}

// ============================================================================
// Update Tests
// ============================================================================

void ADSBVehicleTest::_updateSameIcaoTest()
{
    const auto info1 = _createVehicleInfo(
        0x100, "INITIAL", QGeoCoordinate(1.0, 1.0, 1000.0),
        ADSB::CallsignAvailable | ADSB::LocationAvailable);

    ADSBVehicle vehicle(info1, this);
    QCOMPARE_EQ(vehicle.callsign(), "INITIAL");

    // Update with same ICAO - should update
    const auto info2 = _createVehicleInfo(
        0x100, "UPDATED", QGeoCoordinate(2.0, 2.0, 2000.0),
        ADSB::CallsignAvailable | ADSB::LocationAvailable | ADSB::AltitudeAvailable);

    QSignalSpy callsignSpy(&vehicle, &ADSBVehicle::callsignChanged);
    QSignalSpy coordinateSpy(&vehicle, &ADSBVehicle::coordinateChanged);
    QSignalSpy altitudeSpy(&vehicle, &ADSBVehicle::altitudeChanged);

    vehicle.update(info2);

    QCOMPARE_EQ(vehicle.callsign(), "UPDATED");
    QCOMPARE_EQ(vehicle.coordinate().latitude(), info2.location.latitude());
    QCOMPARE_EQ(vehicle.coordinate().longitude(), info2.location.longitude());
    QCOMPARE_EQ(vehicle.altitude(), info2.location.altitude());
    QCOMPARE_EQ(callsignSpy.count(), 1);
    QCOMPARE_EQ(coordinateSpy.count(), 1);
    QCOMPARE_EQ(altitudeSpy.count(), 1);
}

void ADSBVehicleTest::_updateDifferentIcaoIgnoredTest()
{
    const auto info1 = _createVehicleInfo(
        0x100, "ORIGINAL", QGeoCoordinate(1.0, 1.0, 1000.0),
        ADSB::CallsignAvailable | ADSB::LocationAvailable);

    ADSBVehicle vehicle(info1, this);

    // Update with different ICAO - should be ignored
    const auto info2 = _createVehicleInfo(
        0x200, "DIFFERENT", QGeoCoordinate(9.0, 9.0, 9000.0),
        ADSB::CallsignAvailable | ADSB::LocationAvailable);

    QSignalSpy callsignSpy(&vehicle, &ADSBVehicle::callsignChanged);

    vehicle.update(info2);

    // Should NOT update - ICAO mismatch
    QCOMPARE_EQ(vehicle.callsign(), "ORIGINAL");
    QCOMPARE_EQ(vehicle.icaoAddress(), static_cast<uint32_t>(0x100));
    QCOMPARE_EQ(callsignSpy.count(), 0);
}

// ============================================================================
// Expiration Tests
// ============================================================================

void ADSBVehicleTest::_expirationTest()
{
    const auto info = _createVehicleInfo(0x123, "EXPIRE_TEST", QGeoCoordinate(),
                                         ADSB::CallsignAvailable);

    ADSBVehicle vehicle(info, this);
    QVERIFY(!vehicle.expired());

    // Wait for expiration (ADSBVehicle expires after 120 seconds by default)
    // We can't wait that long in a test, so just verify the method exists
    // and returns false initially
}

// ============================================================================
// Available Flags Tests
// ============================================================================

void ADSBVehicleTest::_availableFlagsTest()
{
    // Test with only callsign flag - location should NOT be set
    const auto callsignOnly = _createVehicleInfo(
        0x100, "CALLSIGN", QGeoCoordinate(50.0, 50.0, 5000.0),
        ADSB::CallsignAvailable);

    ADSBVehicle vehicle1(callsignOnly, this);
    QCOMPARE_EQ(vehicle1.callsign(), "CALLSIGN");
    // Location should be invalid since LocationAvailable wasn't set
    QVERIFY(!vehicle1.coordinate().isValid() || vehicle1.coordinate() != callsignOnly.location);

    // Test with location flag - location should be set
    const auto withLocation = _createVehicleInfo(
        0x200, "LOCATED", QGeoCoordinate(51.5, -0.1, 10000.0),
        ADSB::CallsignAvailable | ADSB::LocationAvailable | ADSB::AltitudeAvailable);

    ADSBVehicle vehicle2(withLocation, this);
    QCOMPARE_EQ(vehicle2.coordinate(), withLocation.location);

    // Test heading flag
    const auto withHeading = _createVehicleInfo(
        0x300, "HEADING", QGeoCoordinate(40.0, -74.0, 8000.0),
        ADSB::CallsignAvailable | ADSB::LocationAvailable | ADSB::HeadingAvailable);

    ADSBVehicle vehicle3(withHeading, this);
    QCOMPARE_EQ(vehicle3.heading(), withHeading.heading);
}
