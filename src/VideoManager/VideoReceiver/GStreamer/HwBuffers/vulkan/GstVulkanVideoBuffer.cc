#include "GstVulkanVideoBuffer.h"

#include <QtGui/qtguiglobal.h>

#include "GstHwPathRegistry.h"
#include "GstHwPathTelemetry.h"
#include "GstHwVideoBuffer.h"
#include "GstVulkanContextBridge.h"

#if defined(QGC_HAS_GST_VULKAN_GPU_PATH) && QT_CONFIG(vulkan)

#include <QtCore/QLoggingCategory>
#include <rhi/qrhi.h>
#include <rhi/qrhi_platform.h>
#include <vulkan/vulkan.h>

// gst-vulkan's gstvkdecoder.h declares a struct member named `slots`, which collides with Qt's `slots` keyword macro
// (qtmetamacros.h). Suppress the macro across the gst/vulkan includes only.
#pragma push_macro("slots")
#undef slots
#include <gst/vulkan/gstvkdevice.h>
#include <gst/vulkan/gstvkimagememory.h>
#pragma pop_macro("slots")

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GstVulkanBufLog, "Video.GStreamer.HwBuffers.GstVulkanBuf")

namespace {

GstHw::MapDiagnostics s_diag;
std::atomic<bool> s_loggedDeviceMismatch{false};
std::atomic<bool> s_loggedMultiMemory{false};
std::atomic<bool> s_loggedSyncFallback{false};

}  // namespace

GstVulkanVideoBuffer::GstVulkanVideoBuffer(GstSample* sample, const GstVideoInfo& videoInfo,
                                           const QVideoFrameFormat& format)
    : GstHwVideoBuffer(QVideoFrame::RhiTextureHandle, sample, videoInfo, format)
{}

bool GstVulkanVideoBuffer::validatePlaneHandles() const
{
    return validatePlanes([](GstMemory* mem) { return mem && gst_is_vulkan_image_memory(mem); });
}

QVideoFrameTexturesUPtr GstVulkanVideoBuffer::mapTextures(QRhi& rhi, QVideoFrameTexturesUPtr& /*old*/)
{
    const GstHwPathTelemetry::ScopedMapTimer mapTimer(HwVideoBufferPath::Vulkan);
    GstBuffer* buffer = nullptr;
    if (!checkMapPreconditions(rhi, static_cast<int>(QRhi::Vulkan), GstVulkanBufLog(), s_diag, buffer)) {
        return GstHwPathTelemetry::fail(HwVideoBufferPath::Vulkan);
    }

    // Vulkan video decode yields one multiplanar VkImage (NV12/P010); disjoint multi-memory layouts are out of scope.
    if (gst_buffer_n_memory(buffer) != 1) {
        QGC_HW_WARN_ONCE(GstVulkanBufLog, s_loggedMultiMemory,
                         "Vulkan import: multi-memory buffer not supported — CPU fallback");
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

    // Hard safety net: createFrom() requires the texture belong to QRhi's VkDevice. A VkImage from a different VkDevice
    // is unusable (UB), so any mismatch routes to the CPU memcpy path. This is the expected outcome until gst-vulkan
    // offers a same-VkDevice wrap (see GstVulkanContextBridge).
    if (!vkMem->device || vkMem->device->device != nh->dev) {
        GstHwPathTelemetry::recordFallbackReason(HwVideoBufferPath::Vulkan,
                                                 GstHwPathTelemetry::HwFallbackReason::NoExt);
        QGC_HW_WARN_ONCE(GstVulkanBufLog, s_loggedDeviceMismatch,
                         "Vulkan import: GstVulkanImageMemory VkDevice != QRhi VkDevice — CPU fallback");
        return GstHwPathTelemetry::fail(HwVideoBufferPath::Vulkan);
    }

    // No decoder->QRhi sync wired: real zero-copy needs a shared timeline semaphore (GstVulkanOperation); the only
    // correct alternative — a per-frame vkDeviceWaitIdle — would stall the whole UI, and an unsynchronized VkImage
    // would tear. Unreachable on gst-vulkan 1.24.2 (the device-match guard above fails first); reachable once a
    // same-VkDevice wrap exists, which is where the import belongs.
    GstHwPathTelemetry::recordFallbackReason(HwVideoBufferPath::Vulkan,
                                             GstHwPathTelemetry::HwFallbackReason::VulkanNoSync);
    QGC_HW_WARN_ONCE(GstVulkanBufLog, s_loggedSyncFallback,
                     "Vulkan import: decoder sync unimplemented (no timeline semaphore) — CPU fallback");
    return GstHwPathTelemetry::fail(HwVideoBufferPath::Vulkan);
}

constinit const HwPathDescriptor GstHwPaths::kVulkan = {
    .path = HwVideoBufferPath::Vulkan,
    .matches = [](GstMemory* mem, const HwVideoBufferContext&) -> bool { return gst_is_vulkan_image_memory(mem); },
    .construct = &GstHwPaths::construct<GstVulkanVideoBuffer>,
    .bridge = &GstVulkanContextBridge::instance,
};

#endif  // QGC_HAS_GST_VULKAN_GPU_PATH
