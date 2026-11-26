#pragma once

#include "GStreamerTestBase.h"

/// What the HwBuffers GPU paths share, without a pipeline or a GPU: context bridges, path selection, the factory, CPU
/// fallbacks, the GPU bin's choice, crop matrices and telemetry. The cases that need a GPU path are built with one.
class GstHwBuffersTest : public GStreamerTestBase
{
    Q_OBJECT

private slots:
#if defined(QGC_HAS_ANY_GPU_PATH)
    void _testContextBridgeRegistry();
    void _testContextBridgeAnswersWholePipeline();
    void _testContextBridgeServesPipelineOnceDeviceIsKnown();
    void _testRemovedDeviceIsToldToThePipelinesThatGotIt();
    void _testHwBufferLifecycleResetsNativeCaches();
    void _testQGCRhiCaptureCacheLifecycle();
    void _testHwPathRegistrySelection_data();
    void _testHwPathRegistrySelection();
    void _testHwVideoBufferCpuMapFallback();
    void _testHwVideoBufferCpuMapRefusesForeignLayout_data();
    void _testHwVideoBufferCpuMapRefusesForeignLayout();
    void _testWithPixelFormatKeepsOtherProperties();
    void _testTexturePadsPicture_data();
    void _testTexturePadsPicture();
    void _testTelemetryMapFailuresReportedInExtraStats();
    void _testHwBufferFactoryDispatchSystemMemory();
#endif
#if defined(QGC_GST_BIN_HAS_GLUPLOAD) && defined(QGC_GST_BIN_HAS_DIRECT_DMABUF)
    void _testGpuBinPreferenceParse_data();
    void _testGpuBinPreferenceParse();
#endif
    void _testGpuBinSelection_data();
    void _testGpuBinSelection();
    void _testCpuLayoutSizesPlanesByComponent();
    void _testCpuCopyOddDimensions_data();
    void _testCpuCopyOddDimensions();
    void _testCpuCopyOfPaddedBuffer_data();
    void _testCpuCopyOfPaddedBuffer();
    void _testFrameGeometryFollowsBufferMetas_data();
    void _testFrameGeometryFollowsBufferMetas();
    void _testTelemetryMapDurationEwma();
    void _testTelemetryFallbackReasonMatrix();
    void _testTelemetrySyncWaitSplit();
};
