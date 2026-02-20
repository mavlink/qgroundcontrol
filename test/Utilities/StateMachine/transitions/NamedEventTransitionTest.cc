#include "NamedEventTransitionTest.h"
#include "TransitionTestCommon.h"


void NamedEventTransitionTest::_testNamedEventTransition()
{
    QGCStateMachine machine(QStringLiteral("NamedEventTest"), nullptr);
    bool transitioned = false;

    auto* waitState = new FunctionState(QStringLiteral("Wait"), &machine, []() {});
    auto* targetState = new FunctionState(QStringLiteral("Target"), &machine, [&transitioned]() {
        transitioned = true;
    });
    auto* finalState = machine.addFinalState();

    // NamedEventTransition uses QAbstractTransition to intercept events posted via postEvent()
    auto* transition = new NamedEventTransition(QStringLiteral("myEvent"), targetState);
    waitState->addTransition(transition);
    targetState->addTransition(targetState, &QGCState::advance, finalState);

    machine.setInitialState(waitState);

    QSignalSpy enteredSpy(waitState, &QState::entered);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);

    machine.start();
    QVERIFY(enteredSpy.wait(500));

    // Post the event to the state machine
    machine.postEvent(QStringLiteral("myEvent"));

    QVERIFY(finishedSpy.wait(500));
    QVERIFY(transitioned);
}

void NamedEventTransitionTest::_testNamedEventTransitionWithGuard()
{
    QGCStateMachine machine(QStringLiteral("NamedEventGuardTest"), nullptr);
    bool guardCalled = false;
    bool transitioned = false;

    auto* waitState = new FunctionState(QStringLiteral("Wait"), &machine, []() {});
    auto* targetState = new FunctionState(QStringLiteral("Target"), &machine, [&transitioned]() {
        transitioned = true;
    });
    auto* finalState = machine.addFinalState();

    auto* transition = new NamedEventTransition(
        QStringLiteral("myEvent"), targetState,
        [&guardCalled](const QGCStateMachineEvent* event) {
            guardCalled = true;
            return event->data().toString() == QStringLiteral("allowed");
        }
    );
    waitState->addTransition(transition);
    targetState->addTransition(targetState, &QGCState::advance, finalState);

    machine.setInitialState(waitState);

    QSignalSpy enteredSpy(waitState, &QState::entered);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);

    machine.start();
    QVERIFY(enteredSpy.wait(500));

    // This should NOT trigger (guard blocks)
    machine.postEvent(QStringLiteral("myEvent"), QStringLiteral("blocked"));
    QTest::qWait(50);
    QVERIFY(!transitioned);

    // This should trigger
    machine.postEvent(QStringLiteral("myEvent"), QStringLiteral("allowed"));
    QVERIFY(finishedSpy.wait(500));
    QVERIFY(guardCalled);
    QVERIFY(transitioned);
}

UT_REGISTER_TEST(NamedEventTransitionTest, TestLabel::Unit, TestLabel::Utilities)
