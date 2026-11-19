#include "MachineEventTransitionTest.h"

#include <memory>

#include <QtCore/QCoreApplication>
#include <QtCore/QRegularExpression>

#include "StateTestCommon.h"

void MachineEventTransitionTest::_testMachineEventTransition_data()
{
    QTest::addColumn<bool>("delayed");

    QTest::newRow("posted event") << false;
    QTest::newRow("delayed event") << true;
}

void MachineEventTransitionTest::_testMachineEventTransition()
{
    QFETCH(bool, delayed);

    QGCStateMachine machine(QStringLiteral("EventTest"), nullptr);
    bool eventHandled = false;

    auto* waitState = new QGCState(QStringLiteral("Wait"), &machine);
    auto* eventState = new FunctionState(QStringLiteral("EventHandler"), &machine, [&eventHandled]() {
        eventHandled = true;
    });
    auto* finalState = new QGCFinalState(QStringLiteral("Final"), &machine);

    auto* transition = new MachineEventTransition(QStringLiteral("myEvent"), eventState);
    waitState->addTransition(transition);
    eventState->addTransition(eventState, &QGCState::advance, finalState);

    machine.setInitialState(waitState);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);

    QVERIFY(startAndWaitForEntered(&machine, waitState));

    if (delayed) {
        machine.postDelayedEvent(QStringLiteral("myEvent"), 100);
    } else {
        machine.postEvent(QStringLiteral("myEvent"), QStringLiteral("testData"));
    }

    QVERIFY(finishedSpy.wait(TestTimeout::shortMs()));
    QVERIFY(eventHandled);
}

void MachineEventTransitionTest::_testMachineEventTransitionWithGuard()
{
    QGCStateMachine machine(QStringLiteral("EventGuardTest"), nullptr);
    bool guardCalled = false;
    bool transitioned = false;

    auto* waitState = new FunctionState(QStringLiteral("Wait"), &machine, []() {});
    auto* targetState =
        new FunctionState(QStringLiteral("Target"), &machine, [&transitioned]() { transitioned = true; });
    auto* finalState = machine.addFinalState();

    auto* transition = new MachineEventTransition(QStringLiteral("myEvent"), targetState,
                                                  [&guardCalled](const QGCStateMachineEvent* event) {
                                                      guardCalled = true;
                                                      return event->data().toString() == QStringLiteral("allowed");
                                                  });
    waitState->addTransition(transition);
    targetState->addTransition(targetState, &QGCState::advance, finalState);

    machine.setInitialState(waitState);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);

    QVERIFY(startAndWaitForEntered(&machine, waitState));

    // This should NOT trigger (guard blocks)
    machine.postEvent(QStringLiteral("myEvent"), QStringLiteral("blocked"));
    QCoreApplication::processEvents();
    QVERIFY(!transitioned);
    QVERIFY(!transition->eventData().isValid());

    // This should trigger
    machine.postEvent(QStringLiteral("myEvent"), QStringLiteral("allowed"));
    QVERIFY(finishedSpy.wait(TestTimeout::shortMs()));
    QVERIFY(guardCalled);
    QVERIFY(transitioned);
    // The transition keeps the data of the event that it took
    QCOMPARE(transition->eventName(), QStringLiteral("myEvent"));
    QCOMPARE(transition->eventData().toString(), QStringLiteral("allowed"));
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

    QVERIFY(startAndWaitForEntered(&machine, waitState));

    int eventId = machine.postDelayedEvent(QStringLiteral("testEvent"), 200);
    QVERIFY(machine.cancelDelayedEvent(eventId));

    // The cancel is synchronous; flush the event loop once to prove no queued
    // delivery is pending, rather than burning a full 250 ms timer wait.
    QCoreApplication::processEvents();
    QVERIFY(!eventReceived);

    machine.stop();
}

void MachineEventTransitionTest::_testEventThatIsNotPostedIsDeleted_data()
{
    QTest::addColumn<bool>("started");
    QTest::addColumn<bool>("running");

    QTest::newRow("machine never started") << false << false;
    QTest::newRow("between start() and the entry of the initial state") << true << false;
    QTest::newRow("running machine, negative delay") << true << true;
}

void MachineEventTransitionTest::_testEventThatIsNotPostedIsDeleted()
{
    QFETCH(bool, started);
    QFETCH(bool, running);

    QGCStateMachine machine(QStringLiteral("EventNotPostedTest"), nullptr);
    auto* waitState = new QGCState(QStringLiteral("Wait"), &machine);
    machine.addEventTransition(waitState, QStringLiteral("go"), machine.addFinalState());
    machine.setInitialState(waitState);

    QSignalSpy enteredSpy(waitState, &QState::entered);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    if (started) {
        machine.start();
    }
    if (running) {
        QVERIFY(spyTriggered(enteredSpy));
    }

    // An event holds a copy of its data for as long as it exists
    const auto payload = std::make_shared<int>(0);
    if (running) {
        expectLogMessage("default", QtWarningMsg, QRegularExpression("postDelayedEvent: delay cannot be negative"));
        QCOMPARE(machine.postDelayedEvent(QStringLiteral("go"), -1, QVariant::fromValue(payload)), -1);
        verifyExpectedLogMessage();
    } else {
        const QRegularExpression notPosted(QStringLiteral("event not posted, the machine is not running: \"go\""));
        expectLogMessage("Utilities.StateMachine.QGCStateMachine", QtWarningMsg, notPosted);
        machine.postEvent(QStringLiteral("go"), QVariant::fromValue(payload));
        verifyExpectedLogMessage();
        expectLogMessage("Utilities.StateMachine.QGCStateMachine", QtWarningMsg, notPosted);
        QCOMPARE(machine.postDelayedEvent(QStringLiteral("go"), 0, QVariant::fromValue(payload)), -1);
        verifyExpectedLogMessage();
    }
    QCOMPARE(payload.use_count(), 1);

    if (started) {
        // The machine has taken no event
        QVERIFY(spyTriggered(enteredSpy));
        QCoreApplication::processEvents();
        QCOMPARE(finishedSpy.count(), 0);
    }
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
    highState->addTransition(new MachineEventTransition(QStringLiteral("normalEvent"), normalState));
    normalState->addTransition(normalState, &QGCState::advance, finalState);

    machine.setInitialState(waitState);

    QVERIFY(startAndWaitForEntered(&machine, waitState));

    // Post normal event first, then high priority
    machine.postEvent(QStringLiteral("normalEvent"));
    machine.postEvent(QStringLiteral("highEvent"), QVariant(), QStateMachine::HighPriority);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    QVERIFY(finishedSpy.wait(TestTimeout::shortMs()));

    // The high priority event is handled first and the normal one after it. In the order they were posted, the
    // normal event would have taken the machine past the High state
    QCOMPARE(eventOrder, QStringList({QStringLiteral("high"), QStringLiteral("normal")}));
}

UT_REGISTER_TEST(MachineEventTransitionTest, TestLabel::Unit, TestLabel::Utilities)
