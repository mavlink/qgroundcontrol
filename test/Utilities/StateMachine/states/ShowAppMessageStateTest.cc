#include "ShowAppMessageStateTest.h"
#include "StateTestCommon.h"


void ShowAppMessageStateTest::_testShowAppMessageStateCreation()
{
    QStateMachine machine;

    auto* state = new ShowAppMessageState(&machine, QStringLiteral("Test message"));

    QVERIFY(state != nullptr);
    // ShowAppMessageState is a QGCState, so it should have advance/error signals
}

void ShowAppMessageStateTest::_testShowAppMessageStateAdvance()
{
    QStateMachine machine;

    auto* state = new ShowAppMessageState(&machine, QStringLiteral("Test message"));
    auto* finalState = new QFinalState(&machine);

    state->addTransition(state, &QGCState::advance, finalState);
    machine.setInitialState(state);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    // ShowAppMessageState should emit advance() after showing the message
    QVERIFY(finishedSpy.wait(500));
}
