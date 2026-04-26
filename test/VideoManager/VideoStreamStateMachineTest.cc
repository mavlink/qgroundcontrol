#include "VideoStreamStateMachineTest.h"

#include <QtCore/QCoreApplication>
#include <QtTest/QSignalSpy>

#include "FakeVideoReceiver.h"
#include "VideoStreamStateMachine.h"

using State = VideoStreamStateMachine::State;

namespace {

/// Wait until the FSM has started AND settled on @p target, or timeout.
///
/// The isRunning() check matters on the very first `waitForState(Idle)` of a
/// test: `_currentState` is initialized to Idle at construction, so a naive
/// "state == target" check returns true before QStateMachine::start() has
/// actually entered Idle — any signal emitted in the window between
/// `fsm.start()` and the real entry is silently dropped.
bool waitForState(VideoStreamStateMachine& fsm, State target, int timeoutMs = 2000)
{
    QElapsedTimer timer;
    timer.start();
    // Pump once so `start()` has a chance to post its initial entry event
    // and flip `isRunning()` to true before we evaluate the first predicate.
    QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
    // `Failed` is a QGCFinalState: once entered, the machine stops running,
    // so the isRunning() check has to be dropped for the terminal target.
    const auto satisfied = [&]() {
        if (target == State::Failed)
            return fsm.state() == target;
        return fsm.isRunning() && fsm.state() == target;
    };
    while (!satisfied()) {
        if (timer.elapsed() > timeoutMs)
            return false;
        QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents, 50);
    }
    return true;
}

/// Drive the FSM from Idle all the way to Streaming.
///
/// With the authoritative FSM, Starting onEntry calls receiver->start(), and
/// Stopping onEntry calls receiver->stop() — tests must NOT call these
/// manually. The helper only drives external events: requestStart() and
/// emitFirstFrame().
bool driveToStreaming(VideoStreamStateMachine& fsm, FakeVideoReceiver& rx)
{
    fsm.start();
    if (!waitForState(fsm, State::Idle))
        return false;
    fsm.requestStart();
    // FSM onEntry calls rx.start() → rx emits receiverStarted → Connected.
    if (!waitForState(fsm, State::Connected))
        return false;
    rx.emitFirstFrame();
    return waitForState(fsm, State::Streaming);
}

}  // namespace

// ════════════════════════════════════════════════════════════════════════
// Lifecycle path
// ════════════════════════════════════════════════════════════════════════

void VideoStreamStateMachineTest::_testInitialStateIsIdle()
{
    VideoStreamStateMachine fsm(QStringLiteral("LifecycleFSM"), {});
    QCOMPARE(fsm.state(), State::Idle);

    fsm.start();
    QVERIFY(waitForState(fsm, State::Idle));
}

void VideoStreamStateMachineTest::_testStartTransitionsToStarting()
{
    // Use an async fake so the FSM stays in Starting long enough to observe.
    // The default sync fake would immediately transition to Connected.
    VideoStreamStateMachine::Policy policy;
    policy.startingTimeoutMs = 2000;
    VideoStreamStateMachine fsm(QStringLiteral("StartTest"), policy);
    FakeVideoReceiver rx;
    rx.setAsyncDelayMs(200);  // defer receiverStarted so Starting is observable
    QVERIFY(fsm.bind(&rx));
    fsm.start();
    QVERIFY(waitForState(fsm, State::Idle));

    fsm.requestStart();
    QVERIFY(waitForState(fsm, State::Starting));
    // Let it settle to Connected so the FSM ends up in a clean state.
    QVERIFY(waitForState(fsm, State::Connected));
}

void VideoStreamStateMachineTest::_testReceiverStartedTransitionsToConnected()
{
    // FSM's Starting onEntry drives receiver->start(); we just wait for Connected.
    VideoStreamStateMachine fsm(QStringLiteral("StartedTest"), {});
    FakeVideoReceiver rx;
    fsm.bind(&rx);
    fsm.start();
    QVERIFY(waitForState(fsm, State::Idle));

    fsm.requestStart();
    QVERIFY(waitForState(fsm, State::Connected));
}

void VideoStreamStateMachineTest::_testFirstFrameTransitionsToStreaming()
{
    VideoStreamStateMachine fsm(QStringLiteral("FirstFrameTest"), {});
    FakeVideoReceiver rx;
    fsm.bind(&rx);
    QVERIFY(driveToStreaming(fsm, rx));
    QCOMPARE(fsm.state(), State::Streaming);
}

