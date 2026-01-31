#include "RequestMessageStateTest.h"
#include "StateTestCommon.h"

#include "RequestMessageState.h"


void RequestMessageStateTest::_testStateCreation()
{
    QStateMachine machine;

    // Create with just message ID
    auto* state = new RequestMessageState(&machine, MAVLINK_MSG_ID_HEARTBEAT);

    QVERIFY(state != nullptr);
    QCOMPARE(state->failureCode(), Vehicle::RequestMessageNoFailure);
    QCOMPARE(state->commandResult(), MAV_RESULT_ACCEPTED);
}

void RequestMessageStateTest::_testDefaultParameters()
{
    QStateMachine machine;

    // Create with message handler
    bool handlerCalled = false;
    auto handler = [&handlerCalled](Vehicle*, const mavlink_message_t&) {
        handlerCalled = true;
    };

    auto* state = new RequestMessageState(&machine, MAVLINK_MSG_ID_AUTOPILOT_VERSION, handler);

    QVERIFY(state != nullptr);
    // Handler will be called when message is received (requires vehicle)
}

void RequestMessageStateTest::_testTimeoutConfiguration()
{
    QStateMachine machine;

    // Create with custom timeout
    auto* state = new RequestMessageState(
        &machine,
        MAVLINK_MSG_ID_HEARTBEAT,
        RequestMessageState::MessageHandler(),
        MAV_COMP_ID_AUTOPILOT1,
        10000  // 10 second timeout
    );

    QVERIFY(state != nullptr);
}
