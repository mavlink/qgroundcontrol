#include "QGCAbstractStateTest.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QEvent>

#include "StateTestCommon.h"

void QGCAbstractStateTest::_testEntryAndExitHooks_data()
{
    QTest::addColumn<bool>("withEntryCallback");
    QTest::addColumn<bool>("withExitCallback");

    QTest::newRow("no callback") << false << false;
    QTest::newRow("setOnEntry") << true << false;
    QTest::newRow("setOnExit") << false << true;
    QTest::newRow("setOnEntry and setOnExit") << true << true;
}

void QGCAbstractStateTest::_testEntryAndExitHooks()
{
    QFETCH(bool, withEntryCallback);
    QFETCH(bool, withExitCallback);

    QStateMachine machine;
    bool entryCallbackCalled = false;
    bool exitCallbackCalled = false;

    auto* state = new TestAbstractState(QStringLiteral("TestHooks"), &machine);
    const auto onEntry = [&entryCallbackCalled]() { entryCallbackCalled = true; };
    const auto onExit = [&exitCallbackCalled]() { exitCallbackCalled = true; };
    if (withEntryCallback) {
        state->setOnEntry(onEntry);
    }
    if (withExitCallback) {
        state->setOnExit(onExit);
    }

    machine.setInitialState(state);

    QVERIFY(startAndWaitForEntered(&machine, state));
    QCOMPARE(entryCallbackCalled, withEntryCallback);
    QVERIFY(state->onEnterCalled);
    QVERIFY(!exitCallbackCalled);
    QVERIFY(!state->onLeaveCalled);

    state->invokeExitForTest();
    QCOMPARE(exitCallbackCalled, withExitCallback);
    QVERIFY(state->onLeaveCalled);
}

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

void QGCAbstractStateTest::_testEventHandler()
{
    QStateMachine machine;
    bool handlerCalled = false;

    auto* state = new TestAbstractState(QStringLiteral("TestEventHandler"), &machine);
    state->setEventHandler([&handlerCalled](QEvent* event) {
        if (event->type() == QEvent::User) {
            handlerCalled = true;
        }
        return false;  // Don't consume the event
    });

    // The handler is given the events that the state object receives
    QEvent event(QEvent::User);
    QCoreApplication::sendEvent(state, &event);
    QVERIFY(handlerCalled);
}

UT_REGISTER_TEST(QGCAbstractStateTest, TestLabel::Unit, TestLabel::Utilities)
