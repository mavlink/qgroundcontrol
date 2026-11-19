#include "QGCStateTest.h"

#include "StateTestCommon.h"

void QGCStateTest::_testStateName()
{
    QStateMachine machine;

    auto* state = new TestSignalState(QStringLiteral("MyStateName"), &machine);

    QCOMPARE(state->stateName(), QStringLiteral("MyStateName"));
    QCOMPARE(state->objectName(), QStringLiteral("MyStateName"));
}

void QGCStateTest::_testStateNameInQGCStateMachine()
{
    QGCStateMachine machine(QStringLiteral("Machine"), nullptr);

    auto* state = new TestSignalState(QStringLiteral("State"), &machine);
    auto* finalState = new QGCFinalState(QStringLiteral("Final"), &machine);

    QCOMPARE(state->stateName(), QStringLiteral("Machine:State"));
    QCOMPARE(finalState->stateName(), QStringLiteral("Machine:Final"));
}

void QGCStateTest::_testSignalStopsMachine_data()
{
    QTest::addColumn<bool>("emitError");

    QTest::newRow("advance") << false;
    QTest::newRow("error") << true;
}

void QGCStateTest::_testSignalStopsMachine()
{
    QFETCH(bool, emitError);
    const auto signal = emitError ? &QGCState::error : &QGCState::advance;

    QStateMachine machine;

    auto* state = new TestSignalState(QStringLiteral("TestSignal"), &machine);
    machine.setInitialState(state);
    QObject::connect(state, signal, &machine, &QStateMachine::stop);

    QSignalSpy signalSpy(state, signal);
    QSignalSpy stoppedSpy(&machine, &QStateMachine::stopped);

    QVERIFY(startAndWaitForEntered(&machine, state));

    if (emitError) {
        state->emitError();
    } else {
        state->emitAdvance();
    }

    QVERIFY(spyTriggered(stoppedSpy, TestTimeout::shortMs()));
    QCOMPARE(signalSpy.count(), 1);
}

UT_REGISTER_TEST(QGCStateTest, TestLabel::Unit, TestLabel::Utilities)
