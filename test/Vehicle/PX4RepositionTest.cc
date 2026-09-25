#include "PX4RepositionTest.h"

#include <QtCore/QtMath>
#include <QtPositioning/QGeoCoordinate>

#include "MockLink.h"
#include "Vehicle.h"

namespace {
constexpr double kAltitudeChange = 5.0;
}  // namespace

void PX4RepositionTest::_verifyRepositionCommandInt(float yaw, float amslAltitude)
{
    mavlink_message_t message{};
    QVERIFY(mockLink()->lastReceivedMavlinkMessage(MAVLINK_MSG_ID_COMMAND_INT, message));
    mavlink_command_int_t command{};
    mavlink_msg_command_int_decode(&message, &command);
    QCOMPARE(command.command, static_cast<uint16_t>(MAV_CMD_DO_REPOSITION));
    QCOMPARE(command.frame, static_cast<uint8_t>(MAV_FRAME_GLOBAL));
    QCOMPARE(command.param1, -1.0f);  // no ground speed change
    QCOMPARE(command.param2, static_cast<float>(MAV_DO_REPOSITION_FLAGS_CHANGE_MODE));
    QCOMPARE(command.param4, yaw);
    QCOMPARE(command.x, INT32_MAX);
    QCOMPARE(command.y, INT32_MAX);
    QCOMPARE(command.z, amslAltitude);
}

double PX4RepositionTest::_changedAmslAltitude(double altitudeChange) const
{
    return vehicle()->homePosition().altitude() + vehicle()->altitudeRelative()->rawValue().toDouble() + altitudeChange;
}

void PX4RepositionTest::_noChangeLatLonSentAsInt32Max()
{
    QVERIFY(vehicle());

    mockLink()->clearReceivedMavCommandCounts();
    mockLink()->clearReceivedMavlinkMessageCounts();
    vehicle()->sendMavCommandInt(vehicle()->defaultComponentId(), MAV_CMD_DO_REPOSITION, MAV_FRAME_GLOBAL,
                                 false,  // showError
                                 -1.0f, MAV_DO_REPOSITION_FLAGS_CHANGE_MODE, 0.0f, qQNaN(), qQNaN(), qQNaN(), qQNaN());
    QVERIFY_TRUE_WAIT(mockLink()->receivedMavCommandCount(MAV_CMD_DO_REPOSITION) == 1, TestTimeout::longMs());

    mavlink_message_t message{};
    QVERIFY(mockLink()->lastReceivedMavlinkMessage(MAVLINK_MSG_ID_COMMAND_INT, message));
    mavlink_command_int_t command{};
    mavlink_msg_command_int_decode(&message, &command);
    QCOMPARE(command.command, static_cast<uint16_t>(MAV_CMD_DO_REPOSITION));
    // NaN has no integer form, so COMMAND_INT marks an unused x/y with INT32_MAX
    QCOMPARE(command.x, INT32_MAX);
    QCOMPARE(command.y, INT32_MAX);
    QVERIFY(qIsNaN(command.z));
}

void PX4RepositionTest::_pauseSendsCommandInt()
{
    QVERIFY(vehicle());

    mockLink()->clearReceivedMavCommandCounts();
    mockLink()->clearReceivedMavlinkMessageCounts();
    vehicle()->pauseVehicle();
    QVERIFY_TRUE_WAIT(mockLink()->receivedMavCommandCount(MAV_CMD_DO_REPOSITION) == 1, TestTimeout::longMs());
    _verifyRepositionCommandInt(qQNaN(), qQNaN());
}

void PX4RepositionTest::_changeHeadingSendsCommandInt()
{
    QVERIFY(vehicle());
    QVERIFY_TRUE_WAIT(vehicle()->coordinate().isValid(), TestTimeout::shortMs());

    const QGeoCoordinate headingCoord = vehicle()->coordinate().atDistanceAndAzimuth(100, 90);
    const float expectedYaw = qDegreesToRadians(vehicle()->coordinate().azimuthTo(headingCoord));

    mockLink()->clearReceivedMavCommandCounts();
    mockLink()->clearReceivedMavlinkMessageCounts();
    vehicle()->guidedModeChangeHeading(headingCoord);
    QVERIFY_TRUE_WAIT(mockLink()->receivedMavCommandCount(MAV_CMD_DO_REPOSITION) == 1, TestTimeout::longMs());
    _verifyRepositionCommandInt(expectedYaw, qQNaN());
}

void PX4RepositionTest::_changeAltitudeSendsCommandInt()
{
    QVERIFY(vehicle());
    // Home position arrives on MockLink's first 1Hz tick and is required to compute the AMSL target
    QVERIFY_TRUE_WAIT(vehicle()->homePosition().isValid() && !qIsNaN(vehicle()->homePosition().altitude()),
                      TestTimeout::shortMs());

    const double expectedAltitude = _changedAmslAltitude(kAltitudeChange);

    mockLink()->clearReceivedMavCommandCounts();
    mockLink()->clearReceivedMavlinkMessageCounts();
    vehicle()->guidedModeChangeAltitude(kAltitudeChange, false /* pauseVehicle */);
    QVERIFY_TRUE_WAIT(mockLink()->receivedMavCommandCount(MAV_CMD_DO_REPOSITION) == 1, TestTimeout::longMs());
    _verifyRepositionCommandInt(qQNaN(), static_cast<float>(expectedAltitude));
}

void PX4RepositionTest::_changeAltitudeAfterPauseSendsCommandInt()
{
    QVERIFY(vehicle());
    QVERIFY_TRUE_WAIT(vehicle()->homePosition().isValid() && !qIsNaN(vehicle()->homePosition().altitude()),
                      TestTimeout::shortMs());

    const double expectedAltitude = _changedAmslAltitude(kAltitudeChange);

    mockLink()->clearReceivedMavCommandCounts();
    mockLink()->clearReceivedMavlinkMessageCounts();
    vehicle()->guidedModeChangeAltitude(kAltitudeChange, true /* pauseVehicle */);
    // The pause is sent first and the altitude change follows once it is accepted. MockLink only
    // accepts DO_REPOSITION as COMMAND_INT, so the second command also proves how the pause was sent.
    QVERIFY_TRUE_WAIT(mockLink()->receivedMavCommandCount(MAV_CMD_DO_REPOSITION) == 2, TestTimeout::longMs());
    _verifyRepositionCommandInt(qQNaN(), static_cast<float>(expectedAltitude));
}

UT_REGISTER_TEST(PX4RepositionTest, TestLabel::Integration, TestLabel::Vehicle)
