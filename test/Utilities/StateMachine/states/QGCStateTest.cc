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

    auto* finalState = addFinalState(&machine);
    state->addTransition(state, &QState::entered, finalState);

    machine.setInitialState(state);

    QVERIFY(startAndWaitForFinished(&machine));
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
    auto* finalState = addFinalState(&machine);

    state->addTransition(state, &QState::entered, finalState);
    machine.setInitialState(state);

    QVERIFY(startAndWaitForFinished(&machine));
    QVERIFY(state->onEnterCalled);
    QVERIFY(state->onLeaveCalled);
}

UT_REGISTER_TEST(QGCStateTest, TestLabel::Unit, TestLabel::Utilities)
