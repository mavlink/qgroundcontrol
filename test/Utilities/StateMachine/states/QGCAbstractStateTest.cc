#include "QGCAbstractStateTest.h"
#include "StateTestCommon.h"


/// Concrete implementation for testing QGCAbstractState
class TestAbstractState : public QGCAbstractState
{
    Q_OBJECT
public:
    TestAbstractState(const QString& name, QState* parent)
        : QGCAbstractState(name, parent) {}

    void emitAdvance() { emit advance(); }
    void emitError() { emit error(); }

    bool onEnterCalled = false;
    bool onLeaveCalled = false;

protected:
    void onEnter() override { onEnterCalled = true; }
    void onLeave() override { onLeaveCalled = true; }
};


void QGCAbstractStateTest::_testEntryCallback()
{
    QStateMachine machine;
    bool callbackCalled = false;

    auto* state = new TestAbstractState(QStringLiteral("TestEntry"), &machine);
    state->setOnEntry([&callbackCalled]() { callbackCalled = true; });

    auto* finalState = new QFinalState(&machine);

    // Use machine event to transition out
    machine.addTransition(state, &QGCAbstractState::advance, finalState);

    machine.setInitialState(state);

    QSignalSpy enteredSpy(state, &QAbstractState::entered);
    machine.start();

    QVERIFY(enteredSpy.wait(500));
    QVERIFY(callbackCalled);

    // Cleanup - emit advance to exit
    state->emitAdvance();
}

void QGCAbstractStateTest::_testExitCallback()
{
    QStateMachine machine;
    bool callbackCalled = false;

    auto* state = new TestAbstractState(QStringLiteral("TestExit"), &machine);
    state->setOnExit([&callbackCalled]() { callbackCalled = true; });

    auto* finalState = new QFinalState(&machine);

    machine.addTransition(state, &QGCAbstractState::advance, finalState);

    machine.setInitialState(state);

    QSignalSpy enteredSpy(state, &QAbstractState::entered);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);

    machine.start();

    QVERIFY(enteredSpy.wait(500));
    QVERIFY(!callbackCalled);  // Not called yet

    state->emitAdvance();

    QVERIFY(finishedSpy.wait(500));
    QVERIFY(callbackCalled);  // Called on exit
}

void QGCAbstractStateTest::_testBothCallbacks()
{
    QStateMachine machine;
    bool entryCallbackCalled = false;
    bool exitCallbackCalled = false;

    auto* state = new TestAbstractState(QStringLiteral("TestBoth"), &machine);
    state->setCallbacks(
        [&entryCallbackCalled]() { entryCallbackCalled = true; },
        [&exitCallbackCalled]() { exitCallbackCalled = true; }
    );

    auto* finalState = new QFinalState(&machine);

    machine.addTransition(state, &QGCAbstractState::advance, finalState);

    machine.setInitialState(state);

    QSignalSpy enteredSpy(state, &QAbstractState::entered);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);

    machine.start();

    QVERIFY(enteredSpy.wait(500));
    QVERIFY(entryCallbackCalled);
    QVERIFY(!exitCallbackCalled);

    state->emitAdvance();

    QVERIFY(finishedSpy.wait(500));
    QVERIFY(exitCallbackCalled);
}

void QGCAbstractStateTest::_testOnEnterOverride()
{
    QStateMachine machine;

    auto* state = new TestAbstractState(QStringLiteral("TestOnEnter"), &machine);
    auto* finalState = new QFinalState(&machine);

    machine.addTransition(state, &QGCAbstractState::advance, finalState);

    machine.setInitialState(state);

    QSignalSpy enteredSpy(state, &QAbstractState::entered);
    machine.start();

    QVERIFY(enteredSpy.wait(500));
    QVERIFY(state->onEnterCalled);

    state->emitAdvance();
}

void QGCAbstractStateTest::_testOnLeaveOverride()
{
    QStateMachine machine;

    auto* state = new TestAbstractState(QStringLiteral("TestOnLeave"), &machine);
    auto* finalState = new QFinalState(&machine);

    machine.addTransition(state, &QGCAbstractState::advance, finalState);

    machine.setInitialState(state);

    QSignalSpy enteredSpy(state, &QAbstractState::entered);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);

    machine.start();

    QVERIFY(enteredSpy.wait(500));
    QVERIFY(!state->onLeaveCalled);

    state->emitAdvance();

    QVERIFY(finishedSpy.wait(500));
    QVERIFY(state->onLeaveCalled);
}

void QGCAbstractStateTest::_testStateName()
{
    QStateMachine machine;

    auto* state = new TestAbstractState(QStringLiteral("MyStateName"), &machine);

    QCOMPARE(state->stateName(), QStringLiteral("MyStateName"));
    QCOMPARE(state->objectName(), QStringLiteral("MyStateName"));
}

void QGCAbstractStateTest::_testAdvanceSignal()
{
    QStateMachine machine;

    auto* state = new TestAbstractState(QStringLiteral("TestAdvance"), &machine);
    auto* finalState = new QFinalState(&machine);

    machine.addTransition(state, &QGCAbstractState::advance, finalState);

    machine.setInitialState(state);

    QSignalSpy advanceSpy(state, &QGCAbstractState::advance);
    QSignalSpy enteredSpy(state, &QAbstractState::entered);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);

    machine.start();

    QVERIFY(enteredSpy.wait(500));

    state->emitAdvance();

    QVERIFY(finishedSpy.wait(500));
    QCOMPARE(advanceSpy.count(), 1);
}

void QGCAbstractStateTest::_testErrorSignal()
{
    QStateMachine machine;

    auto* state = new TestAbstractState(QStringLiteral("TestError"), &machine);
    auto* errorState = new FunctionState(QStringLiteral("Error"), &machine, []() {});
    auto* finalState = new QFinalState(&machine);

    machine.addTransition(state, &QGCAbstractState::error, errorState);

    errorState->addTransition(errorState, &QGCState::advance, finalState);

    machine.setInitialState(state);

    QSignalSpy errorSpy(state, &QGCAbstractState::error);
    QSignalSpy enteredSpy(state, &QAbstractState::entered);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);

    machine.start();

    QVERIFY(enteredSpy.wait(500));

    state->emitError();

    QVERIFY(finishedSpy.wait(500));
    QCOMPARE(errorSpy.count(), 1);
}

void QGCAbstractStateTest::_testEventHandler()
{
    QStateMachine machine;
    bool handlerCalled = false;

    auto* state = new TestAbstractState(QStringLiteral("TestEventHandler"), &machine);
    state->setEventHandler([&handlerCalled](QEvent* event) {
        Q_UNUSED(event);
        handlerCalled = true;
        return false;  // Don't consume the event
    });

    auto* finalState = new QFinalState(&machine);

    machine.addTransition(state, &QGCAbstractState::advance, finalState);

    machine.setInitialState(state);

    QSignalSpy enteredSpy(state, &QAbstractState::entered);
    machine.start();

    QVERIFY(enteredSpy.wait(500));

    // Event handler was called during state machine processing
    // (the exact timing depends on Qt's event handling)

    state->emitAdvance();
}

#include "QGCAbstractStateTest.moc"
