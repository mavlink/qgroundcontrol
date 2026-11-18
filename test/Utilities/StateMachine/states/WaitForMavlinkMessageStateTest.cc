#include "WaitForMavlinkMessageStateTest.h"
#include "MAVLinkLib.h"
#include "StateTestCommon.h"

#include "WaitForMavlinkMessageState.h"


void WaitForMavlinkMessageStateTest::_testMessageId()
{
    QStateMachine machine;

    auto* state = new WaitForMavlinkMessageState(
        &machine,
        MAVLINK_MSG_ID_AUTOPILOT_VERSION,
        0  // No timeout
    );

    QCOMPARE(state->messageId(), static_cast<uint32_t>(MAVLINK_MSG_ID_AUTOPILOT_VERSION));
}

UT_REGISTER_TEST(WaitForMavlinkMessageStateTest, TestLabel::Unit, TestLabel::Utilities)
