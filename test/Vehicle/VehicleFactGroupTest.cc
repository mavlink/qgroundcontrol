#include "VehicleFactGroupTest.h"

#include "MAVLinkLib.h"
#include "VehicleFactGroup.h"

void VehicleFactGroupTest::_altitudeTuningSetpoint_data()
{
    QTest::addColumn<float>("altError");
    QTest::addColumn<double>("expectedSetpoint");

    // altitudeTuning reads -5 m, and NAV_CONTROLLER_OUTPUT.alt_error is setpoint minus current altitude
    QTest::newRow("below-setpoint") << 5.0f << 0.0;
    QTest::newRow("above-setpoint") << -5.0f << -10.0;
}

void VehicleFactGroupTest::_altitudeTuningSetpoint()
{
    QFETCH(float, altError);
    QFETCH(double, expectedSetpoint);

    VehicleFactGroup factGroup;
    mavlink_message_t message{};

    // the first VFR_HUD sets the offset that altitudeTuning is relative to
    for (const float alt : {100.0f, 95.0f}) {
        mavlink_vfr_hud_t vfrHud{};
        vfrHud.alt = alt;
        mavlink_msg_vfr_hud_encode(1, 1, &message, &vfrHud);
        factGroup.handleMessage(nullptr, message);
    }
    QCOMPARE(factGroup.altitudeTuning()->rawValue().toDouble(), -5.0);

    mavlink_nav_controller_output_t navControllerOutput{};
    navControllerOutput.alt_error = altError;
    mavlink_msg_nav_controller_output_encode(1, 1, &message, &navControllerOutput);
    factGroup.handleMessage(nullptr, message);
    QCOMPARE(factGroup.altitudeTuningSetpoint()->rawValue().toDouble(), expectedSetpoint);
}

UT_REGISTER_TEST(VehicleFactGroupTest, TestLabel::Unit, TestLabel::Vehicle)
