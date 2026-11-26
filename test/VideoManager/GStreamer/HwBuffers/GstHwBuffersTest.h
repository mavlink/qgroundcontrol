#pragma once

#include "GStreamerTestBase.h"

/// The HwBuffers GPU paths without a pipeline or a GPU: path selection, import caches, frame-texture bundles on the
/// Null QRhi, DMABuf modifier and fourcc policy, CPU fallbacks, crop matrices and telemetry.
class GstHwBuffersTest : public GStreamerTestBase
{
    Q_OBJECT

private slots:
    void _testDmaDrmModifierPolicy_data();
    void _testDmaDrmModifierPolicy();
    void _testDmaDrmPlaneFourccsMatchQtImporter();
    void _testDmaBufPathNeedsQtEglDisplay();
    void _testDmaBufPathResumesOnceDisplayIsKnown();
    void _testDmaBufMapTexturesRejectsForeignRhi();
    void _testVulkanOwnedFrameTexturesReleaseOnDestroy();
    void _testContextBridgeRegistry();
    void _testContextBridgeAnswersWholePipeline();
    void _testContextBridgeServesPipelineOnceDeviceIsKnown();
    void _testRemovedDeviceIsToldToThePipelinesThatGotIt();
    void _testHwBufferLifecycleResetsNativeCaches();
    void _testHwFrameTexturesRejectsForeignOldTextures();
    void _testHwFrameTexturesZeroHandleInvalidatesBundle_data();
    void _testHwFrameTexturesZeroHandleInvalidatesBundle();
    void _testHwFrameTexturesFinishMapReusesAcceptedBundle_data();
    void _testHwFrameTexturesFinishMapReusesAcceptedBundle();
    void _testHwFrameTexturesFinishMapRejectsIncompleteBundle();
    void _testHwFrameTexturesKeepResourcesUntilDestroyed();
    void _testHwPathRegistrySelection_data();
    void _testHwPathRegistrySelection();
    void _testHwBufferFactoryCacheRejectsMemoryTypeChange();
    void _testDmaBufTiledFrameRefusesCpuMap();
    void _testDmaBufTiledFrameDroppedWithoutImport_data();
    void _testDmaBufTiledFrameDroppedWithoutImport();
    void _testDmaBufImportFailureLatch_data();
    void _testDmaBufImportFailureLatch();
    void _testDmaBufProducerWaitNeedsSignalledFences_data();
    void _testDmaBufProducerWaitNeedsSignalledFences();
    void _testDmaBufFdCheckRejectsOtherFds();
    void _testDmaBufVulkanImportNeedsProbedDevice_data();
    void _testDmaBufVulkanImportNeedsProbedDevice();
    void _testDmaBufVulkanRequestsDeviceExtensions_data();
    void _testDmaBufVulkanRequestsDeviceExtensions();
    void _testDmaBufVulkanFdDupFailureIsNotARefusal();
    void _testDmaBufVulkanRetiredBufferNeedsNoLaterFrame_data();
    void _testDmaBufVulkanRetiredBufferNeedsNoLaterFrame();
    void _testCpuLayoutSizesPlanesByComponent();
    void _testCpuCopyOddDimensions_data();
    void _testCpuCopyOddDimensions();
    void _testCpuCopyOfPaddedBuffer_data();
    void _testCpuCopyOfPaddedBuffer();
    void _testQGCRhiCaptureCacheLifecycle();
    void _testGpuBinPreferenceParse_data();
    void _testGpuBinPreferenceParse();
    void _testGpuBinSelection_data();
    void _testGpuBinSelection();
    void _testTexturePadsPicture_data();
    void _testTexturePadsPicture();
    void _testFrameGeometryFollowsBufferMetas_data();
    void _testFrameGeometryFollowsBufferMetas();
    void _testHwVideoBufferCpuMapFallback();
    void _testHwVideoBufferCpuMapRefusesForeignLayout_data();
    void _testHwVideoBufferCpuMapRefusesForeignLayout();
    void _testWithPixelFormatKeepsOtherProperties();
    void _testTelemetryMapDurationEwma();
    void _testTelemetryFallbackReasonMatrix();
    void _testTelemetrySyncWaitSplit();
    void _testTelemetryMapFailuresReportedInExtraStats();
    void _testTelemetryDmaBufExtraStats();
};
