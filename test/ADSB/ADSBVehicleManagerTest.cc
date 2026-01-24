#include "ADSBVehicleManagerTest.h"
#include "TestHelpers.h"
#include "ADSBVehicleManager.h"
#include "ADSBVehicle.h"
#include "QmlObjectListModel.h"
#include "MAVLinkLib.h"

#include <QtTest/QTest>

// ============================================================================
// Helper Methods
// ============================================================================

ADSB::VehicleInfo_t ADSBVehicleManagerTest::_createVehicleInfo(
    uint32_t icaoAddress,
    const QString &callsign,
    const QGeoCoordinate &location,
    ADSB::AvailableInfoTypes flags)
{
    ADSB::VehicleInfo_t info{};
    info.icaoAddress = icaoAddress;
    info.callsign = callsign.isEmpty() ? QString::number(icaoAddress, 16).toUpper() : callsign;
    info.location = location.isValid() ? location : QGeoCoordinate(51.5, -0.1, 10000.0);
    info.heading = 90.0;
    info.velocity = 250.0;
    info.alert = false;
    info.availableFlags = flags;
    return info;
}

// ============================================================================
// Instance Tests
// ============================================================================

void ADSBVehicleManagerTest::_instanceTest()
{
    ADSBVehicleManager* manager = ADSBVehicleManager::instance();
    VERIFY_NOT_NULL(manager);
    VERIFY_NOT_NULL(manager->adsbVehicles());

    // Verify singleton returns same instance
    QCOMPARE_EQ(ADSBVehicleManager::instance(), manager);
}

// ============================================================================
// Vehicle Management Tests
// ============================================================================

void ADSBVehicleManagerTest::_addVehicleTest()
{
    ADSBVehicleManager* manager = ADSBVehicleManager::instance();
    const int initialCount = manager->adsbVehicles()->count();

    // Use unique ICAO to avoid conflicts with other tests
    const uint32_t testIcao = 0xF00001 + static_cast<uint32_t>(initialCount);
    const auto info = _createVehicleInfo(testIcao, "ADD_TEST", QGeoCoordinate(52.0, 0.0, 8000.0));

    manager->adsbVehicleUpdate(info);

    QCOMPARE_EQ(manager->adsbVehicles()->count(), initialCount + 1);

    // Verify the vehicle was added with correct data
    ADSBVehicle* vehicle = manager->adsbVehicles()->value<ADSBVehicle*>(initialCount);
    VERIFY_NOT_NULL(vehicle);
    QCOMPARE_EQ(vehicle->icaoAddress(), testIcao);
    QCOMPARE_EQ(vehicle->callsign(), "ADD_TEST");
}

void ADSBVehicleManagerTest::_updateExistingVehicleTest()
{
    ADSBVehicleManager* manager = ADSBVehicleManager::instance();
    const int initialCount = manager->adsbVehicles()->count();

    // Add a vehicle
    const uint32_t testIcao = 0xF00100 + static_cast<uint32_t>(initialCount);
    const auto info1 = _createVehicleInfo(testIcao, "UPDATE_V1", QGeoCoordinate(51.5, -0.1, 10000.0));
    manager->adsbVehicleUpdate(info1);

    const int countAfterAdd = manager->adsbVehicles()->count();
    QCOMPARE_EQ(countAfterAdd, initialCount + 1);

    // Update same vehicle (same ICAO)
    auto info2 = info1;
    info2.callsign = "UPDATE_V2";
    info2.location = QGeoCoordinate(51.6, -0.2, 11000.0);
    manager->adsbVehicleUpdate(info2);

    // Count should remain the same (update, not add)
    QCOMPARE_EQ(manager->adsbVehicles()->count(), countAfterAdd);
}

// ============================================================================
// MAVLink Message Tests
// ============================================================================

void ADSBVehicleManagerTest::_mavlinkMessageTest()
{
    ADSBVehicleManager* manager = ADSBVehicleManager::instance();
    const int initialCount = manager->adsbVehicles()->count();

    // Create a MAVLink ADSB_VEHICLE message
    mavlink_message_t msg{};
    mavlink_adsb_vehicle_t adsb{};

    adsb.ICAO_address = 0xF00200 + static_cast<uint32_t>(initialCount);
    adsb.lat = static_cast<int32_t>(51.5 * 1e7);
    adsb.lon = static_cast<int32_t>(-0.1 * 1e7);
    adsb.altitude = 10000 * 1000;  // mm
    adsb.heading = 9000;  // centidegrees
    adsb.hor_velocity = 25000;  // cm/s
    adsb.ver_velocity = 0;
    adsb.flags = ADSB_FLAGS_VALID_COORDS | ADSB_FLAGS_VALID_ALTITUDE | ADSB_FLAGS_VALID_HEADING;
    adsb.squawk = 1234;
    adsb.altitude_type = ADSB_ALTITUDE_TYPE_PRESSURE_QNH;
    adsb.emitter_type = ADSB_EMITTER_TYPE_LARGE;
    adsb.tslc = 1;  // 1 second since last contact
    memcpy(adsb.callsign, "MAVTEST ", 8);

    mavlink_msg_adsb_vehicle_encode(1, 1, &msg, &adsb);

    manager->mavlinkMessageReceived(msg);

    // Vehicle should be added
    QCOMPARE_EQ(manager->adsbVehicles()->count(), initialCount + 1);
}