void VideoStreamStateMachineTest::_testPauseResume()
{
    VideoStreamStateMachine fsm(QStringLiteral("PauseResumeTest"), {});
    FakeVideoReceiver rx;
    fsm.bind(&rx);
    QVERIFY(driveToStreaming(fsm, rx));

    rx.pause();
    QVERIFY(waitForState(fsm, State::Paused));

    rx.resume();
    QVERIFY(waitForState(fsm, State::Streaming));
}

void VideoStreamStateMachineTest::_testStopFromStreamingDrainsThroughStopping()
{
    VideoStreamStateMachine fsm(QStringLiteral("StopDrainTest"), {});
    FakeVideoReceiver rx;
    rx.setAsyncDelayMs(20);  // defer receiverStopped so Stopping is observable
    fsm.bind(&rx);
    QVERIFY(driveToStreaming(fsm, rx));

    QSignalSpy stateSpy(&fsm, &VideoStreamStateMachine::stateChanged);

    // FSM Stopping onEntry calls rx.stop() automatically — do not call it here.
    fsm.requestStop();

    QVERIFY(waitForState(fsm, State::Stopping));
    QVERIFY(waitForState(fsm, State::Idle));

    // Verify we transited through Stopping, not straight to Idle.
    bool sawStopping = false;
    for (const auto& args : stateSpy) {
        if (args.value(0).value<State>() == State::Stopping)
            sawStopping = true;
    }
    QVERIFY(sawStopping);
}

void VideoStreamStateMachineTest::_testReceiverStoppedReturnsToIdle()
{
    VideoStreamStateMachine fsm(QStringLiteral("StoppedToIdleTest"), {});
    FakeVideoReceiver rx;
    fsm.bind(&rx);
    QVERIFY(driveToStreaming(fsm, rx));

    // FSM Stopping onEntry calls rx.stop() automatically.
    fsm.requestStop();
    QVERIFY(waitForState(fsm, State::Idle));
}

// ════════════════════════════════════════════════════════════════════════
// Error handling
// ════════════════════════════════════════════════════════════════════════

void VideoStreamStateMachineTest::_testFatalErrorWithSoftReconnectGoesToReconnecting()
{
    VideoStreamStateMachine::Policy policy;
    policy.allowSoftReconnect = true;
    VideoStreamStateMachine fsm(QStringLiteral("SoftReconnectTest"), policy);
    FakeVideoReceiver rx;
    fsm.bind(&rx);
    QVERIFY(driveToStreaming(fsm, rx));

    rx.emitReceiverError(VideoReceiver::ErrorCategory::Fatal, QStringLiteral("socket gone"));
    QVERIFY(waitForState(fsm, State::Reconnecting));
}

void VideoStreamStateMachineTest::_testFatalErrorWithoutSoftReconnectGoesToFailed()
{
    VideoStreamStateMachine::Policy policy;
    policy.allowSoftReconnect = false;
    VideoStreamStateMachine fsm(QStringLiteral("HardFailTest"), policy);
    FakeVideoReceiver rx;
    fsm.bind(&rx);
    QVERIFY(driveToStreaming(fsm, rx));

    rx.emitReceiverError(VideoReceiver::ErrorCategory::Fatal, QStringLiteral("bad URI"));
    QVERIFY(waitForState(fsm, State::Failed));
}

void VideoStreamStateMachineTest::_testTransientErrorDoesNotChangeState()
{
    VideoStreamStateMachine fsm(QStringLiteral("TransientErrorTest"), {});
    FakeVideoReceiver rx;
    fsm.bind(&rx);
    QVERIFY(driveToStreaming(fsm, rx));

    QSignalSpy stateSpy(&fsm, &VideoStreamStateMachine::stateChanged);
    rx.emitReceiverError(VideoReceiver::ErrorCategory::Transient, QStringLiteral("hiccup"));

    // Give the event loop a chance to dispatch anything the FSM may have queued.
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    QCOMPARE(stateSpy.count(), 0);
    QCOMPARE(fsm.state(), State::Streaming);
}

