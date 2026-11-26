#pragma once

#include "GstVideoReceiverTestBase.h"

/// GstVideoReceiver's pipeline: its states, display, restarts, decoders and screenshots.
class GstVideoReceiverPipelineTest : public GstVideoReceiverTestBase
{
    Q_OBJECT

private slots:
    void _testStreamingStatesAndDisplay();
    void _testStaticSourcePadWaitsForData();
    void _testDecodesMockLinkStreams_data();
    void _testDecodesMockLinkStreams();
    void _testConfigChangeRestarts();
    void _testOutputDetachKeepsPipeline();
    void _testRestartRebuildsVideoSink();
    void _testDecoderQosDisabled();
    void _testDecoderDecodesVideoStreamsOnly();
    void _testDecoderThreads_data();
    void _testDecoderThreads();
    void _testDisplaySinkHasLatencyOnceLinked();
    void _testFailingHardwareDecoderGivesWayToTheNext();
    void _testSilentHardwareDecoderGivesWayAfterOneStall();
    void _testFailedDecoderLeavesAutoplugOnlyForAnother_data();
    void _testFailedDecoderLeavesAutoplugOnlyForAnother();
    void _testDecoderWaitsForTheMainWindowsSceneGraph();
    void _testLostGraphicsDeviceRestartsTheStream();
#if defined(QGC_HAS_ANY_GPU_PATH)
    void _testDecoderFailingOnALostGraphicsDeviceKeepsItsRank();
#endif
    void _testOverrunWithoutLeakLeavesKeyframeGateOpen();
    void _testKeyframeGateOpensWithoutKeyframe();
    void _testDecoderInsertionFailureKeepsStream_data();
    void _testDecoderInsertionFailureKeepsStream();
    void _testScreenshotSavesDisplayedFrame();
    void _testDeactivateReleasesDisplayedFrame();
    void _testDecodesH265Stream();
    void _testResolutionChangeUpdatesVideoSize();
    void _testMissingDecoderIsNamed();
    void _testSilentFlowErrorStaysInItsBranch_data();
    void _testSilentFlowErrorStaysInItsBranch();
};
