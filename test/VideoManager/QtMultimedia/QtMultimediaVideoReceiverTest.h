#pragma once

#include "GStreamerTestBase.h"

class QtMultimediaVideoReceiver;

/// QtMultimediaVideoReceiver against MockVideoStreamServer's streams, which need GStreamer to serve them.
class QtMultimediaVideoReceiverTest : public GStreamerTestBase
{
    Q_OBJECT

public:
    /// Has @p receiver sample its stream five times a second rather than once: the tests wait for its statistics,
    /// which take two samples, and for the timeout the samples check.
    static void sampleOften(QtMultimediaVideoReceiver& receiver);

private:
    /// Qt's FFmpeg player warns that a live stream can't loop, though nothing asked it to, and tells when the machine
    /// has no hardware decoder for the stream.
    void _ignorePlayerLogs();
    /// Qt reports each format this platform can't encode, which the recorder moves past.
    void _ignoreEncoderFallbacks();
    void _ignoreRtspServerLogs();

private slots:
    void _testPlayerSource_data();
    void _testPlayerSource();
    void _testUnplayableUri();
    void _testRecordingNeedsActiveStream();
    void _testStartWhileRecordingIsRefused();
    void _testRecordingUsesEncodingSet();
    void _testUdpListensUntilStreamStarts();
    void _testRecordingTimes_data();
    void _testRecordingTimes();
#ifdef QGC_GST_STREAMING
    void _testPlaysMockStreams_data();
    void _testPlaysMockStreams();
    void _testRestartReopensPort();
    void _testReconnectsAfterStreamLoss();
    void _testStopsWithoutAutoReconnect();
    void _testDetachingOutputDoesNotBlock();
    void _testPlaysRtspOverTcp();
    void _testRecordsStream();
#endif
};
