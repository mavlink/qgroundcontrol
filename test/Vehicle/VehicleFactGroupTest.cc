#include "VehicleFactGroupTest.h"

#include "MAVLinkLib.h"
#include "Vehicle.h"
#include "VehicleFactGroup.h"

void VehicleFactGroupTest::_altitudeTuningSetpoint_data()
{
    QTest::addColumn<MAV_AUTOPILOT>("autopilot");
    QTest::addColumn<int>("majorVersion");
    QTest::addColumn<int>("minorVersion");
    QTest::addColumn<int>("patchVersion");
    QTest::addColumn<float>("altError");
    QTest::addColumn<double>("expectedSetpoint");

    constexpr MAV_AUTOPILOT px4 = MAV_AUTOPILOT_PX4;
    constexpr MAV_AUTOPILOT ardupilot = MAV_AUTOPILOT_ARDUPILOTMEGA;

    // altitudeTuning reads -5 m. Since 1.14, PX4 sends alt_error as setpoint minus current altitude.
    QTest::newRow("px4-1.14-below-setpoint") << px4 << 1 << 14 << 0 << 5.0f << 0.0;
    QTest::newRow("px4-1.14-above-setpoint") << px4 << 1 << 14 << 0 << -5.0f << -10.0;
    // PX4 1.13 and earlier send current altitude minus setpoint
    QTest::newRow("px4-1.13-below-setpoint") << px4 << 1 << 13 << 3 << -5.0f << 0.0;
    QTest::newRow("px4-1.13-above-setpoint") << px4 << 1 << 13 << 3 << 5.0f << -10.0;
    // Until AUTOPILOT_VERSION arrives, the current convention is assumed. The copy keeps operator<< from
    // binding a reference to versionNotSetValue, which has no out-of-line definition.
    const int notSet = Vehicle::versionNotSetValue;
    QTest::newRow("px4-version-not-set") << px4 << notSet << notSet << notSet << 5.0f << 0.0;
    // ArduPilot sends setpoint minus current altitude (Plane, QuadPlane, Sub, and Copter up to 4.6)
    QTest::newRow("ardupilot-below-setpoint") << ardupilot << 4 << 6 << 0 << 5.0f << 0.0;
    QTest::newRow("ardupilot-above-setpoint") << ardupilot << 4 << 6 << 0 << -5.0f << -10.0;
}

void VehicleFactGroupTest::_altitudeTuningSetpoint()
{
    QFETCH(MAV_AUTOPILOT, autopilot);
    QFETCH(int, majorVersion);
    QFETCH(int, minorVersion);
    QFETCH(int, patchVersion);
    QFETCH(float, altError);
    QFETCH(double, expectedSetpoint);

    if ((autopilot == MAV_AUTOPILOT_ARDUPILOTMEGA) && !apmFirmwareSupported()) {
        QSKIP("ArduPilot support not registered in this build");
    }
    _connectMockLink(autopilot);
    QVERIFY(vehicle());
    QCOMPARE(vehicle()->px4Firmware(), autopilot == MAV_AUTOPILOT_PX4);
    QCOMPARE(vehicle()->apmFirmware(), autopilot == MAV_AUTOPILOT_ARDUPILOTMEGA);
    vehicle()->setFirmwareVersion(majorVersion, minorVersion, patchVersion);

    // A separate fact group, so the vehicle's own telemetry from MockLink doesn't interfere
    VehicleFactGroup factGroup;
    mavlink_message_t message{};

    // the first VFR_HUD sets the offset that altitudeTuning is relative to
    for (const float alt : {100.0f, 95.0f}) {
        mavlink_vfr_hud_t vfrHud{};
        vfrHud.alt = alt;
        mavlink_msg_vfr_hud_encode(1, 1, &message, &vfrHud);
        factGroup.handleMessage(vehicle(), message);
    }
    QCOMPARE(factGroup.altitudeTuning()->rawValue().toDouble(), -5.0);

    mavlink_nav_controller_output_t navControllerOutput{};
    navControllerOutput.alt_error = altError;
    mavlink_msg_nav_controller_output_encode(1, 1, &message, &navControllerOutput);
    factGroup.handleMessage(vehicle(), message);
    QCOMPARE(factGroup.altitudeTuningSetpoint()->rawValue().toDouble(), expectedSetpoint);
}

UT_REGISTER_TEST(VehicleFactGroupTest, TestLabel::Integration, TestLabel::Vehicle)
