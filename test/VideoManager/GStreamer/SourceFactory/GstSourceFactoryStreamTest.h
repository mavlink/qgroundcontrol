#pragma once

#include "GStreamerTestBase.h"

/// GStreamer::SourceFactory source bins receiving real streams on loopback: RTP over UDP and an RTSP session.
class GstSourceFactoryStreamTest : public GStreamerTestBase
{
    Q_OBJECT

private slots:
#ifdef QGC_GST_STREAMING
    void _testSourceFactoryRtspSetsUpOnlyVideoStream();
    void _testSourceFactoryDropsRtpUntilParameterSets();
#endif
};
