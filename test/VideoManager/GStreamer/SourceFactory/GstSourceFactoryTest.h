#pragma once

#include "GStreamerTestBase.h"

/// GStreamer::SourceFactory without a stream: the source bin built for each URI scheme, its jitter-buffer and
/// retransmission policy, RTSP stream selection, dynamic-pad linking and the parameter-set gate.
class GstSourceFactoryTest : public GStreamerTestBase
{
    Q_OBJECT

private slots:
#ifdef QGC_GST_STREAMING
    void _testSourceFactoryUdpRtpJitterBuffer();
    void _testSourceFactoryJitterBufferNone();
    void _testSourceFactoryNoRetransmission();
    void _testSourceFactoryRtspJitterBufferPolicy_data();
    void _testSourceFactoryRtspJitterBufferPolicy();
    void _testSourceFactoryRtspSelectsOnlyVideoStreams_data();
    void _testSourceFactoryRtspSelectsOnlyVideoStreams();
    void _testSourceFactoryRejectsBadUri();
    void _testSourceFactoryTcpMpegTs();
    void _testSourceFactoryRejectsBadTcpUri();
    void _testSourceFactoryUdp265();
    void _testSourceFactoryUdpH264Caps();
    void _testParameterSetGateHoldsUntilCompleteSet();
    void _testParameterSetGatePassesLengthPrefixedOutput();
    void _testSourceFactoryUdpMpegTs();
    void _testSourceFactorySchemeCaseInsensitive();
    void _testSourceFactoryNegativeLatencyClamped();
    void _testSourceFactoryDynamicRtpLinkFailureCleansJitterBuffer();
    void _testSourceFactoryDynamicLinkSkipsNonVideoRtp();
#endif
};
