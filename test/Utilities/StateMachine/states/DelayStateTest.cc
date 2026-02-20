#include "DelayStateTest.h"
#include "StateTestCommon.h"


void DelayStateTest::_testDelayState()
{
    QStateMachine machine;
    const int delayMs = 100;

    auto* delayState = new DelayState(&machine, delayMs);
    auto* finalState = new QFinalState(&machine);

    delayState->addTransition(delayState, &DelayState::delayComplete, finalState);
    machine.setInitialState(delayState);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    QElapsedTimer timer;
    timer.start();

    machine.start();

    QVERIFY(finishedSpy.wait(500));
    QVERIFY(timer.elapsed() >= delayMs - 10);
}

UT_REGISTER_TEST(DelayStateTest, TestLabel::Unit, TestLabel::Utilities)
