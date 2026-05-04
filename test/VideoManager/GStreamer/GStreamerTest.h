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
    void _testAppsinkYuvPassthrough();
    void _testAppsinkPtsAndColorimetry();
    void _testQgcVideoSinkBinGpuZeroCopyProperty();
    void _testGlMemoryDispatch();
    void _testCapsCacheInvalidation();
    void _testGpuZeroCopyFallback();
    void _testAppsinkTeardownUnderLoad();
    void _testBridgeDispatcherFanout();
    void _testHwBufferMapTexturesGuard();
    void _testFrameCountsTelemetrySignal();
    void _testGetAppsinkAccessor();
    void _testContextBridgeRegistry();
    void _testHwBufferFactoryDispatchSystemMemory();
    void _testCpuMemcpyActiveRowStrideHandling();
    void _testQGCRhiCaptureCacheLifecycle();
    void _testColorimetryPixelFormatMapping();
    void _testColorimetryColorSpaceMapping();
    void _testColorimetryTransferMapping();
    void _testColorimetryFrameRatePropagation();
    void _testHwBufferCropMatrixIdentityWithoutMeta();
    void _testHwBufferCropMatrixFromVideoCropMeta();
    void _testApplyOrientationToFrameMapping();
    void _testAdapterFlushDropsInFlightSamples();
};
