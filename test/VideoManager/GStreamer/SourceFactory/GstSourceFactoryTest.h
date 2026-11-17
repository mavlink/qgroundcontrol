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
    void _testSourceFactoryRtspUdpBufferSize();
    void _testSourceFactoryRtspTimestampsTcpPackets();
    void _testSourceFactoryRtspSelectsOnlyVideoStreams_data();
    void _testSourceFactoryRtspSelectsOnlyVideoStreams();
    void _testSourceFactoryRtspAdvertisesKeyframeRequests_data();
    void _testSourceFactoryRtspAdvertisesKeyframeRequests();
    void _testSourceFactoryRtspCredentials();
    void _testSourceFactoryRejectsBadUri();
    void _testSourceFactoryTcpMpegTs();
    void _testSourceFactoryRejectsBadTcpUri();
    void _testSourceFactoryUdpRtp_data();
    void _testSourceFactoryUdpRtp();
    void _testSourceFactoryUdpListener_data();
    void _testSourceFactoryUdpListener();
    void _testParameterSetGateHoldsUntilCompleteSet();
    void _testParameterSetGatePassesLengthPrefixedOutput();
    void _testSourceFactoryUdpMpegTs();
    void _testSourceFactoryMpegTsLatency_data();
    void _testSourceFactoryMpegTsLatency();
    void _testSourceFactorySchemeCaseInsensitive();
    void _testTransportFor_data();
    void _testTransportFor();
    void _testSourceFactoryNegativeLatencyClamped();
    void _testSourceFactoryDynamicLinkSkipsNonVideoRtp();
    void _testSourceFactoryRtspDepayloaderRequestsKeyframes_data();
    void _testSourceFactoryRtspDepayloaderRequestsKeyframes();
    void _testSourceFactoryRtspPlugsCodec_data();
    void _testSourceFactoryRtspPlugsCodec();
    void _testSourceFactoryRtspPadAddedLeavesBinStateLock();
    void _testSourceFactoryRtspPushesAccessUnitsAtOnce_data();
    void _testSourceFactoryRtspPushesAccessUnitsAtOnce();
    void _testSourceFactoryRtspFollowsStreamToNewPad_data();
    void _testSourceFactoryRtspFollowsStreamToNewPad();
#endif
};
