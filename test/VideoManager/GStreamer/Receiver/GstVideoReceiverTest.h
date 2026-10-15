#pragma once

#include "UnitTest.h"

/// Drives GstVideoReceiver end to end against an in-process H.264 RTP/UDP stream on loopback.
class GstVideoReceiverTest : public UnitTest
{
    Q_OBJECT

private slots:
    void init() override;

    void _testStartDecodeStop();
    void _testStopAfterUriClearedTearsDown();
    void _testFailedStartLeavesCleanState();
    void _testVideoSinkRefcountStableAcrossRestarts();
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
    void _testBusDoesNotQueueMessages();
};
