#include "QGCStateTest.h"
#include "StateTestCommon.h"


void QGCStateTest::_testEntryExitCallbacks()
{
    QStateMachine machine;
    bool entryCalled = false;
    bool exitCalled = false;

    auto* state = new QGCState(QStringLiteral("TestState"), &machine);
    state->setOnEntry([&entryCalled]() { entryCalled = true; });
    state->setOnExit([&exitCalled]() { exitCalled = true; });

    auto* finalState = new QFinalState(&machine);
    state->addTransition(state, &QState::entered, finalState);

    machine.setInitialState(state);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    QVERIFY(finishedSpy.wait(500));
    QVERIFY(entryCalled);
    QVERIFY(exitCalled);
}

void QGCStateTest::_testOnEnterOnLeaveVirtuals()
{
    // Test that onEnter/onLeave virtuals are called in the correct order
    class TestState : public QGCState
    {
    public:
        TestState(QState* parent) : QGCState(QStringLiteral("TestState"), parent) {}
        bool onEnterCalled = false;
        bool onLeaveCalled = false;
    protected:
        void onEnter() override { onEnterCalled = true; }
        void onLeave() override { onLeaveCalled = true; }
    };

    QStateMachine machine;
    auto* state = new TestState(&machine);
    auto* finalState = new QFinalState(&machine);

    state->addTransition(state, &QState::entered, finalState);
    machine.setInitialState(state);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    QVERIFY(finishedSpy.wait(500));
    QVERIFY(state->onEnterCalled);
    QVERIFY(state->onLeaveCalled);
}
