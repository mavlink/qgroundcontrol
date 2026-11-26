#pragma once

#include "UnitTest.h"

/// GstVideoReceiver's components on their own, without a running pipeline.
class GstVideoReceiverComponentsTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testPipelineDotStaysInDirectory_data();
    void _testPipelineDotStaysInDirectory();
    void _testPipelineDotOmitsElementProperties();
    void _testPipelineNamedAfterReceiver_data();
    void _testPipelineNamedAfterReceiver();
    void _testPresentedSize_data();
    void _testPresentedSize();
    void _testBusPump();
    void _testStatsProbe();
    void _testStatsProbeCountsRtpPackets();
    void _testRecordingContainerHoldsCodec_data();
    void _testRecordingContainerHoldsCodec();
    void _testRecordingNamesCodecNoContainerHolds();
};
