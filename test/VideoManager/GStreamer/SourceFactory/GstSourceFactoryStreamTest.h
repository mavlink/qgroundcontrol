#pragma once

#include "GStreamerTestBase.h"

/// GStreamer::SourceFactory source bins receiving real streams on loopback: RTP over UDP and an RTSP session; and the
/// udpsrc they use, stopped right after it starts.
class GstSourceFactoryStreamTest : public GStreamerTestBase
{
    Q_OBJECT

private slots:
#ifdef QGC_GST_STREAMING
    void _testSourceFactoryRtspSetsUpOnlyVideoStream();
    void _testSourceFactoryDropsRtpUntilParameterSets();
    void _testUdpSourceStopsRightAfterPlaying();
#endif
};