void VideoStreamStateMachineTest::_testReconnectingRecoversOnReceiverStarted()
{
    VideoStreamStateMachine fsm(QStringLiteral("ReconnectRecoveryTest"), {});
    FakeVideoReceiver rx;
    fsm.bind(&rx);
    QVERIFY(driveToStreaming(fsm, rx));

    rx.emitReceiverError(VideoReceiver::ErrorCategory::Fatal, QStringLiteral("drop"));
    QVERIFY(waitForState(fsm, State::Reconnecting));

    // Simulate the retry loop succeeding — receiver emits receiverStarted again.
    emit rx.receiverStarted();  // direct emission: FSM drives the receiver, not vice versa
    QVERIFY(waitForState(fsm, State::Connected));
}

// ════════════════════════════════════════════════════════════════════════
// Timeouts
// ════════════════════════════════════════════════════════════════════════

void VideoStreamStateMachineTest::_testStartingTimeoutGoesToFailed()
{
    // Isolate the startingTimeout watchdog: configure the fake to defer all
    // start emissions past the timeout (setAsyncDelayMs > startingTimeoutMs) and
    // disable soft reconnect so any stray hard-failure path also resolves to
    // Failed (not Reconnecting). The timeout transition must fire.
    VideoStreamStateMachine::Policy policy;
    policy.startingTimeoutMs = 50;
    policy.allowSoftReconnect = false;
    VideoStreamStateMachine fsm(QStringLiteral("StartTimeoutTest"), policy);
    FakeVideoReceiver rx;
    rx.setAsyncDelayMs(500);  // receiver primitives arrive well after the watchdog
    fsm.bind(&rx);
    fsm.start();
    QVERIFY(waitForState(fsm, State::Idle));

    fsm.requestStart();
    // FSM onEntry called rx.start() which deferred its emissions.
    // The startingTimeoutMs watchdog must trip before they arrive.
    QVERIFY(waitForState(fsm, State::Failed, 500));
}

void VideoStreamStateMachineTest::_testStoppingTimeoutGoesToIdle()
{
    VideoStreamStateMachine::Policy policy;
    policy.stoppingTimeoutMs = 50;
    VideoStreamStateMachine fsm(QStringLiteral("StopTimeoutTest"), policy);
    FakeVideoReceiver rx;
    fsm.bind(&rx);
    QVERIFY(driveToStreaming(fsm, rx));

    // Enable long async delay only after Streaming — so rx.stop()'s
    // receiverStopped is deferred past the FSM's Stopping watchdog.
    rx.setAsyncDelayMs(10000);
    fsm.requestStop();
    QVERIFY(waitForState(fsm, State::Stopping));
    QVERIFY(waitForState(fsm, State::Idle, 500));
}

// ════════════════════════════════════════════════════════════════════════
// Binding
// ════════════════════════════════════════════════════════════════════════

void VideoStreamStateMachineTest::_testBindRejectedOutsideIdle()
{
    VideoStreamStateMachine::Policy policy;
    policy.startingTimeoutMs = 2000;
    VideoStreamStateMachine fsm(QStringLiteral("BindRejectedTest"), policy);
    FakeVideoReceiver rx;
    rx.setAsyncDelayMs(200);  // stay in Starting long enough for the assertion
    fsm.bind(&rx);
    fsm.start();
    QVERIFY(waitForState(fsm, State::Idle));
    fsm.requestStart();
    QVERIFY(waitForState(fsm, State::Starting));

    FakeVideoReceiver rx2;
    QVERIFY(!fsm.bind(&rx2));
    QCOMPARE(fsm.receiver(), &rx);
}

void VideoStreamStateMachineTest::_testUnbindClearsReceiverConnections()
{
    VideoStreamStateMachine fsm(QStringLiteral("UnbindTest"), {});
    FakeVideoReceiver rx;
    fsm.bind(&rx);
    fsm.start();
    QVERIFY(waitForState(fsm, State::Idle));

    QVERIFY(fsm.unbind());
    QCOMPARE(fsm.receiver(), nullptr);

    // After unbind, receiver's primitive signals should no longer drive the FSM.
    QSignalSpy stateSpy(&fsm, &VideoStreamStateMachine::stateChanged);
    emit rx.receiverStarted();
    QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    QCOMPARE(stateSpy.count(), 0);
    QCOMPARE(fsm.state(), State::Idle);
}

UT_REGISTER_TEST(VideoStreamStateMachineTest, TestLabel::Unit)
