#include "QGCHistoryStateTest.h"
#include "StateTestCommon.h"


void QGCHistoryStateTest::_testQGCHistoryState()
{
    // Test QGCHistoryState restores to the last active child state
    QGCStateMachine machine(QStringLiteral("HistoryTest"), nullptr);

    auto* parentState = new QState(&machine);
    auto* childA = new FunctionState(QStringLiteral("ChildA"), parentState, []() {});
    auto* childB = new FunctionState(QStringLiteral("ChildB"), parentState, []() {});
    auto* historyState = new QGCHistoryState(QStringLiteral("History"), parentState);

    auto* outsideState = new FunctionState(QStringLiteral("Outside"), &machine, []() {});
    auto* finalState = machine.addFinalState();

    parentState->setInitialState(childA);

    int childBEntryCount = 0;

    // Track childB entries (connected BEFORE transitions for correct counting)
    QObject::connect(childB, &QState::entered, childB, [&childBEntryCount]() { childBEntryCount++; });

    // A -> B when A advances
    childA->addTransition(childA, &QGCState::advance, childB);

    // B -> outside when B advances (first entry: count == 1)
    auto* toOutsideTransition = new GuardedTransition(
        childB, &QGCState::advance, outsideState,
        [&childBEntryCount]() { return childBEntryCount == 1; }
    );
    childB->addTransition(toOutsideTransition);

    // outside -> history (should restore to B)
    outsideState->addTransition(outsideState, &QGCState::advance, historyState);

    // B -> final when B advances (second entry: count >= 2)
    auto* toFinalTransition = new GuardedTransition(
        childB, &QGCState::advance, finalState,
        [&childBEntryCount]() { return childBEntryCount >= 2; }
    );
    childB->addTransition(toFinalTransition);

    machine.setInitialState(parentState);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    QVERIFY(finishedSpy.wait(500));
    QCOMPARE(childBEntryCount, 2);
}
