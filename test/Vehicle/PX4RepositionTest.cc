#include "PX4RepositionTest.h"

#include "MockLink.h"
#include "Vehicle.h"

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

UT_REGISTER_TEST(PX4RepositionTest, TestLabel::Integration, TestLabel::Vehicle)
