#pragma once

#include "GStreamerTestBase.h"

/// The DMABuf path without a pipeline or a GPU: modifier and fourcc policy, the gates a frame passes before its
/// import, import failures, and the import under Vulkan on a stand-in device. Built with the path.
class GstHwBuffersDmaBufTest : public GStreamerTestBase
{
    Q_OBJECT

private slots:
    void _testDmaDrmModifierPolicy_data();
    void _testDmaDrmModifierPolicy();
    void _testDmaBufImportsEveryPlaneOfOfferedFormats();
    void _testDmaBufPathNeedsQtEglDisplay();
    void _testDmaBufPathResumesOnceDisplayIsKnown();
    void _testDmaBufMapTexturesRejectsForeignRhi();
    void _testHwBufferFactoryCacheRejectsMemoryTypeChange();
    void _testDmaBufTiledFrameRefusesCpuMap();
    void _testDmaBufTiledFrameDroppedWithoutImport_data();
    void _testDmaBufTiledFrameDroppedWithoutImport();
    void _testDmaBufImportFailureLatch_data();
    void _testDmaBufImportFailureLatch();
    void _testDmaBufProducerWaitNeedsSignalledFences_data();
    void _testDmaBufProducerWaitNeedsSignalledFences();
    void _testTelemetryDmaBufExtraStats();
#if defined(QGC_HAS_GST_DMABUF_VULKAN_IMPORT)
    void _testDmaBufVulkanImportNeedsProbedDevice_data();
    void _testDmaBufVulkanImportNeedsProbedDevice();
    void _testDmaBufVulkanRequestsDeviceExtensions_data();
    void _testDmaBufVulkanRequestsDeviceExtensions();
    void _testDmaBufVulkanFdDupFailureIsNotARefusal();
    void _testDmaBufVulkanRetiredBufferNeedsNoLaterFrame_data();
    void _testDmaBufVulkanRetiredBufferNeedsNoLaterFrame();
    void _testVulkanOwnedFrameTexturesReleaseOnDestroy();
#endif
};

/// The DMABuf path fed by pipelines: frames a VA decoder exports, and the bin that routes each stream by its memory.
/// Each test skips where the host lacks the device or the elements.
class GstHwBuffersDmaBufPipelineTest : public GStreamerTestBase
{
    Q_OBJECT

private slots:
    void _testDmaBufDispatch();
#if defined(QGC_GST_BIN_HAS_GLUPLOAD)
    void _testDmaBufOrGlUploadBinRoutesByMemory();
#endif
};
