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
    void _testFirstFrameGrace_data();
    void _testFirstFrameGrace();
    void _testDecoderSilenceCountsFromInsertion();
    void _testPipelineDotStaysInDirectory_data();
    void _testPipelineDotStaysInDirectory();
    void _testPipelineNamedAfterReceiver_data();
    void _testPipelineNamedAfterReceiver();
    void _testPresentedSize_data();
    void _testPresentedSize();
    void _testBusPump();
    void _testStatsProbe();
    void _testRtpPacketCounter_data();
    void _testRtpPacketCounter();
    void _testRtpPacketCounterDropoutFollowsPacketRate_data();
    void _testRtpPacketCounterDropoutFollowsPacketRate();
    void _testRtpPacketCounterJitter_data();
    void _testRtpPacketCounterJitter();
    void _testStatsProbeCountsRtpPackets();
    void _testRecordingContainerHoldsCodec_data();
    void _testRecordingContainerHoldsCodec();
    void _testRecordingNamesCodecNoContainerHolds();
#else
    void _testGStreamerDisabled();
#endif
};
