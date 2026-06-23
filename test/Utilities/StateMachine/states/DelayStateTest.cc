#include "DelayStateTest.h"
#include "StateTestCommon.h"


void DelayStateTest::_testDelayState()
{
    QStateMachine machine;
    const int delayMs = 100;

    auto* delayState = new DelayState(&machine, delayMs);
    auto* finalState = addFinalState(&machine);

    delayState->addTransition(delayState, &DelayState::delayComplete, finalState);
    machine.setInitialState(delayState);

    QElapsedTimer timer;
    timer.start();

    QVERIFY(startAndWaitForFinished(&machine));
    QVERIFY(timer.elapsed() >= delayMs - 10);
}

UT_REGISTER_TEST(DelayStateTest, TestLabel::Unit, TestLabel::Utilities)
