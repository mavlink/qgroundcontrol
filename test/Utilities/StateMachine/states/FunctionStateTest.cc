#include "FunctionStateTest.h"
#include "StateTestCommon.h"


void FunctionStateTest::_testFunctionState()
{
    QStateMachine machine;
    bool functionCalled = false;

    auto* state = new FunctionState(QStringLiteral("TestFunction"), &machine, [&functionCalled]() {
        functionCalled = true;
    });
    auto* finalState = new QFinalState(&machine);

    state->addTransition(state, &QGCState::advance, finalState);
    machine.setInitialState(state);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    QVERIFY(finishedSpy.wait(500));
    QVERIFY(functionCalled);
}

void FunctionStateTest::_testFunctionStateChain()
{
    QStateMachine machine;
    QStringList executionOrder;

    auto* state1 = new FunctionState(QStringLiteral("State1"), &machine, [&executionOrder]() {
        executionOrder.append(QStringLiteral("state1"));
    });
    auto* state2 = new FunctionState(QStringLiteral("State2"), &machine, [&executionOrder]() {
        executionOrder.append(QStringLiteral("state2"));
    });
    auto* state3 = new FunctionState(QStringLiteral("State3"), &machine, [&executionOrder]() {
        executionOrder.append(QStringLiteral("state3"));
    });
    auto* finalState = new QFinalState(&machine);

    state1->addTransition(state1, &QGCState::advance, state2);
    state2->addTransition(state2, &QGCState::advance, state3);
    state3->addTransition(state3, &QGCState::advance, finalState);
    machine.setInitialState(state1);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    QVERIFY(finishedSpy.wait(500));
    QCOMPARE(executionOrder.size(), 3);
    QCOMPARE(executionOrder[0], QStringLiteral("state1"));
    QCOMPARE(executionOrder[1], QStringLiteral("state2"));
    QCOMPARE(executionOrder[2], QStringLiteral("state3"));
}
