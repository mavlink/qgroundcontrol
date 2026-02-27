#include "WaitStateBaseTest.h"
#include "StateTestCommon.h"

namespace {
bool _spyTriggered(QSignalSpy& spy, int timeoutMsecs)
{
    return (spy.count() > 0) || spy.wait(timeoutMsecs);
}
}


/// Concrete implementation for testing WaitStateBase
class TestWaitState : public WaitStateBase
{
    Q_OBJECT
public:
    TestWaitState(const QString& name, QState* parent, int timeoutMsecs = 0)
        : WaitStateBase(name, parent, timeoutMsecs) {}

    void triggerComplete() { waitComplete(); }
    void triggerFailed() { waitFailed(); }

    bool connectCalled = false;
    bool disconnectCalled = false;

protected:
    void connectWaitSignal() override { connectCalled = true; }
    void disconnectWaitSignal() override { disconnectCalled = true; }
};


void WaitStateBaseTest::_testTimeoutEmission()
{
    QStateMachine machine;
    const int timeoutMs = 50;

    auto* waitState = new TestWaitState(QStringLiteral("TestTimeout"), &machine, timeoutMs);
    auto* finalState = new QFinalState(&machine);

    waitState->addTransition(waitState, &WaitStateBase::timeout, finalState);
    machine.setInitialState(waitState);

    QSignalSpy timeoutSpy(waitState, &WaitStateBase::timeout);
    QSignalSpy timedOutSpy(waitState, &WaitStateBase::timedOut);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);

    machine.start();

    QVERIFY(_spyTriggered(finishedSpy, 500));
    QCOMPARE(timeoutSpy.count(), 1);
    QCOMPARE(timedOutSpy.count(), 1);
}

void WaitStateBaseTest::_testWaitComplete()
{
    QStateMachine machine;

    auto* waitState = new TestWaitState(QStringLiteral("TestComplete"), &machine, 0);
    auto* finalState = new QFinalState(&machine);

    waitState->addTransition(waitState, &WaitStateBase::completed, finalState);
    machine.setInitialState(waitState);

    QSignalSpy completedSpy(waitState, &WaitStateBase::completed);
    QSignalSpy advanceSpy(waitState, &QGCState::advance);
    QSignalSpy enteredSpy(waitState, &QState::entered);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);

    machine.start();

    QVERIFY(enteredSpy.wait(500));

    // Trigger completion
    waitState->triggerComplete();

    QVERIFY(_spyTriggered(finishedSpy, 500));
    QCOMPARE(completedSpy.count(), 1);
    QCOMPARE(advanceSpy.count(), 1);
}

void WaitStateBaseTest::_testWaitFailed()
{
    QStateMachine machine;

    auto* waitState = new TestWaitState(QStringLiteral("TestFailed"), &machine, 0);
    auto* errorState = new FunctionState(QStringLiteral("Error"), &machine, []() {});
    auto* finalState = new QFinalState(&machine);

    waitState->addTransition(waitState, &QGCState::error, errorState);
    errorState->addTransition(errorState, &QGCState::advance, finalState);
    machine.setInitialState(waitState);

    QSignalSpy errorSpy(waitState, &QGCState::error);
    QSignalSpy enteredSpy(waitState, &QState::entered);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);

    machine.start();

    QVERIFY(enteredSpy.wait(500));

    // Trigger failure
    waitState->triggerFailed();

    QVERIFY(_spyTriggered(finishedSpy, 500));
    QCOMPARE(errorSpy.count(), 1);
}

void WaitStateBaseTest::_testNoTimeoutWhenZero()
{
    QStateMachine machine;

    // Create with 0 timeout - should never timeout
    auto* waitState = new TestWaitState(QStringLiteral("TestNoTimeout"), &machine, 0);
    auto* finalState = new QFinalState(&machine);

    waitState->addTransition(waitState, &WaitStateBase::completed, finalState);
    machine.setInitialState(waitState);

    QSignalSpy timeoutSpy(waitState, &WaitStateBase::timeout);
    QSignalSpy enteredSpy(waitState, &QState::entered);

    machine.start();

    QVERIFY(enteredSpy.wait(500));

    // Wait a bit - no timeout should occur
    QTest::qWait(100);
    QCOMPARE(timeoutSpy.count(), 0);

    // Complete manually to end the test
    waitState->triggerComplete();
}

