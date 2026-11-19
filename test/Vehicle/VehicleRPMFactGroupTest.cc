#include "VehicleRPMFactGroupTest.h"

#include "MAVLinkLib.h"
#include "VehicleRPMFactGroup.h"

namespace {

mavlink_message_t rpmSensorMessage(float rpm1, float rpm2)
{
    mavlink_message_t message{};
    mavlink_msg_rpm_pack(1, 1, &message, rpm1, rpm2);
    return message;
}

}  // namespace

void VehicleRPMFactGroupTest::_validValuesPassThrough()
{
    VehicleRPMFactGroup rpm;
    QVERIFY(!rpm.telemetryAvailable());
    QVERIFY(qIsNaN(rpm.rpmSensor1()->rawValue().toDouble()));

    rpm.handleMessage(nullptr, rpmSensorMessage(2200.0F, 1100.0F));

    QVERIFY(rpm.telemetryAvailable());
    QCOMPARE(rpm.rpmSensor1()->rawValue().toDouble(), 2200.0);
    QCOMPARE(rpm.rpmSensor2()->rawValue().toDouble(), 1100.0);
}

void VehicleRPMFactGroupTest::_negativeRpmIsNaN()
{
    VehicleRPMFactGroup rpm;
    rpm.handleMessage(nullptr, rpmSensorMessage(2200.0F, 1100.0F));

    // ArduPilot sends -1 for an unconfigured or unhealthy sensor
    rpm.handleMessage(nullptr, rpmSensorMessage(2200.0F, -1.0F));

    QCOMPARE(rpm.rpmSensor1()->rawValue().toDouble(), 2200.0);
    QVERIFY(qIsNaN(rpm.rpmSensor2()->rawValue().toDouble()));
}

void VehicleRPMFactGroupTest::_zeroRpmStaysZero()
{
    VehicleRPMFactGroup rpm;
    rpm.handleMessage(nullptr, rpmSensorMessage(0.0F, 0.0F));

    QCOMPARE(rpm.rpmSensor1()->rawValue().toDouble(), 0.0);
    QCOMPARE(rpm.rpmSensor2()->rawValue().toDouble(), 0.0);
}

UT_REGISTER_TEST(VehicleRPMFactGroupTest, TestLabel::Unit, TestLabel::Vehicle)
