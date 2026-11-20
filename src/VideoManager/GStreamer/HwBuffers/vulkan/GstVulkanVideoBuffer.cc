#include "GstVulkanVideoBuffer.h"

#include <QtGui/qtguiglobal.h>

#include "GstHwPathRegistry.h"
#include "GstHwPathTelemetry.h"
#include "GstHwVideoBuffer.h"
#include "GstVulkanContextBridge.h"

#if defined(QGC_HAS_GST_VULKAN_GPU_PATH) && QT_CONFIG(vulkan)

#include <rhi/qrhi.h>
#include <rhi/qrhi_platform.h>
#include <vulkan/vulkan.h>

#include <QtCore/QLoggingCategory>

// gst-vulkan's gstvkdecoder.h declares a struct member named `slots`, which collides with Qt's `slots` keyword macro
// (qtmetamacros.h). Suppress the macro across the gst/vulkan includes only.
#pragma push_macro("slots")
#undef slots
#include <gst/vulkan/gstvkdevice.h>
#include <gst/vulkan/gstvkimagememory.h>
#pragma pop_macro("slots")

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GstVulkanBufLog, "Video.GStreamer.HwBuffers.GstVulkanBuf")

bool GstVulkanVideoBuffer::validatePlaneHandles() const
{
    return validatePlanes([](GstMemory* mem) { return mem && gst_is_vulkan_image_memory(mem); });
}

QVideoFrameTexturesUPtr GstVulkanVideoBuffer::mapTextures(QRhi& rhi, QVideoFrameTexturesUPtr& /*old*/)
{
    const GstHwPathTelemetry::ScopedMapTimer mapTimer(HwVideoBufferPath::Vulkan);
    GstBuffer* buffer = nullptr;
    if (!checkMapPreconditions(rhi, static_cast<int>(QRhi::Vulkan), GstVulkanBufLog(), buffer)) {
        return GstHwPathTelemetry::fail(HwVideoBufferPath::Vulkan);
    }

    // Vulkan video decode yields one multiplanar VkImage (NV12/P010); disjoint multi-memory layouts are out of scope.
    if (gst_buffer_n_memory(buffer) != 1) {
        QGC_HW_WARN_ONCE(GstVulkanBufLog, "Vulkan import: multi-memory buffer not supported; frame not imported");
        return GstHwPathTelemetry::fail(HwVideoBufferPath::Vulkan);
    }

    GstMemory* mem0 = gst_buffer_peek_memory(buffer, 0);
    if (!mem0 || !gst_is_vulkan_image_memory(mem0)) {
        return GstHwPathTelemetry::fail(HwVideoBufferPath::Vulkan);
    }
    auto* vkMem = reinterpret_cast<GstVulkanImageMemory*>(mem0);

    const auto* nh = static_cast<const QRhiVulkanNativeHandles*>(rhi.nativeHandles());
    if (!nh || nh->dev == VK_NULL_HANDLE) {
        return GstHwPathTelemetry::fail(HwVideoBufferPath::Vulkan);
    }

    // createFrom() requires the image to belong to QRhi's VkDevice; one of another VkDevice is unusable (UB). Every
    // GstVulkanImageMemory is: gst-vulkan creates its own VkDevice and can't wrap Qt's (see GstVulkanContextBridge).
    if (!vkMem->device || vkMem->device->device != nh->dev) {
        GstHwPathTelemetry::recordFallbackReason(HwVideoBufferPath::Vulkan,
                                                 GstHwPathTelemetry::HwFallbackReason::NoExt);
        QGC_HW_WARN_ONCE(GstVulkanBufLog,
                         "Vulkan import: GstVulkanImageMemory VkDevice != QRhi VkDevice; frame not imported");
        return GstHwPathTelemetry::fail(HwVideoBufferPath::Vulkan);
    }

    // Unreachable until gst-vulkan can wrap Qt's VkDevice. Sampling the image would then also need the decoder's
    // writes to be complete: its timeline semaphore (GstVulkanOperation) can't be handed to QRhi's submission, a
    // per-frame vkDeviceWaitIdle would stall the whole UI, and an unsynchronized VkImage would tear.
    GstHwPathTelemetry::recordFallbackReason(HwVideoBufferPath::Vulkan,
                                             GstHwPathTelemetry::HwFallbackReason::VulkanNoSync);
    QGC_HW_WARN_ONCE(GstVulkanBufLog,
                     "Vulkan import: no decoder-to-QRhi sync (timeline semaphore); frame not imported");
    return GstHwPathTelemetry::fail(HwVideoBufferPath::Vulkan);
}

constinit const HwPathDescriptor GstHwPaths::kVulkan = {
    .path = HwVideoBufferPath::Vulkan,
    .matches = [](GstMemory* mem) -> bool { return gst_is_vulkan_image_memory(mem); },
    .construct = &GstHwPaths::construct<GstVulkanVideoBuffer>,
    .bridge = &GstVulkanContextBridge::instance,
};

#endif  // QGC_HAS_GST_VULKAN_GPU_PATH
