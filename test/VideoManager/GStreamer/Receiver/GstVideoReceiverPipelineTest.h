#pragma once

#include "GstVideoReceiverTestBase.h"

/// GstVideoReceiver's pipeline: its states, display, restarts, decoders and screenshots.
class GstVideoReceiverPipelineTest : public GstVideoReceiverTestBase
{
    Q_OBJECT

private slots:
#ifdef QGC_GST_STREAMING
    void _testStreamingStatesAndDisplay();
    void _testStaticSourcePadWaitsForData();
    void _testDecodesMockLinkStreams_data();
    void _testDecodesMockLinkStreams();
    void _testConfigChangeRestarts();
    void _testOutputDetachKeepsPipeline();
    void _testRestartRebuildsVideoSink();
    void _testDecoderQosDisabled();
    void _testDecoderDelayFollowsSourceLiveness_data();
    void _testDecoderDelayFollowsSourceLiveness();
    void _testDisplaySinkHasLatencyOnceLinked();
    void _testFailingHardwareDecoderGivesWayToTheNext();
    void _testFailedDecoderLeavesAutoplugOnlyForAnother_data();
    void _testFailedDecoderLeavesAutoplugOnlyForAnother();
    void _testDecoderInsertionFailureStopsStream_data();
    void _testDecoderInsertionFailureStopsStream();
    void _testScreenshotSavesDisplayedFrame();
    void _testDeactivateReleasesDisplayedFrame();
    void _testDecodesH265Stream();
    void _testResolutionChangeUpdatesVideoSize();
#endif
};
