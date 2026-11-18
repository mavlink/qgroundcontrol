#include "QGCSignalTransitionTest.h"

#include "StateTestCommon.h"

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

    QVERIFY(startAndWaitForFinished(&machine));
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

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);

    QVERIFY(startAndWaitForEntered(&machine, waitState));

    machine.postEvent(QStringLiteral("testEvent"));

    QVERIFY(finishedSpy.wait(TestTimeout::shortMs()));
    QCOMPARE(capturedMachine, &machine);
}

void QGCSignalTransitionTest::_testSignalForms_data()
{
    QTest::addColumn<bool>("signalByName");

    QTest::newRow("pointer to the signal") << false;
    QTest::newRow("name of the signal") << true;
}

void QGCSignalTransitionTest::_testSignalForms()
{
    QFETCH(bool, signalByName);

    QGCStateMachine machine(QStringLiteral("SignalFormsTest"), nullptr);
    QObject signalSource;
    auto* startState = new QGCState(QStringLiteral("Start"), &machine);
    auto* finalState = machine.addFinalState();

    // Constructed with a source state, the transition adds itself to that state
    QGCSignalTransition* transition =
        signalByName ? new QGCSignalTransition(&signalSource, SIGNAL(objectNameChanged(QString)), startState)
                     : new QGCSignalTransition(&signalSource, &QObject::objectNameChanged, startState);
    transition->setTargetState(finalState);
    machine.setInitialState(startState);

    QCOMPARE(transition->sourceState(), startState);
    QCOMPARE(transition->senderObject(), &signalSource);
    QCOMPARE(transition->machine(), &machine);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    QVERIFY(startAndWaitForEntered(&machine, startState));
    QCOMPARE(finishedSpy.count(), 0);

    signalSource.setObjectName(QStringLiteral("signalled"));
    QVERIFY(spyTriggered(finishedSpy));
}

UT_REGISTER_TEST(QGCSignalTransitionTest, TestLabel::Unit, TestLabel::Utilities)
