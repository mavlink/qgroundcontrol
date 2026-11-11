#pragma once

#include "GStreamerTestBase.h"

/// GStreamer::SourceFactory without a stream: the source bin built for each URI scheme, its jitter-buffer and
/// retransmission policy, RTSP stream selection, dynamic-pad linking and the parameter-set gate.
class GstSourceFactoryTest : public GStreamerTestBase
{
    Q_OBJECT

private slots:
#ifdef QGC_GST_STREAMING
    void _testIsValidRtspUri();
    void _testSourceFactoryUdpRtpJitterBuffer();
    void _testSourceFactoryCountsRtpPacketsWithoutJitterBuffer_data();
    void _testSourceFactoryCountsRtpPacketsWithoutJitterBuffer();
    void _testSourceFactoryNoRetransmission();
    void _testSourceFactoryRtspJitterBufferPolicy_data();
    void _testSourceFactoryRtspJitterBufferPolicy();
    void _testSourceFactoryRtspSelectsOnlyVideoStreams_data();
    void _testSourceFactoryRtspSelectsOnlyVideoStreams();
    void _testSourceFactoryRtspCredentials();
    void _testSourceFactoryRejectsBadUri();
    void _testSourceFactoryTcpMpegTs();
    void _testSourceFactoryRejectsBadTcpUri();
    void _testSourceFactoryUdpRtp_data();
    void _testSourceFactoryUdpRtp();
    void _testParameterSetGateHoldsUntilCompleteSet();
    void _testParameterSetGatePassesLengthPrefixedOutput();
    void _testSourceFactoryUdpMpegTs();
    void _testSourceFactorySchemeCaseInsensitive();
    void _testTransportFor_data();
    void _testTransportFor();
    void _testSourceFactoryNegativeLatencyClamped();
    void _testSourceFactoryDynamicRtpLinkFailureCleansJitterBuffer();
    void _testSourceFactoryDynamicLinkSkipsNonVideoRtp();
    void _testSourceFactoryRtspDepayloaderRequestsKeyframes_data();
    void _testSourceFactoryRtspDepayloaderRequestsKeyframes();
#endif
};
