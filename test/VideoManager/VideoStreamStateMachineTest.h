#pragma once

#include "StateMachineTest.h"

/// Unit tests for VideoStreamStateMachine — the primitive-signal-driven FSM
/// that replaces VideoStream's ad-hoc session-state tracking.
///
/// All tests use FakeVideoReceiver so the FSM exercises the same
/// `receiverStarted/Stopped/Paused/Resumed/FirstFrame/Error` primitives that
/// GstVideoReceiver / QtMultimediaReceiver / UVCReceiver emit in production.
class VideoStreamStateMachineTest : public StateMachineTest
{
    Q_OBJECT

private slots:
    // ── Lifecycle path ─────────────────────────────────────────────────
    void _testInitialStateIsIdle();
    void _testStartTransitionsToStarting();
    void _testReceiverStartedTransitionsToConnected();
    void _testFirstFrameTransitionsToStreaming();
    void _testPauseResume();
    void _testStopFromStreamingDrainsThroughStopping();
    void _testReceiverStoppedReturnsToIdle();

    // ── Error handling ─────────────────────────────────────────────────
    void _testFatalErrorWithSoftReconnectGoesToReconnecting();
    void _testFatalErrorWithoutSoftReconnectGoesToFailed();
    void _testTransientErrorDoesNotChangeState();
    void _testReconnectingRecoversOnReceiverStarted();

    // ── Timeouts ───────────────────────────────────────────────────────
    void _testStartingTimeoutGoesToFailed();
    void _testStoppingTimeoutGoesToIdle();

    // ── Binding ────────────────────────────────────────────────────────
    void _testBindRejectedOutsideIdle();
    void _testUnbindClearsReceiverConnections();
};
