#pragma once

#include "UnitTest.h"

/// GstVideoReceiver's components on their own, without a running pipeline.
class GstVideoReceiverComponentsTest : public UnitTest
{
    Q_OBJECT

private slots:
#ifdef QGC_GST_STREAMING
    void _testStallDetection_data();
    void _testStallDetection();
    void _testFirstFrameGrace();
    void _testDecoderSilenceCountsFromInsertion();
    void _testSourceTimeoutStartsAtFirstCheck();
    void _testFinalizerSettlesOnlyFromArmedRecording_data();
    void _testFinalizerSettlesOnlyFromArmedRecording();
    void _testFinalizerKeepsFirstOutcome();
    void _testPipelineDotStaysInDirectory_data();
    void _testPipelineDotStaysInDirectory();
    void _testPipelineNamedAfterReceiver_data();
    void _testPipelineNamedAfterReceiver();
    void _testPresentedSize_data();
    void _testPresentedSize();
    void _testBusPump();
    void _testStatsProbe();
#else
    void _testGStreamerDisabled();
#endif
};
