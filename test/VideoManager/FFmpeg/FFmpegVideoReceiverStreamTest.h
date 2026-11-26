#pragma once

#include "GStreamerTestBase.h"

class FFmpegVideoReceiver;

/// FFmpegVideoReceiver against MockVideoStreamServer's streams, which need GStreamer to serve them: the kinds of stream
/// it plays, the stop of a stream that is still connecting, and its recordings, also across a reconnect, which the test
/// reads back with libavformat. Also what it opens for a URI, and that the video backend setting chooses it.
/// VideoReceiverContractTest holds it to what every receiver does.
class FFmpegVideoReceiverStreamTest : public GStreamerTestBase
{
    Q_OBJECT

public:
    /// Has @p receiver sample its stream five times a second rather than once: the tests wait for its statistics,
    /// which take two samples, and for the timeouts the samples check.
    static void sampleOften(FFmpegVideoReceiver& receiver);

private:
    /// No session of @p receiver's runs any more: their threads have ended.
    static bool _sessionsEnded(const FFmpegVideoReceiver& receiver);
    void _ignoreRtspServerLogs();

private slots:
    void _testChosenByBackendSetting();
    void _testPlaysMockStreams_data();
    void _testPlaysMockStreams();
    void _testStopWhileConnectingIsPrompt_data();
    void _testStopWhileConnectingIsPrompt();
    void _testRecordsStream_data();
    void _testRecordsStream();
    void _testRecordingContinuesAfterReconnect();
};
