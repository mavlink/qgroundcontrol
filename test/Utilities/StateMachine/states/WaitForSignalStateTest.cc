#include "WaitForSignalStateTest.h"
#include "StateTestCommon.h"


void WaitForSignalStateTest::_testWaitForSignalState()
{
    QStateMachine machine;
    QObject signalSource;

    auto* waitState = new WaitForSignalState(
        QStringLiteral("TestWait"),
        &machine,
        &signalSource,
        &QObject::objectNameChanged,
        0
    );

    auto* finalState = new QFinalState(&machine);

    waitState->addTransition(waitState, &QGCState::advance, finalState);
    machine.setInitialState(waitState);

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    QSignalSpy enteredSpy(waitState, &QState::entered);

    machine.start();

    QVERIFY(enteredSpy.wait(500));

    QTimer::singleShot(50, &signalSource, [&signalSource]() {
        signalSource.setObjectName(QStringLiteral("triggered"));
    });

    QVERIFY(finishedSpy.wait(500));
}

void WaitForSignalStateTest::_testWaitForSignalStateTimeout()
{
    QStateMachine machine;
    QObject signalSource;
    bool timeoutReached = false;
    const int timeoutMs = 100;

    auto* waitState = new WaitForSignalState(
        QStringLiteral("TestWaitTimeout"),
        &machine,
        &signalSource,
        &QObject::objectNameChanged,
        timeoutMs
    );
    auto* timeoutState = new FunctionState(QStringLiteral("TimeoutHandler"), &machine, [&timeoutReached]() {
        timeoutReached = true;
    });
    auto* finalState = new QFinalState(&machine);

    waitState->addTransition(waitState, &QGCState::advance, finalState);
    waitState->addTransition(waitState, &WaitForSignalState::timeout, timeoutState);
    timeoutState->addTransition(timeoutState, &QGCState::advance, finalState);
    machine.setInitialState(waitState);

    // Use MultiSignalSpyV2 to verify timeout() fired but advance() did not
    MultiSignalSpyV2 stateSpy;
    QVERIFY(stateSpy.init(waitState));

    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    machine.start();

    QVERIFY(finishedSpy.wait(500));
    QVERIFY(timeoutReached);
    // Verify timeout path taken, not success path
    QVERIFY(stateSpy.checkSignalByMask(stateSpy.signalNameToMask("timeout")));
    QVERIFY(stateSpy.checkNoSignalByMask(stateSpy.signalNameToMask("advance")));
}

void WaitForSignalStateTest::_testCompletedSignal()
{
    // Test that completed() is emitted alongside advance() for wait states
    QStateMachine machine;
    QObject signalSource;

    auto* waitState = new WaitForSignalState(
        QStringLiteral("TestCompleted"),
        &machine,
        &signalSource,
        &QObject::objectNameChanged,
        0
    );

    auto* finalState = new QFinalState(&machine);

    // Wire using completed() instead of advance()
    waitState->addTransition(waitState, &WaitStateBase::completed, finalState);
    machine.setInitialState(waitState);

    QSignalSpy completedSpy(waitState, &WaitStateBase::completed);
    QSignalSpy advanceSpy(waitState, &QGCState::advance);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);

    machine.start();

    // Trigger the signal
    QTimer::singleShot(50, &signalSource, [&signalSource]() {
        signalSource.setObjectName(QStringLiteral("triggered"));
    });

    QVERIFY(finishedSpy.wait(500));
    // Both completed() and advance() should have fired
    QCOMPARE(completedSpy.count(), 1);
    QCOMPARE(advanceSpy.count(), 1);
}

void WaitForSignalStateTest::_testTimedOutSignal()
{
    // Test that timedOut() is emitted alongside timeout() for wait states
    QStateMachine machine;
    QObject signalSource;
    const int timeoutMs = 50;

    auto* waitState = new WaitForSignalState(
        QStringLiteral("TestTimedOut"),
        &machine,
        &signalSource,
        &QObject::objectNameChanged,
        timeoutMs
    );

    auto* finalState = new QFinalState(&machine);

    // Wire using timedOut() instead of timeout()
    waitState->addTransition(waitState, &WaitStateBase::timedOut, finalState);
    machine.setInitialState(waitState);

    QSignalSpy timedOutSpy(waitState, &WaitStateBase::timedOut);
    QSignalSpy timeoutSpy(waitState, &WaitStateBase::timeout);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);

    machine.start();

    QVERIFY(finishedSpy.wait(500));
    // Both timedOut() and timeout() should have fired
    QCOMPARE(timedOutSpy.count(), 1);
    QCOMPARE(timeoutSpy.count(), 1);
}
