#pragma once

#include <functional>
#include <memory>

#include "GStreamerTestBase.h"

typedef struct _GstElement GstElement;
class GstVideoReceiver;

namespace VideoTestHelpers {
class FactRestorer;
}

/// Drives GstVideoReceiver end to end against in-process H.264 streams on loopback.
class GstVideoReceiverTest : public GStreamerTestBase
{
    Q_OBJECT

public:
#ifdef QGC_GST_STREAMING
    /// Runs @p task with @p receiver's pipeline (null when none runs) on the worker thread that owns it, and waits.
    static bool runOnPipeline(GstVideoReceiver* receiver, const std::function<void(GstElement* pipeline)>& task);
    static GstElement* displaySink(const GstVideoReceiver* receiver);
#endif

private slots:
#ifdef QGC_GST_STREAMING
    void initTestCase() override;
    void cleanupTestCase() override;

    void _testStreamingStatesAndDisplay();
    void _testStaticSourcePadWaitsForData();
    void _testDecodesMockLinkStreams_data();
    void _testDecodesMockLinkStreams();
    void _testConfigChangeRestarts();
    void _testOutputDetachKeepsPipeline();
    void _testRestartRebuildsVideoSink();
    void _testDecoderFailureKeepsRecording();
    void _testReconnectResumesRecordingInNextPart();
    void _testSlowRecordingDoesNotStallDisplay();
    void _testPermanentFailureStops();
    void _testReconnectBacksOffAfterFailedStart();
    void _testStreamLossRestartPolicy_data();
    void _testStreamLossRestartPolicy();
    void _testDeactivateFinishesRecording();
    void _testRecordingPlayableWhileInProgress_data();
    void _testRecordingPlayableWhileInProgress();
    void _testSecondRecordingIsNotTruncated();
    void _testRecordingSwitchWithoutDisplay_data();
    void _testRecordingSwitchWithoutDisplay();
    void _testStreamEndClosesRecordingAtOnce();
    void _testRecordingErrorKeepsStreaming();
    void _testFailedRecordingStartLeavesBranchUsable();
    void _testRecordingStartKeepsPipelinePlaying();
    void _testDecoderQosDisabled();
    void _testScreenshotSavesDisplayedFrame();
    void _testDeactivateReleasesDisplayedFrame();
    void _testDestroyWhileRecordingFinalizesFile();
    void _testDestroyWithHungRecorderIsBounded();
    void _testDecodesH265Stream();
    void _testResolutionChangeUpdatesVideoSize();
    void _testUnresponsiveRtspServer_data();
    void _testUnresponsiveRtspServer();
#endif

private:
#ifdef QGC_GST_STREAMING
    std::unique_ptr<VideoTestHelpers::FactRestorer> _settings;
#endif
};
