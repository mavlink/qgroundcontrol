#include "GuidedModeROITest.h"

#include <QtPositioning/QGeoCoordinate>

#include "MockLink.h"
#include "Vehicle.h"

namespace {
const QGeoCoordinate kRoiCoord(47.3977419, 8.5455938);
constexpr double kRelativeAltitudeMeters = 10.0;
}  // namespace

void GuidedModeROITest::_px4SendsAmslConvertedAltitude()
{
    QVERIFY(vehicle());

    // Home position arrives on MockLink's first 1Hz tick and is required for the PX4 AMSL conversion
    QVERIFY_TRUE_WAIT(vehicle()->homePosition().isValid() && !qIsNaN(vehicle()->homePosition().altitude()),
                      TestTimeout::shortMs());

    mockLink()->clearReceivedMavCommandCounts();
    QCOMPARE(vehicle()->roiRelativeAltitudeMeters(), 0.0);
    QVERIFY(vehicle()->guidedModeROI(kRoiCoord, kRelativeAltitudeMeters));
    // The last commanded altitude is remembered so ROI re-positioning can reuse it
    QCOMPARE(vehicle()->roiRelativeAltitudeMeters(), kRelativeAltitudeMeters);
    QVERIFY_TRUE_WAIT(mockLink()->receivedMavCommandCount(MAV_CMD_DO_SET_ROI_LOCATION) == 1, TestTimeout::longMs());

    mavlink_message_t message{};
    QVERIFY(mockLink()->lastReceivedMavlinkMessage(MAVLINK_MSG_ID_COMMAND_INT, message));
    mavlink_command_int_t command{};
    mavlink_msg_command_int_decode(&message, &command);
    QCOMPARE(command.command, static_cast<uint16_t>(MAV_CMD_DO_SET_ROI_LOCATION));
    QCOMPARE(command.frame, static_cast<uint8_t>(MAV_FRAME_GLOBAL));
    QCOMPARE(command.x, static_cast<int32_t>(kRoiCoord.latitude() * 1e7));
    QCOMPARE(command.y, static_cast<int32_t>(kRoiCoord.longitude() * 1e7));
    // PX4 treats the ROI altitude as AMSL, so the above-home value must be converted
    QCOMPARE(command.z, static_cast<float>(vehicle()->homePosition().altitude() + kRelativeAltitudeMeters));
}

void GuidedModeROIAPMTest::_apmSendsRelativeAltitude()
{
    QVERIFY(vehicle());

    mockLink()->clearReceivedMavCommandCounts();
    QVERIFY(vehicle()->guidedModeROI(kRoiCoord, kRelativeAltitudeMeters));
    QVERIFY_TRUE_WAIT(mockLink()->receivedMavCommandCount(MAV_CMD_DO_SET_ROI_LOCATION) == 1, TestTimeout::longMs());

    mavlink_message_t message{};
    QVERIFY(mockLink()->lastReceivedMavlinkMessage(MAVLINK_MSG_ID_COMMAND_INT, message));
    mavlink_command_int_t command{};
    mavlink_msg_command_int_decode(&message, &command);
    QCOMPARE(command.command, static_cast<uint16_t>(MAV_CMD_DO_SET_ROI_LOCATION));
    // ArduPilot honors the frame in the command, so the above-home value is sent unconverted
    QCOMPARE(command.frame, static_cast<uint8_t>(MAV_FRAME_GLOBAL_RELATIVE_ALT));
    QCOMPARE(command.x, static_cast<int32_t>(kRoiCoord.latitude() * 1e7));
    QCOMPARE(command.y, static_cast<int32_t>(kRoiCoord.longitude() * 1e7));
    QCOMPARE(command.z, static_cast<float>(kRelativeAltitudeMeters));
}

UT_REGISTER_TEST(GuidedModeROITest, TestLabel::Integration, TestLabel::Vehicle)
UT_REGISTER_TEST(GuidedModeROIAPMTest, TestLabel::Integration, TestLabel::Vehicle)
