#pragma once

#include "UnitTest.h"
#include "VideoStream.h"

class FakeVideoReceiver;
class VideoManager;

/// VideoManager-layer integration tests using FakeVideoReceiver-backed
/// VideoStreams. Exercises Phase 1 + refactor/video-gstreamer-unify changes:
///   * VideoStream::frameDeliveryChanged signal fires on sink registration (#2)
///   * SubtitleWriter re-wire on frameDelivery swap (#8)
///   * Per-property NOTIFY signals (Q1: hfov / thermalAspectRatio / thermalHfov)
///   * Declarative sink routing via stream->registerVideoSink() (#2, #10)
///   * VideoStreamModel reflects stream additions
///   * Aggregate state (decoding/recording/streaming/hasVideo) computed from streams
///   * videoSink flush path (deferred sink flushed after receiver creation)
///
class VideoManagerIntegrationTest : public UnitTest
{
    Q_OBJECT

private:
    /// Result of buildAndRegisterStream — the manager owns the stream;
    /// fake is owned by the stream's receiver (captured for introspection).
    struct PrimedStream
    {
        VideoStream* stream = nullptr;
        FakeVideoReceiver* fake = nullptr;
    };

    /// Build a VideoStream backed by a FakeVideoReceiver and register it with
    /// the manager's orchestrator through the explicit test hook.
    static PrimedStream buildAndRegisterStream(VideoManager& mgr, VideoStream::Role role,
                                               const QString& uri = QStringLiteral("udp://0.0.0.0:5600"));

private slots:
    void init() override;

    void _testCameraBinderDefersCameraSwapWhileRecording();
    void _testMediaServicesOwnsImageCaptureFallback();
    void _testPrimaryFrameDeliveryChangedFiresOnSinkSwap();
    void _testThermalFrameDeliveryChangedFiresOnSinkSwap();
    void _testRegisterVideoSinkRoutesByRole();
    void _testStreamModelExposesPrimaryAndThermal();
    void _testAggregateDecodingFromStreams();
    void _testOrchestratorOwnsAutoStreamConfigured();
    void _testDeferredSinkFlushedAfterStreamCreation();
    void _testCreateStreamsDoesNotPrecreateUvcWhenInactive();
    void _testQgcVideoOutputInstantiates();
    void _testUvcActivationTransfersSinkDeclaratively();  ///< #16

    // Headless frame-delivery harness: exercises the receiver→frameDelivery→QVideoSink
    // path end-to-end without a real receiver. FakeVideoReceiver synthesizes
    // QVideoFrames through its frameDelivery; the test asserts on frameDelivery counters,
    // sink.videoFrame(), and frameDelivery signals.
    void _testFrameDeliveryReachesSink();
    void _testFrameDeliverySnapshotSequenceAdvancesPerFrame();
    void _testFrameDeliveryCapturesFrameDiagnostics();
    void _testObservedFrameUpdatesDeliveryWithoutRedisplay();
    void _testQtVideoSinkRouterAppliesSinkAndObservesFrames();
    void _testQtVideoSinkRouterExposesFallbackAndRhiState();
    void _testAnnouncedFormatUpdatesVideoSizeBeforeFirstFrame();
    void _testBackpressureDropsExcessFrames();
    void _testWatchdogFiresAfterSilence();

    // #1 dynamic topology: orchestrator reconciles Role::Dynamic streams
    // against a MAVLink-reported stream list.
    void _testDynamicStreamsReconcile();

    // Stream-swap regressions. QGCVideoOutput.qml tracks the stream it
    // registered with and deregisters the old one before re-registering
    // on role swap; VideoFrameDelivery's watchdog no longer pre-stamps
    // `_lastFrameMs` on arm so slow decoder startup doesn't trip it.
    void _testRegisterNullSinkClearsFrameDelivery();
    void _testSwapSinkBetweenStreamsClearsOldFrameDelivery();
    void _testWatchdogDoesNotFireBeforeFirstFrame();

    // Dark-corner error/lifecycle paths: fatal receiver errors propagate to
    // QML via lastError; clearing the URI tears down the receiver; repeated
    // sink registrations are idempotent; destroying a registered QVideoSink
    // automatically clears the frameDelivery (no dangling pointer).
    void _testReceiverFatalErrorSurfacesViaLastError();
    void _testUriClearDestroysReceiver();
    void _testRepeatedSinkRegistrationIsIdempotent();
    void _testSinkDestructionClearsFrameDeliveryAutomatically();
};
