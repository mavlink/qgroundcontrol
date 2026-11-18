#include "WaitStateBaseTest.h"
#include "StateTestCommon.h"

#include <QtCore/QCoreApplication>


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
    ignoreTimeoutWarnings();
    QStateMachine machine;
    const int timeoutMs = 50;

    auto* waitState = new TestWaitState(QStringLiteral("TestTimeout"), &machine, timeoutMs);
    auto* finalState = new QFinalState(&machine);

    waitState->addTransition(waitState, &WaitStateBase::timeout, finalState);
    machine.setInitialState(waitState);

    QSignalSpy timeoutSpy(waitState, &WaitStateBase::timeout);
    QSignalSpy timedOutSpy(waitState, &WaitStateBase::timedOut);
    QVERIFY(startAndWaitForFinished(&machine));
    QCOMPARE(timeoutSpy.count(), 1);
    QCOMPARE(timedOutSpy.count(), 1);
}

void WaitStateBaseTest::_testCompleteAndFail_data()
{
    QTest::addColumn<bool>("fail");
    QTest::addColumn<int>("calls");

    QTest::newRow("complete once") << false << 1;
    QTest::newRow("complete twice") << false << 2;
    QTest::newRow("fail once") << true << 1;
    QTest::newRow("fail twice") << true << 2;
}

void WaitStateBaseTest::_testCompleteAndFail()
{
    QFETCH(bool, fail);
    QFETCH(int, calls);

    QStateMachine machine;

    auto* waitState = new TestWaitState(QStringLiteral("TestCompleteAndFail"), &machine, 0);
    auto* errorState = new FunctionState(QStringLiteral("Error"), &machine, []() {});
    auto* finalState = new QFinalState(&machine);

    waitState->addTransition(waitState, &WaitStateBase::completed, finalState);
    waitState->addTransition(waitState, &QGCState::error, errorState);
    errorState->addTransition(errorState, &QGCState::advance, finalState);
    machine.setInitialState(waitState);

    QSignalSpy completedSpy(waitState, &WaitStateBase::completed);
    QSignalSpy advanceSpy(waitState, &QGCState::advance);
    QSignalSpy errorSpy(waitState, &QGCState::error);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);

    QVERIFY(startAndWaitForEntered(&machine, waitState));
    QVERIFY(waitState->connectCalled);

    // Only the first call may have an effect
    for (int i = 0; i < calls; i++) {
        if (fail) {
            waitState->triggerFailed();
        } else {
            waitState->triggerComplete();
        }
    }

    QVERIFY(spyTriggered(finishedSpy, TestTimeout::shortMs()));
    QCOMPARE(completedSpy.count(), fail ? 0 : 1);
    QCOMPARE(advanceSpy.count(), fail ? 0 : 1);
    QCOMPARE(errorSpy.count(), fail ? 1 : 0);
    QVERIFY(waitState->disconnectCalled);
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

    QVERIFY(startAndWaitForEntered(&machine, waitState));

    // Zero-timeout state arms no timer; a single event-loop flush is enough
    // to prove no spurious timeout fires.
    QCoreApplication::processEvents();
    QCOMPARE(timeoutSpy.count(), 0);

    // Complete manually to end the test
    waitState->triggerComplete();
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
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);

    QVERIFY(startAndWaitForEntered(&machine, waitState));

    // Complete before timeout
    QTimer::singleShot(50, waitState, [waitState]() {
        waitState->triggerComplete();
    });

    QVERIFY(spyTriggered(finishedSpy, TestTimeout::shortMs()));
    QCOMPARE(completedSpy.count(), 1);
    QCOMPARE(timeoutSpy.count(), 0);  // Timeout should have been cancelled
}

#include "WaitStateBaseTest.moc"

UT_REGISTER_TEST(WaitStateBaseTest, TestLabel::Unit, TestLabel::Utilities)
