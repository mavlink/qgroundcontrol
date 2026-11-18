#include "SendMavlinkCommandStateTest.h"
#include "StateTestCommon.h"

#include <QtCore/QRegularExpression>

#include "SendMavlinkCommandState.h"


void SendMavlinkCommandStateTest::_testUnconfiguredStateFails()
{
    QStateMachine machine;

    // Expected: unconfigured state emits critical + Qt warns about null connect
    expectLogMessage("qt.core.qobject.connect", QtWarningMsg, QRegularExpression("invalid nullptr parameter"));
    expectLogMessage("Utilities.QGCStateMachine", QtCriticalMsg, QRegularExpression("SendMavlinkCommandState not configured"));

    auto* state = new SendMavlinkCommandState(&machine);
    auto* errorState = new FunctionState(QStringLiteral("Error"), &machine, []() {});
    auto* finalState = addFinalState(&machine);

    state->addTransition(state, &QGCState::error, errorState);
    errorState->addTransition(errorState, &QGCState::advance, finalState);

    machine.setInitialState(state);

    QSignalSpy errorSpy(state, &QGCState::error);

    // Should fail immediately because not configured and no vehicle
    QVERIFY(startAndWaitForFinished(&machine));
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    QCOMPARE(errorSpy.count(), 1);
}

UT_REGISTER_TEST(SendMavlinkCommandStateTest, TestLabel::Unit, TestLabel::Utilities)
