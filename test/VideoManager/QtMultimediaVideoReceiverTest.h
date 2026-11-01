#pragma once

#include "GStreamerTestBase.h"

/// QtMultimediaVideoReceiver against MockVideoStreamServer's streams, which need GStreamer to serve them.
class QtMultimediaVideoReceiverTest : public GStreamerTestBase
{
    Q_OBJECT

private:
    /// Qt's FFmpeg player warns that a live stream can't loop, though nothing asked it to.
    void _ignoreLoopWarning();

private slots:
    void _testPlayerSource_data();
    void _testPlayerSource();
    void _testUnplayableUri();
    void _testRecordingUnsupported();
#ifdef QGC_GST_STREAMING
    void _testPlaysMockStreams_data();
    void _testPlaysMockStreams();
    void _testRestartReopensPort();
    void _testReconnectsAfterStreamLoss();
    void _testStopsWithoutAutoReconnect();
#endif
};
