#pragma once

#include "GStreamerTestBase.h"

/// What the HwBuffers GPU paths share, without a pipeline or a GPU: context bridges, path selection, the factory, CPU
/// fallbacks, texture padding and telemetry. The cases that need a GPU path are built with one.
class GstHwBuffersTest : public GStreamerTestBase
{
    Q_OBJECT

private slots:
#if defined(QGC_HAS_ANY_GPU_PATH)
    void _testContextBridgeRegistry();
    void _testPipelineHandsBridgeContextsToItsElements();
    void _testRemovedDeviceIsToldToThePipelinesThatGotIt();
    void _testHwBufferLifecycleResetsNativeCaches();
    void _testGraphicsDeviceSettledUnderSoftwareSceneGraph();
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
    void _testTelemetryMapDurationEwma();
    void _testTelemetryCountsPerPathAndKind();
};
