#include "QGCAbstractStateTest.h"

#include "StateTestCommon.h"

void QGCAbstractStateTest::_testStateName()
{
    QStateMachine machine;

    auto* state = new TestAbstractState(QStringLiteral("MyStateName"), &machine);

    QCOMPARE(state->stateName(), QStringLiteral("MyStateName"));
    QCOMPARE(state->objectName(), QStringLiteral("MyStateName"));
}

void QGCAbstractStateTest::_testStateNameInQGCStateMachine()
{
    QGCStateMachine machine(QStringLiteral("Machine"), nullptr);

    auto* state = new TestAbstractState(QStringLiteral("State"), &machine);
    auto* finalState = new QGCFinalState(QStringLiteral("Final"), &machine);

    QCOMPARE(state->stateName(), QStringLiteral("Machine:State"));
    QCOMPARE(finalState->stateName(), QStringLiteral("Machine:Final"));
}

void QGCAbstractStateTest::_testSignalStopsMachine_data()
{
    QTest::addColumn<bool>("emitError");

    QTest::newRow("advance") << false;
    QTest::newRow("error") << true;
}

void QGCAbstractStateTest::_testSignalStopsMachine()
{
    QFETCH(bool, emitError);
    const auto signal = emitError ? &QGCAbstractState::error : &QGCAbstractState::advance;

    QStateMachine machine;

    auto* state = new TestAbstractState(QStringLiteral("TestSignal"), &machine);
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

UT_REGISTER_TEST(QGCAbstractStateTest, TestLabel::Unit, TestLabel::Utilities)
