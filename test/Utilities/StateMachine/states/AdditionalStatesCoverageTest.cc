#include "AdditionalStatesCoverageTest.h"

#include "MAVLinkLib.h"
#include "SendMavlinkCommandState.h"
#include "SendMavlinkMessageState.h"
#include "StateTestCommon.h"
#include "WaitForMavlinkMessageState.h"

void AdditionalStatesCoverageTest::_testStatesConstructWithoutVehicle()
{
    // These states act on a vehicle once a machine enters them. Without one, this constructs each with its
    // optional arguments and destroys it with the machine.
    QStateMachine machine;

    new SendMavlinkCommandState(&machine, MAV_CMD_COMPONENT_ARM_DISARM, 1.0);
    auto* deferredCommandState = new SendMavlinkCommandState(&machine);
    deferredCommandState->setup(MAV_CMD_DO_SET_MODE, 1.0, 4.0);
    new SendMavlinkMessageState(&machine, [](quint32, uint8_t, mavlink_message_t*) {}, 3);
    new WaitForMavlinkMessageState(&machine, MAVLINK_MSG_ID_HEARTBEAT, 5000,
                                   [](const mavlink_message_t&) { return true; });

    QCOMPARE(machine.findChildren<QGCState*>().size(), 4);
}

UT_REGISTER_TEST(AdditionalStatesCoverageTest, TestLabel::Unit, TestLabel::Utilities)
