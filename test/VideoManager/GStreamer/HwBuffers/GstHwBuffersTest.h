#pragma once

#include "GStreamerTestBase.h"

/// The HwBuffers GPU paths without a pipeline or a GPU: path selection, import caches, frame-texture bundles on the
/// Null QRhi, DMABuf modifier and fourcc policy, CPU fallbacks, crop matrices and telemetry.
class GstHwBuffersTest : public GStreamerTestBase
{
    Q_OBJECT

private slots:
#ifdef QGC_GST_STREAMING
    void _testDmaDrmModifierPolicy_data();
    void _testDmaDrmModifierPolicy();
    void _testDmaDrmPlaneFourccsMatchQtImporter();
    void _testDmaBufPathNeedsQtEglDisplay();
    void _testVulkanOwnedFrameTexturesReleaseOnDestroy();
    void _testContextBridgeRegistry();
    void _testHwBufferLifecycleResetsNativeCaches();
    void _testHwFrameTexturesRejectsForeignOldTextures();
    void _testHwFrameTexturesZeroHandleInvalidatesBundle_data();
    void _testHwFrameTexturesZeroHandleInvalidatesBundle();
    void _testHwFrameTexturesFinishMapReusesAcceptedBundle_data();
    void _testHwFrameTexturesFinishMapReusesAcceptedBundle();
    void _testHwFrameTexturesFinishMapRejectsIncompleteBundle();
    void _testHwFrameTexturesKeepResourcesUntilDestroyed();
    void _testHwImportCacheEvictsLeastRecentlyUsed();
    void _testHwImportCacheReplaceDestroysPrevious();
    void _testHwImportCacheEraseIfDestroysMatches();
    void _testHwImportCachePinnedEntriesOutliveRemoval();
    void _testHwImportCacheRejectsInsertWhenAllPinned();
    void _testHwImportCacheDefersDestroyToOwnerThread();
    void _testHwPathRegistrySelection_data();
    void _testHwPathRegistrySelection();
    void _testHwBufferFactoryCacheRejectsMemoryTypeChange();
    void _testDmaBufTiledFrameRefusesCpuMap();
    void _testDmaBufTiledFrameDroppedWithoutImport_data();
    void _testDmaBufTiledFrameDroppedWithoutImport();
    void _testDmaBufImportFailureLatch_data();
    void _testDmaBufImportFailureLatch();
    void _testDmaBufVulkanImportRejectsMultiPlanar_data();
    void _testDmaBufVulkanImportRejectsMultiPlanar();
    void _testCpuLayoutSizesPlanesByComponent();
    void _testCpuCopyOddDimensions_data();
    void _testCpuCopyOddDimensions();
    void _testQGCRhiCaptureCacheLifecycle();
    void _testPreflightVerdictsDroppedWithRhi();
    void _testGpuBinPreferenceParse_data();
    void _testGpuBinPreferenceParse();
    void _testGpuBinSelection_data();
    void _testGpuBinSelection();
    void _testHwBufferCropMatrixIdentityWithoutMeta();
    void _testTexturePadsPicture_data();
    void _testTexturePadsPicture();
    void _testHwBufferCropMatrixFromVideoCropMeta();
    void _testHwVideoBufferCpuMapFallback();
    void _testHwVideoBufferCpuMapRefusesForeignLayout_data();
    void _testHwVideoBufferCpuMapRefusesForeignLayout();
    void _testTelemetryMapDurationEwma();
    void _testTelemetryFallbackReasonMatrix();
    void _testTelemetrySyncWaitSplit();
    void _testTelemetryMapFailuresReportedInExtraStats();
    void _testTelemetryDmaBufExtraStatsDrain();
#endif
};
