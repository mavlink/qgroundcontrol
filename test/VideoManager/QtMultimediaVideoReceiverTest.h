#pragma once

#include "GStreamerTestBase.h"

/// QtMultimediaVideoReceiver against MockVideoStreamServer's streams, which need GStreamer to serve them.
class QtMultimediaVideoReceiverTest : public GStreamerTestBase
{
    Q_OBJECT

private:
    /// Qt's FFmpeg player warns that a live stream can't loop, though nothing asked it to, and tells when the machine
    /// has no hardware decoder for the stream.
    void _ignorePlayerLogs();
    /// Qt reports each format this platform can't encode, which the recorder moves past.
    void _ignoreEncoderFallbacks();

private slots:
    void _testPlayerSource_data();
    void _testPlayerSource();
    void _testUnplayableUri();
    void _testRecordingNeedsActiveStream();
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
