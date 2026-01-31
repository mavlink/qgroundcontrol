#include "TimeoutTransitionTest.h"
#include "TransitionTestCommon.h"

void TimeoutTransitionTest::_testTimeoutFires()
{
    QStateMachine machine;
    bool timeoutReached = false;

    auto* startState = new QState(&machine);
    auto* timeoutState = new FunctionState(QStringLiteral("Timeout"), &machine, [&timeoutReached]() {
        timeoutReached = true;
    });
    auto* finalState = new QFinalState(&machine);

    // Timeout after 50ms
    auto* timeoutTransition = new TimeoutTransition(50, timeoutState);
    startState->addTransition(timeoutTransition);
    timeoutState->addTransition(timeoutState, &QGCState::advance, finalState);
    machine.setInitialState(startState);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    QVERIFY(finishedSpy.wait(500));
    QVERIFY(timeoutReached);
}

void TimeoutTransitionTest::_testTimeoutCancelledOnExit()
{
    QStateMachine machine;
    bool timeoutReached = false;
    bool normalExitReached = false;

    auto* startState = new FunctionState(QStringLiteral("Start"), &machine, []() {});
    auto* timeoutState = new FunctionState(QStringLiteral("Timeout"), &machine, [&timeoutReached]() {
        timeoutReached = true;
    });
    auto* normalState = new FunctionState(QStringLiteral("Normal"), &machine, [&normalExitReached]() {
        normalExitReached = true;
    });
    auto* finalState = new QFinalState(&machine);

    // Long timeout that shouldn't fire
    auto* timeoutTransition = new TimeoutTransition(500, timeoutState);
    startState->addTransition(timeoutTransition);

    // Normal transition that fires immediately (FunctionState emits advance on entry)
    startState->addTransition(startState, &QGCState::advance, normalState);

    timeoutState->addTransition(timeoutState, &QGCState::advance, finalState);
    normalState->addTransition(normalState, &QGCState::advance, finalState);
    machine.setInitialState(startState);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    QVERIFY(finishedSpy.wait(200));
    QVERIFY(normalExitReached);   // Normal path taken
    QVERIFY(!timeoutReached);     // Timeout never fired
    QVERIFY(!timeoutTransition->isTimerActive());  // Timer stopped
}

void TimeoutTransitionTest::_testTimeoutRestartsOnReentry()
{
    QStateMachine machine;
    int timeoutCount = 0;

    auto* waitState = new QState(&machine);
    auto* countState = new FunctionState(QStringLiteral("Count"), &machine, [&timeoutCount]() {
        timeoutCount++;
    });
    auto* finalState = new QFinalState(&machine);

    // Timeout after 30ms
    auto* timeoutTransition = new TimeoutTransition(30, countState);
    waitState->addTransition(timeoutTransition);

    // Go back to wait state if count < 2, otherwise to final
    countState->addTransition(countState, &QGCState::advance, waitState);

    // Add a guarded transition to final when count reaches 2
    auto* toFinal = new GuardedTransition(countState, &QGCState::advance, finalState,
        [&timeoutCount]() { return timeoutCount >= 2; });
    countState->addTransition(toFinal);

    machine.setInitialState(waitState);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    QVERIFY(finishedSpy.wait(500));
    QCOMPARE(timeoutCount, 2);  // Timeout fired twice (timer restarted on reentry)
}

void TimeoutTransitionTest::_testMultipleTimeoutTransitions()
{
    QStateMachine machine;
    bool shortTimeoutReached = false;
    bool longTimeoutReached = false;

    auto* startState = new QState(&machine);
    auto* shortState = new FunctionState(QStringLiteral("Short"), &machine, [&shortTimeoutReached]() {
        shortTimeoutReached = true;
    });
    auto* longState = new FunctionState(QStringLiteral("Long"), &machine, [&longTimeoutReached]() {
        longTimeoutReached = true;
    });
    auto* finalState = new QFinalState(&machine);

    // Short timeout should win
    auto* shortTimeout = new TimeoutTransition(30, shortState);
    auto* longTimeout = new TimeoutTransition(200, longState);

    startState->addTransition(shortTimeout);
    startState->addTransition(longTimeout);

    shortState->addTransition(shortState, &QGCState::advance, finalState);
    longState->addTransition(longState, &QGCState::advance, finalState);
    machine.setInitialState(startState);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    QVERIFY(finishedSpy.wait(500));
    QVERIFY(shortTimeoutReached);   // Short timeout fired first
    QVERIFY(!longTimeoutReached);   // Long timeout never reached
}
