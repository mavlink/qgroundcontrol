#pragma once

#include "GStreamerTestBase.h"

/// The GStreamer facade without a running pipeline: URI checks, decoder classification, recording sinks and init.
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
    void _testPipelineDotOmitsElementProperties();
    void _testCompleteInit();
    void _testCreateVideoReceiver();
    void _testRecordingSinkAcceptsElementaryStreams_data();
    void _testRecordingSinkAcceptsElementaryStreams();
    void _testBindDebugLevelFactRejectsNullContext();
    void _testRuntimeVersionCheck();
#endif
};
