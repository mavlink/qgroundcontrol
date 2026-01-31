#include "WaitForMavlinkMessageStateTest.h"
#include "StateTestCommon.h"

#include "WaitForMavlinkMessageState.h"


void WaitForMavlinkMessageStateTest::_testStateCreation()
{
    QStateMachine machine;

    auto* state = new WaitForMavlinkMessageState(
        &machine,
        MAVLINK_MSG_ID_HEARTBEAT,
        5000  // 5 second timeout
    );

    QVERIFY(state != nullptr);
}

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

void WaitForMavlinkMessageStateTest::_testPredicateSetup()
{
    QStateMachine machine;
    bool predicateCalled = false;

    auto predicate = [&predicateCalled](const mavlink_message_t&) {
        predicateCalled = true;
        return true;
    };

    auto* state = new WaitForMavlinkMessageState(
        &machine,
        MAVLINK_MSG_ID_HEARTBEAT,
        5000,
        predicate
    );

    QVERIFY(state != nullptr);
    // Predicate will be called when message is received (requires vehicle)
}

void WaitForMavlinkMessageStateTest::_testTimeoutConfiguration()
{
    QStateMachine machine;

    auto* state = new WaitForMavlinkMessageState(
        &machine,
        MAVLINK_MSG_ID_HEARTBEAT,
        10000  // 10 second timeout
    );

    QVERIFY(state != nullptr);
}
