#pragma once

#include "GStreamerTestBase.h"

/// The boot self-test (GStreamer::SelfTest::run()) against the GStreamer the tests run on: it passes, and a hardware
/// decoder that fails mid-stream leaves it to decode with software decoders, restoring the decoder ranks.
class GstSelfTestPipelineTest : public GStreamerTestBase
{
    Q_OBJECT

private slots:
#ifdef QGC_GST_STREAMING
    void _testSystemRuntimePasses();
    void _testFailingHardwareDecoderFallsBackToSoftware();
#endif
};
