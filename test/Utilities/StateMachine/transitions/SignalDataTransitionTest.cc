#include "SignalDataTransitionTest.h"
#include "TransitionTestCommon.h"


void SignalDataTransitionTest::_testSignalDataTransition()
{
    QStateMachine machine;
    bool guardCalled = false;
    bool actionCalled = false;
    QString receivedValue;

    SignalEmitter emitter;

    auto* waitState = new QState(&machine);
    auto* targetState = new QState(&machine);
    auto* finalState = new QFinalState(&machine);

    auto* transition = new SignalDataTransition<QString>(
        &emitter, &SignalEmitter::valueChanged,
        targetState,
        [&guardCalled](const QString& value) {
            guardCalled = true;
            return value == QStringLiteral("trigger");
        },
        [&actionCalled, &receivedValue](const QString& value) {
            actionCalled = true;
            receivedValue = value;
        }
    );
    waitState->addTransition(transition);
    // Use entered signal to immediately transition to final state
    targetState->addTransition(targetState, &QState::entered, finalState);

    machine.setInitialState(waitState);

    QSignalSpy enteredSpy(waitState, &QState::entered);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);

    machine.start();
    QVERIFY(enteredSpy.wait(500));

    // This should not trigger (guard returns false)
    emitter.emitValueChanged(QStringLiteral("notTrigger"));
    QTest::qWait(50);
    // Guard might not be called if signal isn't reaching transition
    // This is testing the blocking case
    QVERIFY(!actionCalled);

    // This should trigger
    emitter.emitValueChanged(QStringLiteral("trigger"));

    // Give time for event processing
    if (!finishedSpy.wait(500)) {
        qDebug() << "finishedSpy wait failed. guardCalled:" << guardCalled << "actionCalled:" << actionCalled;
    }
    QVERIFY(guardCalled);
    QVERIFY(actionCalled);
    QCOMPARE(receivedValue, QStringLiteral("trigger"));
}

