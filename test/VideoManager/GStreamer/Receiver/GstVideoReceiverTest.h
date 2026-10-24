#pragma once

#include <functional>

#include "UnitTest.h"

typedef struct _GstElement GstElement;
class GstVideoReceiver;

/// Drives GstVideoReceiver end to end against in-process H.264 streams on loopback.
class GstVideoReceiverTest : public UnitTest
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
    void init() override;

    void _testStreamingStatesAndDisplay();
    void _testStaticSourcePadWaitsForData();
    void _testUdpSourceStopsRightAfterPlaying();
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
#else
    void _testGStreamerDisabled();
#endif
};
