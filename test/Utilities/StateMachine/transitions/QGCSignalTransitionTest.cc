#include "QGCSignalTransitionTest.h"
#include "TransitionTestCommon.h"


void QGCSignalTransitionTest::_testTransitionMachineAccessor()
{
    QGCStateMachine machine(QStringLiteral("AccessorTest"), nullptr);

    // Test QGCSignalTransition::machine() via GuardedTransition
    QGCStateMachine* capturedMachine = nullptr;

    auto* startState = new FunctionState(QStringLiteral("Start"), &machine, []() {});
    auto* targetState = new FunctionState(QStringLiteral("Target"), &machine, []() {});
    auto* finalState = machine.addFinalState();

    GuardedTransition* transition = nullptr;
    transition = new GuardedTransition(
        startState, &QGCState::advance,
        targetState,
        [&capturedMachine, &transition]() {
            capturedMachine = transition->machine();
            return true;
        }
    );
    startState->addTransition(transition);
    targetState->addTransition(targetState, &QGCState::advance, finalState);

    machine.setInitialState(startState);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    QVERIFY(finishedSpy.wait(500));
    QCOMPARE(capturedMachine, &machine);
}

void QGCSignalTransitionTest::_testTransitionMachineAccessorAbstract()
{
    QGCStateMachine machine(QStringLiteral("AbstractAccessorTest"), nullptr);

    auto* waitState = new FunctionState(QStringLiteral("Wait"), &machine, []() {});
    auto* targetState = new FunctionState(QStringLiteral("Target"), &machine, []() {});
    auto* finalState = machine.addFinalState();

    // Test QGCAbstractTransition::machine() via MachineEventTransition
    QGCStateMachine* capturedMachine = nullptr;
    MachineEventTransition* transition = nullptr;
    transition = new MachineEventTransition(
        QStringLiteral("testEvent"), targetState,
        [&capturedMachine, &transition](const QGCStateMachineEvent*) {
            capturedMachine = transition->machine();
            return true;
        }
    );
    waitState->addTransition(transition);
    targetState->addTransition(targetState, &QGCState::advance, finalState);

    machine.setInitialState(waitState);

    QSignalSpy enteredSpy(waitState, &QState::entered);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);

    machine.start();
    QVERIFY(enteredSpy.wait(500));

    machine.postEvent(QStringLiteral("testEvent"));

    QVERIFY(finishedSpy.wait(500));
    QCOMPARE(capturedMachine, &machine);
}

UT_REGISTER_TEST(QGCSignalTransitionTest, TestLabel::Unit, TestLabel::Utilities)
