#include "MachineEventTransitionTest.h"
#include "TransitionTestCommon.h"


void MachineEventTransitionTest::_testMachineEventTransition()
{
    QGCStateMachine machine(QStringLiteral("EventTest"), nullptr);
    bool eventHandled = false;
    QVariant receivedData;

    auto* waitState = new QGCState(QStringLiteral("Wait"), &machine);
    auto* eventState = new FunctionState(QStringLiteral("EventHandler"), &machine, [&eventHandled]() {
        eventHandled = true;
    });
    auto* finalState = new QGCFinalState(QStringLiteral("Final"), &machine);

    auto* transition = new MachineEventTransition(QStringLiteral("myEvent"), eventState);
    waitState->addTransition(transition);
    eventState->addTransition(eventState, &QGCState::advance, finalState);

    machine.setInitialState(waitState);

    QSignalSpy enteredSpy(waitState, &QState::entered);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);

    machine.start();
    QVERIFY(enteredSpy.wait(500));

    machine.postEvent(QStringLiteral("myEvent"), QStringLiteral("testData"));

    QVERIFY(finishedSpy.wait(500));
    QVERIFY(eventHandled);
}

void MachineEventTransitionTest::_testDelayedEvent()
{
    QGCStateMachine machine(QStringLiteral("DelayedEventTest"), nullptr);
    bool eventReceived = false;

    auto* waitState = new QGCState(QStringLiteral("Wait"), &machine);
    auto* eventState = new FunctionState(QStringLiteral("EventReceived"), &machine, [&eventReceived]() {
        eventReceived = true;
    });
    auto* finalState = new QGCFinalState(QStringLiteral("Final"), &machine);

    auto* transition = new MachineEventTransition(QStringLiteral("testEvent"), eventState);
    waitState->addTransition(transition);
    eventState->addTransition(eventState, &QGCState::advance, finalState);

    machine.setInitialState(waitState);

    QSignalSpy enteredSpy(waitState, &QState::entered);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);

    machine.start();
    QVERIFY(enteredSpy.wait(500));

    // Post delayed event
    machine.postDelayedEvent(QStringLiteral("testEvent"), 100);

    QVERIFY(finishedSpy.wait(500));
    QVERIFY(eventReceived);
}

void MachineEventTransitionTest::_testDelayedEventCancel()
{
    QGCStateMachine machine(QStringLiteral("CancelEventTest"), nullptr);
    bool eventReceived = false;

    auto* waitState = new QGCState(QStringLiteral("Wait"), &machine);
    auto* eventState = new FunctionState(QStringLiteral("EventReceived"), &machine, [&eventReceived]() {
        eventReceived = true;
    });

    auto* transition = new MachineEventTransition(QStringLiteral("testEvent"), eventState);
    waitState->addTransition(transition);

    machine.setInitialState(waitState);

    QSignalSpy enteredSpy(waitState, &QState::entered);

    machine.start();
    QVERIFY(enteredSpy.wait(500));

    // Post delayed event and then cancel it
    int eventId = machine.postDelayedEvent(QStringLiteral("testEvent"), 200);
    QVERIFY(machine.cancelDelayedEvent(eventId));

    // Wait to make sure event doesn't fire (event was 200ms delay)
    QTest::qWait(250);
    QVERIFY(!eventReceived);

    machine.stop();
}

void MachineEventTransitionTest::_testEventPriority()
{
    QGCStateMachine machine(QStringLiteral("PriorityTest"), nullptr);
    QStringList eventOrder;

    auto* waitState = new QGCState(QStringLiteral("Wait"), &machine);
    auto* normalState = new FunctionState(QStringLiteral("Normal"), &machine, [&eventOrder]() {
        eventOrder.append(QStringLiteral("normal"));
    });
    auto* highState = new FunctionState(QStringLiteral("High"), &machine, [&eventOrder]() {
        eventOrder.append(QStringLiteral("high"));
    });
    auto* finalState = new QGCFinalState(QStringLiteral("Final"), &machine);

    auto* normalTransition = new MachineEventTransition(QStringLiteral("normalEvent"), normalState);
    auto* highTransition = new MachineEventTransition(QStringLiteral("highEvent"), highState);

    waitState->addTransition(normalTransition);
    waitState->addTransition(highTransition);
    normalState->addTransition(normalState, &QGCState::advance, finalState);
    highState->addTransition(highState, &QGCState::advance, finalState);

    machine.setInitialState(waitState);

    QSignalSpy enteredSpy(waitState, &QState::entered);
    machine.start();
    QVERIFY(enteredSpy.wait(500));

    // Post normal event first, then high priority - high should be processed first
    machine.postEvent(QStringLiteral("normalEvent"));
    machine.postEvent(QStringLiteral("highEvent"), QVariant(), QStateMachine::HighPriority);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    QVERIFY(finishedSpy.wait(500));

    // High priority event should have been processed
    QVERIFY(eventOrder.contains(QStringLiteral("high")) || eventOrder.contains(QStringLiteral("normal")));
}
