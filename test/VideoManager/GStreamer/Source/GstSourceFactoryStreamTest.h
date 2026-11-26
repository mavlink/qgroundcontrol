#pragma once

#include "GStreamerTestBase.h"

/// GStreamer::SourceFactory source bins receiving real streams on loopback: RTP and MPEG-TS over UDP and an RTSP
/// session; and the udpsrc they use, stopped right after it starts.
class GstSourceFactoryStreamTest : public GStreamerTestBase
{
    Q_OBJECT

private slots:
    void _testSourceFactoryRtspSetsUpOnlyVideoStream();
    void _testSourceFactoryRtspEndsSessionWithoutPause();
    void _testSourceFactoryDropsRtpUntilParameterSets();
    void _testStatsCountPacketsDroppedOnLatency();
    void _testMpegTsStreamObjectCarriesParsedCaps();
    void _testMpegTsFrameLeavesBeforeTheNextArrives_data();
    void _testMpegTsFrameLeavesBeforeTheNextArrives();
    void _testMpegTsPacketWithinFrameEndsAlignment();
    void _testUdpSourceRefusesPortInUse();
    void _testUdpSourceStopsRightAfterPlaying();
};
