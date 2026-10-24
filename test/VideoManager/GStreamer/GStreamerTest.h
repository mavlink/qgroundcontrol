#pragma once

#include "GStreamerTestBase.h"

/// The GStreamer facade without a running pipeline: URI checks, decoder ranking, logging, environment and init.
class GStreamerTest : public GStreamerTestBase
{
    Q_OBJECT

private slots:
#ifdef QGC_GST_STREAMING
    void _testIsValidRtspUri();
    void _testIsHardwareDecoderFactory_data();
    void _testIsHardwareDecoderFactory();
    void _testDecoderFamilyForFactory_data();
    void _testDecoderFamilyForFactory();
    void _testSetCodecPrioritiesDefault();
    void _testSetCodecPrioritiesDefaultPrefersMatchingD3DDecoder();
    void _testSetCodecPrioritiesSkipsAbsentD3DDecoders();
    void _testD3D12RhiDisablesGpuZeroCopySink();
    void _testSetCodecPrioritiesSoftware();
    void _testSetCodecPrioritiesHardware();
    void _testRedirectGLibLogging();
    void _testConfigureDebugLoggingIsIdempotent();
    void _testVerifyRequiredPlugins();
    void _testEnvironmentSetup();
    void _testPipelineDotOmitsElementProperties();
    void _testCompleteInit();
    void _testCreateVideoReceiver();
    void _testRecordingSinkAcceptsElementaryStreams_data();
    void _testRecordingSinkAcceptsElementaryStreams();
    void _testBindDebugLevelFactRejectsNullContext();
    void _testRuntimeVersionCheck();
#endif
};
