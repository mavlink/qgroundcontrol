#pragma once

#include "UnitTest.h"

class VideoStreamTest : public UnitTest
{
    Q_OBJECT

private slots:
    void init() override;

    // SessionState FSM
    void _testStartTransitionsToStarting();
    void _testStopFromRunningTransitionsToStopping();
    void _testStopFromStartingTransitionsToStopping();
    void _testFailedStartAutoRestartsOnce();
    void _testRestartFromRunningSetsPendingAndStops();
    void _testRestartFromStoppedCallsStartDirectly();
    void _testCannotStartWhenAlreadyStarting();
    void _testCannotStartWhenAlreadyRunning();

    // _wantRunning semantics
    void _testStartSetsWantRunning();
    void _testStopClearsWantRunning();
    void _testRestartSetsWantRunningBeforeStop();
    void _testAutoReconnectWhenWantRunningAfterStopCompleted();
    void _testNoAutoReconnectAfterExplicitStop();

    // Receiver/source routing
    void _testVideoSourceCatalogOwnsSettingsClassification();
    void _testVideoSourceAvailabilityOwnsRuntimeEnumeration();
    void _testSourceDescriptorCarriesPolicy();
    void _testVideoSourceResolverPrefersAutoStreamAndLimitsWriteback();
    void _testVideoSourceControllerOwnsResolutionAndWriteback();
    void _testVideoStreamUsesSourceMetadataSnapshot();
    void _testVideoStreamExposesSnapshotMetadataForQml();
    void _testGStreamerIngestSourcesUseQtDisplayReceiver();
    void _testLocalCameraSourcesUseQtDisplayReceiver();
    void _testGStreamerIngestPlaybackDeviceIsReceiverOnly();
    void _testQtFfmpegRuntimePolicyAppliesDefaults();
    void _testQtFfmpegRuntimePolicyPreservesExistingEnvironment();
    void _testQtPlaybackTrackPolicyDisablesUnusedTracksForPlayback();
    void _testVideoPlaybackRuntimeAppliesResolvedInput();
    void _testStartRefreshesIngestPlaybackInput();
    void _testReconnectRefreshesIngestPlaybackInput();
    void _testVideoIngestControllerRefreshesIngestInput();
    void _testVideoIngestControllerMarksTransportKind();
    void _testVideoStreamAggregateMonitorTracksNonThermalStreams();
    void _testVideoUvcControllerActivatesAndDeactivatesUvcStream();
    void _testVideoStatsMovedOffStreamMetaObject();
    void _testVideoStreamDoesNotOwnSettingsResolutionApi();
    void _testLifecycleControllerOwnsFsmSignals();
    void _testVideoStreamSessionOwnsLifecycleResources();
    void _testFrameDeliveryAccessorIsAvailable();
    void _testSourceModeSwitchPreservesReceiver();
    void _testSinkPreservedAcrossSourceModeSwitch();
    void _testSinkChangeRestartIsReceiverOwned();

    // Pending sink
    void _testRegisterSinkBeforeReceiverDeferred();
    void _testDeferredSinkFlushedOnReceiverCreate();

    // Phase 1 invariants
    void _testStopDrainsAsyncReceiverBeforeDestroy();
    void _testStopDrainTimeoutDoesNotHang();
    void _testIdempotentStopReturnsOk();
    void _testReceiverErrorSurfacesAsLastError();
    void _testStartingStateObservableUnderAsync();
    void _testRecordingLifecycleEmitsSignals();
    void _testStateMirrorRemovedDelegatesToReceiver();
    void _testNoReceiverMeansAccessorsReturnFalse();

    // Formal SessionState FSM
    void _testLifecyclePolicyMapsRolesAndStates();
    void _testSessionStateChangedSignalEmitted();
    void _testSelfLoopDoesNotEmitSignal();
    void _testIsLegalTransitionTable();
    void _testFullLifecycleSignalSequence();

    // Recording integration (mock recorder injected via setRecorderForTest)
    void _testRecordingEmitsStartedStoppedSignals();
    void _testRecordingFailsWithoutFrameDelivery();
    void _testRecordingIdempotentStop();

    // Second-pass review new tests
    void _testReconnectBackoffDoubles();          ///< #13 exponential backoff
    void _testStreamInfoSwapReconnectsSignal();   ///< #2 stale infoChanged fix
    void _testSourceModeSwitchPreservesSink();    ///< #12 sink preservation

    // Shadow FSM integration (PR #5)
    /// Drives a full start → run → stop lifecycle and verifies the FSM
    /// tracks `sessionState` according to the documented mapping, and that
    /// VideoStream::fsmStateChanged fires on every transition.
    void _testShadowFsmTracksSessionStateLifecycle();
    /// Teardown emits a synthetic Idle transition so QML bindings on
    /// `fsmState` reset when the FSM is destroyed.
    void _testShadowFsmEmitsIdleOnTeardown();
    /// A Fatal receiver error routes the FSM through its configured failure
    /// path (Reconnecting for Primary/Thermal, Failed for Dynamic).
    void _testShadowFsmRoleBasedFailurePath();
};
