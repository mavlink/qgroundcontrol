#pragma once

#include "GStreamerTestBase.h"

/// What every VideoReceiver does with a stream, whichever backend plays it. Each case runs against the GStreamer and
/// the Qt Multimedia receiver, and the FFmpeg receiver where it is built, with the same H.264 RTP stream sent to a UDP
/// port on loopback.
class VideoReceiverContractTest : public GStreamerTestBase
{
    Q_OBJECT

private:
    /// Lets pass what the backend of the current row logs while it plays and records a stream.
    void _ignoreBackendLogs();

private slots:
    void _testListensUntilStreamStarts_data();
    void _testListensUntilStreamStarts();
    void _testIncompleteAddressStops_data();
    void _testIncompleteAddressStops();
    void _testStreamLossRestartPolicy_data();
    void _testStreamLossRestartPolicy();
    void _testRestartReopensPort_data();
    void _testRestartReopensPort();
    void _testDeactivateEndsRecording_data();
    void _testDeactivateEndsRecording();
    void _testStreamStoppedForGoodEndsRecording_data();
    void _testStreamStoppedForGoodEndsRecording();
    void _testRecordingBeforeFirstFrame_data();
    void _testRecordingBeforeFirstFrame();
};
