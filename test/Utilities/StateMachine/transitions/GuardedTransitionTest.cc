#include "GuardedTransitionTest.h"
#include "TransitionTestCommon.h"


void GuardedTransitionTest::_testGuardedTransitionAllowed()
{
    QStateMachine machine;
    bool transitionTaken = false;
    bool guardCalled = false;

    auto* startState = new FunctionState(QStringLiteral("Start"), &machine, []() {});
    auto* targetState = new FunctionState(QStringLiteral("Target"), &machine, [&transitionTaken]() {
        transitionTaken = true;
    });
    auto* finalState = new QFinalState(&machine);

    auto* guardedTransition = new GuardedTransition(
        startState, &QGCState::advance,
        targetState,
        [&guardCalled]() {
            guardCalled = true;
            return true;
        }
    );
    startState->addTransition(guardedTransition);
    targetState->addTransition(targetState, &QGCState::advance, finalState);
    machine.setInitialState(startState);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    QVERIFY(finishedSpy.wait(500));
    QVERIFY(guardCalled);
    QVERIFY(transitionTaken);
}

void GuardedTransitionTest::_testGuardedTransitionBlocked()
{
    QStateMachine machine;
    bool guardCalled = false;
    bool targetReached = false;
    bool alternateReached = false;

    auto* startState = new FunctionState(QStringLiteral("Start"), &machine, []() {});
    auto* blockedTarget = new FunctionState(QStringLiteral("BlockedTarget"), &machine, [&targetReached]() {
        targetReached = true;
    });
    auto* alternateState = new FunctionState(QStringLiteral("Alternate"), &machine, [&alternateReached]() {
        alternateReached = true;
    });
    auto* finalState = new QFinalState(&machine);

    auto* guardedTransition = new GuardedTransition(
        startState, &QGCState::advance,
        blockedTarget,
        [&guardCalled]() {
            guardCalled = true;
            return false;
        }
    );
    startState->addTransition(guardedTransition);
    startState->addTransition(startState, &QGCState::advance, alternateState);
    blockedTarget->addTransition(blockedTarget, &QGCState::advance, finalState);
    alternateState->addTransition(alternateState, &QGCState::advance, finalState);
    machine.setInitialState(startState);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    QVERIFY(finishedSpy.wait(500));
    QVERIFY(guardCalled);
    QVERIFY(!targetReached);
    QVERIFY(alternateReached);
}

UT_REGISTER_TEST(GuardedTransitionTest, TestLabel::Unit, TestLabel::Utilities)
