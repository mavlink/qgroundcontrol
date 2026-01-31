#include "SendMavlinkCommandStateTest.h"
#include "StateTestCommon.h"

#include "SendMavlinkCommandState.h"


void SendMavlinkCommandStateTest::_testStateCreation()
{
    QStateMachine machine;

    // Create with command parameters
    auto* state = new SendMavlinkCommandState(
        &machine,
        MAV_CMD_COMPONENT_ARM_DISARM,
        1.0,  // param1: arm
        0.0,  // param2
        0.0,  // param3
        0.0,  // param4
        0.0,  // param5
        0.0,  // param6
        0.0   // param7
    );

    QVERIFY(state != nullptr);
}

void SendMavlinkCommandStateTest::_testDeferredSetup()
{
    QStateMachine machine;

    // Create without parameters
    auto* state = new SendMavlinkCommandState(&machine);
    QVERIFY(state != nullptr);

    // Setup later
    state->setup(MAV_CMD_DO_SET_MODE, 1.0, 4.0);  // Custom mode
}

void SendMavlinkCommandStateTest::_testUnconfiguredStateFails()
{
    QStateMachine machine;

    // Create without configuration
    auto* state = new SendMavlinkCommandState(&machine);
    auto* errorState = new FunctionState(QStringLiteral("Error"), &machine, []() {});
    auto* finalState = new QFinalState(&machine);

    state->addTransition(state, &QGCState::error, errorState);
    errorState->addTransition(errorState, &QGCState::advance, finalState);

    machine.setInitialState(state);

    QSignalSpy errorSpy(state, &QGCState::error);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);

    machine.start();

    // Should fail immediately because not configured and no vehicle
    QVERIFY(finishedSpy.wait(500));
    QCOMPARE(errorSpy.count(), 1);
}