void WaitStateBaseTest::_testDoubleCompleteProtection()
{
    QStateMachine machine;

    auto* waitState = new TestWaitState(QStringLiteral("TestDoubleComplete"), &machine, 0);
    auto* finalState = new QFinalState(&machine);

    waitState->addTransition(waitState, &WaitStateBase::completed, finalState);
    machine.setInitialState(waitState);

    QSignalSpy completedSpy(waitState, &WaitStateBase::completed);
    QSignalSpy enteredSpy(waitState, &QState::entered);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);

    machine.start();

    QVERIFY(enteredSpy.wait(500));

    // Call complete twice - should only emit once
    waitState->triggerComplete();
    waitState->triggerComplete();

    QVERIFY(_spyTriggered(finishedSpy, 500));
    QCOMPARE(completedSpy.count(), 1);
}

void WaitStateBaseTest::_testDoubleFailProtection()
{
    QStateMachine machine;

    auto* waitState = new TestWaitState(QStringLiteral("TestDoubleFail"), &machine, 0);
    auto* errorState = new FunctionState(QStringLiteral("Error"), &machine, []() {});
    auto* finalState = new QFinalState(&machine);

    waitState->addTransition(waitState, &QGCState::error, errorState);
    errorState->addTransition(errorState, &QGCState::advance, finalState);
    machine.setInitialState(waitState);

    QSignalSpy errorSpy(waitState, &QGCState::error);
    QSignalSpy enteredSpy(waitState, &QState::entered);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);

    machine.start();

    QVERIFY(enteredSpy.wait(500));

    // Call fail twice - should only emit once
    waitState->triggerFailed();
    waitState->triggerFailed();

    QVERIFY(_spyTriggered(finishedSpy, 500));
    QCOMPARE(errorSpy.count(), 1);
}

void WaitStateBaseTest::_testTimeoutCancelledOnComplete()
{
    QStateMachine machine;
    const int timeoutMs = 200;

    auto* waitState = new TestWaitState(QStringLiteral("TestCancelTimeout"), &machine, timeoutMs);
    auto* finalState = new QFinalState(&machine);

    waitState->addTransition(waitState, &WaitStateBase::completed, finalState);
    machine.setInitialState(waitState);

    QSignalSpy timeoutSpy(waitState, &WaitStateBase::timeout);
    QSignalSpy completedSpy(waitState, &WaitStateBase::completed);
    QSignalSpy enteredSpy(waitState, &QState::entered);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);

    machine.start();

    QVERIFY(enteredSpy.wait(500));

    // Complete before timeout
    QTimer::singleShot(50, waitState, [waitState]() {
        waitState->triggerComplete();
    });

    QVERIFY(_spyTriggered(finishedSpy, 500));
    QCOMPARE(completedSpy.count(), 1);
    QCOMPARE(timeoutSpy.count(), 0);  // Timeout should have been cancelled
}

void WaitStateBaseTest::_testSignalDisconnectOnExit()
{
    QStateMachine machine;

    auto* waitState = new TestWaitState(QStringLiteral("TestDisconnect"), &machine, 0);
    auto* finalState = new QFinalState(&machine);

    waitState->addTransition(waitState, &WaitStateBase::completed, finalState);
    machine.setInitialState(waitState);

    QSignalSpy enteredSpy(waitState, &QState::entered);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);

    machine.start();

    QVERIFY(enteredSpy.wait(500));
    QVERIFY(waitState->connectCalled);

    waitState->triggerComplete();

    QVERIFY(_spyTriggered(finishedSpy, 500));
    QVERIFY(waitState->disconnectCalled);
}

#include "WaitStateBaseTest.moc"

UT_REGISTER_TEST(WaitStateBaseTest, TestLabel::Unit, TestLabel::Utilities)
