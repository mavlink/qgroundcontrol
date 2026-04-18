#pragma once

#include "UnitTest.h"

class GStreamerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void init() override;

    void _testIsValidRtspUri();
    void _testIsHardwareDecoderFactory();
    void _testSetCodecPrioritiesDefault();
    void _testSetCodecPrioritiesSoftware();
    void _testSetCodecPrioritiesHardware();
    void _testRedirectGLibLogging();
    void _testVerifyRequiredPlugins();
    void _testEnvironmentSetup();
    void _testCompleteInit();
    void _testCreateVideoReceiver();
    void _testPipelineSmokeTest();
    void _testRuntimeVersionCheck();
    void _testAppsinkFrameDelivery();
    void _testGray8FormatMapping();
    void _testGray16FormatMapping();
    void _testGray8PipelineEndToEnd();

    // #5 Level 1: DMA-BUF caps advertising. Validates that the DMA_DRM variant
    // (GStreamer 1.24+) is well-formed and accepted by gst_caps_from_string.
    void _testDmaBufDrmCapsWellFormed();
};
