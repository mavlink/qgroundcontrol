#pragma once

#include <functional>

#include "UnitTest.h"

typedef struct _GstElement GstElement;
class VideoReceiver;

/// Drives GstVideoReceiver end to end against an in-process H.264 RTP/UDP stream on loopback.
class GstVideoReceiverTest : public UnitTest
{
    Q_OBJECT

private slots:
#ifdef QGC_GST_STREAMING
    void initTestCase() override;
    void init() override;

    void _testStopAfterUriClearedTearsDown();
    void _testFailedStartLeavesCleanState();
    void _testVideoSinkRefcountStableAcrossRestarts();
    void _testDecodesMockLinkStreams_data();
    void _testDecodesMockLinkStreams();
    void _testRecordingPlayableWhileInProgress_data();
    void _testRecordingPlayableWhileInProgress();
    void _testDestroyWhileRecordingFinalizesFile();
    void _testSecondRecordingIsNotTruncated();
    void _testReconnectBacksOffAfterFailedStart();
    void _testStreamLossRestartPolicy_data();
    void _testStreamLossRestartPolicy();
    void _testDecodingRequestedBeforeStreamStarts();
    void _testStopDecodingBeforeFirstFrame();
    void _testStopWhileStoppingRecordingFinalizesFile();
    void _testRecordingErrorKeepsStreaming();
    void _testFailedRecordingStartLeavesBranchUsable();
    void _testRecordingStartKeepsPipelinePlaying();
    void _testDecoderQosDisabled();
    void _testDecoderNameReported();
    void _testVideoSizeFromNegotiatedCaps();
    void _testScreenshotSavesDisplayedFrame();
    void _testStopReleasesDisplayedFrame();
    void _testBusDoesNotQueueMessages();
#else
    void _testGStreamerDisabled();
#endif

#ifdef QGC_GST_STREAMING
private:
    /// The two warnings of a start that fails on a bound port.
    void _expectFailedStartWarnings();

    /// Runs @p task with @p receiver's pipeline on the worker thread that owns it, and waits for it.
    static bool _runOnPipeline(VideoReceiver* receiver, const std::function<void(GstElement* pipeline)>& task);
#endif
};
