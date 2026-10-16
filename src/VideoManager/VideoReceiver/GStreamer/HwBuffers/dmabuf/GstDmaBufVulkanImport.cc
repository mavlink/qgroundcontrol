#include "GstDmaBufVulkanImport.h"

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH) && defined(QGC_HAS_GST_VULKAN_GPU_PATH)

#include <algorithm>
#include <atomic>
#include <drm_fourcc.h>
#include <fcntl.h>
#include <gst/allocators/gstdmabuf.h>
#include <gst/video/video.h>
#include <limits>
#include <rhi/qrhi.h>
#include <rhi/qrhi_platform.h>
#include <unistd.h>
#include <vector>

#include <QtCore/QLoggingCategory>
#include <QtCore/QScopeGuard>
#include <QtGui/QVulkanInstance>

#include "GstDmaBufVideoBuffer.h"
#include "GstDmaFourcc.h"
#include "GstHwImportPreflight.h"
#include "GstHwPathTelemetry.h"
#include "GstHwVideoBuffer.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GstDmaBufVulkanLog, "Video.GStreamer.HwBuffers.GstDmaBufVulkan")

namespace {

GstHw::MapDiagnostics s_diag;
std::atomic<bool> s_loggedVulkanUnimpl{false};

/// Entry points the import needs. Device-level ones come from vkGetDeviceProcAddr for the QRhi's VkDevice: an
/// instance-level lookup of a device function may return a trampoline or null.
struct DeviceFns
{
    PFN_vkCreateImage createImage = nullptr;
    PFN_vkDestroyImage destroyImage = nullptr;
    PFN_vkAllocateMemory allocateMemory = nullptr;
    PFN_vkFreeMemory freeMemory = nullptr;
    PFN_vkBindImageMemory bindImageMemory = nullptr;
    PFN_vkGetImageMemoryRequirements getImageMemoryRequirements = nullptr;
    PFN_vkGetMemoryFdPropertiesKHR getMemoryFdProperties = nullptr;
    PFN_vkGetPhysicalDeviceMemoryProperties getPhysicalDeviceMemoryProperties = nullptr;
    PFN_vkCreateFence createFence = nullptr;
    PFN_vkDestroyFence destroyFence = nullptr;
    PFN_vkGetFenceStatus getFenceStatus = nullptr;
    PFN_vkWaitForFences waitForFences = nullptr;
    PFN_vkQueueSubmit queueSubmit = nullptr;
    PFN_vkQueueWaitIdle queueWaitIdle = nullptr;

    bool ok() const noexcept
    {
        return createImage && destroyImage && allocateMemory && freeMemory && bindImageMemory &&
               getImageMemoryRequirements && getMemoryFdProperties && getPhysicalDeviceMemoryProperties &&
               createFence && destroyFence && getFenceStatus && waitForFences && queueSubmit && queueWaitIdle;
    }
};

// Bumped by resetCachedState() so every render thread re-resolves after a device reset.
std::atomic<quint64> s_deviceFnsGeneration{0};

/// @p dev's entry points, cached per render thread (each window's QRhi owns its VkDevice) and re-resolved when that
/// thread's device changes or after resetCachedState().
const DeviceFns& deviceFns(QVulkanInstance& inst, VkDevice dev)
{
    struct CachedFns
    {
        VkDevice dev = VK_NULL_HANDLE;
        quint64 generation = 0;
        DeviceFns fns;
    };

    thread_local CachedFns cache;

    const quint64 generation = s_deviceFnsGeneration.load(std::memory_order_acquire);
    if (cache.dev == dev && cache.generation == generation) {
        return cache.fns;
    }
    const auto getDeviceProcAddr =
        reinterpret_cast<PFN_vkGetDeviceProcAddr>(inst.getInstanceProcAddr("vkGetDeviceProcAddr"));
    const auto devFn = [&](const char* name) -> PFN_vkVoidFunction {
        return getDeviceProcAddr ? getDeviceProcAddr(dev, name) : nullptr;
    };
    DeviceFns& fns = cache.fns;
    fns.createImage = reinterpret_cast<PFN_vkCreateImage>(devFn("vkCreateImage"));
    fns.destroyImage = reinterpret_cast<PFN_vkDestroyImage>(devFn("vkDestroyImage"));
    fns.allocateMemory = reinterpret_cast<PFN_vkAllocateMemory>(devFn("vkAllocateMemory"));
    fns.freeMemory = reinterpret_cast<PFN_vkFreeMemory>(devFn("vkFreeMemory"));
    fns.bindImageMemory = reinterpret_cast<PFN_vkBindImageMemory>(devFn("vkBindImageMemory"));
    fns.getImageMemoryRequirements =
        reinterpret_cast<PFN_vkGetImageMemoryRequirements>(devFn("vkGetImageMemoryRequirements"));
    fns.getMemoryFdProperties = reinterpret_cast<PFN_vkGetMemoryFdPropertiesKHR>(devFn("vkGetMemoryFdPropertiesKHR"));
    fns.getPhysicalDeviceMemoryProperties = reinterpret_cast<PFN_vkGetPhysicalDeviceMemoryProperties>(
        inst.getInstanceProcAddr("vkGetPhysicalDeviceMemoryProperties"));
    fns.createFence = reinterpret_cast<PFN_vkCreateFence>(devFn("vkCreateFence"));
    fns.destroyFence = reinterpret_cast<PFN_vkDestroyFence>(devFn("vkDestroyFence"));
    fns.getFenceStatus = reinterpret_cast<PFN_vkGetFenceStatus>(devFn("vkGetFenceStatus"));
    fns.waitForFences = reinterpret_cast<PFN_vkWaitForFences>(devFn("vkWaitForFences"));
    fns.queueSubmit = reinterpret_cast<PFN_vkQueueSubmit>(devFn("vkQueueSubmit"));
    fns.queueWaitIdle = reinterpret_cast<PFN_vkQueueWaitIdle>(devFn("vkQueueWaitIdle"));
    cache.dev = dev;
    cache.generation = generation;
    return fns;
}

/// An imported image and its memory, owned by one frame bundle.
struct OwnedImage
{
    VkDevice dev = VK_NULL_HANDLE;
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    DeviceFns fns;
};

// Render-thread confined, like the QRhi whose queue the fences are submitted to.
struct RetiredImage
{
    OwnedImage owned;
    VkFence fence = VK_NULL_HANDLE;
};

thread_local std::vector<RetiredImage> t_retiredImages;
const char s_retiredImagesCleanupKey = 0;

void destroyRetired(const RetiredImage& retired)
{
    const OwnedImage& o = retired.owned;
    o.fns.destroyImage(o.dev, o.image, nullptr);
    o.fns.freeMemory(o.dev, o.memory, nullptr);
    if (retired.fence != VK_NULL_HANDLE) {
        o.fns.destroyFence(o.dev, retired.fence, nullptr);
    }
}

// Destroys @p dev's retired images whose fence signalled; with @p waitAll (QRhi teardown) waits for every fence first.
void reapRetired(VkDevice dev, bool waitAll)
{
    for (auto it = t_retiredImages.begin(); it != t_retiredImages.end();) {
        const OwnedImage& o = it->owned;
        if (o.dev != dev) {
            ++it;
            continue;
        }
        if (waitAll) {
            (void) o.fns.waitForFences(dev, 1, &it->fence, VK_TRUE, UINT64_MAX);
        } else if (o.fns.getFenceStatus(dev, it->fence) != VK_SUCCESS) {
            ++it;
            continue;
        }
        destroyRetired(*it);
        it = t_retiredImages.erase(it);
    }
}

void retire(VkQueue queue, const OwnedImage& owned)
{
    RetiredImage retired{owned};
    const VkFenceCreateInfo fenceInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, 0};
    // A submit without batches signals its fence once all work already submitted to the queue has completed.
    if (owned.fns.createFence(owned.dev, &fenceInfo, nullptr, &retired.fence) != VK_SUCCESS ||
        owned.fns.queueSubmit(queue, 0, nullptr, retired.fence) != VK_SUCCESS) {
        (void) owned.fns.queueWaitIdle(queue);
        destroyRetired(retired);
        return;
    }
    t_retiredImages.push_back(retired);
    reapRetired(owned.dev, /*waitAll=*/false);
}

/// Release callback for a frame bundle owning @p owned (see GstVulkanOwnedFrameTextures::adoptVulkanResources). Qt
/// destroys a bundle when its pool slot is reused, which can be right after the frame that last sampled it was
/// submitted, so the callback queues a fence behind QRhi's submitted work and destroys the image once it signals; a
/// keyed QRhi cleanup callback destroys what is left while the device still exists. Render thread only.
std::function<void()> deferredRelease(QRhi& rhi, OwnedImage owned)
{
    const auto* nh = static_cast<const QRhiVulkanNativeHandles*>(rhi.nativeHandles());
    const VkQueue queue = nh ? nh->gfxQueue : VK_NULL_HANDLE;
    const VkDevice dev = owned.dev;
    // Keyed: one teardown hook per QRhi, run while its device is still alive.
    rhi.addCleanupCallback(&s_retiredImagesCleanupKey, [dev](QRhi*) { reapRetired(dev, /*waitAll=*/true); });
    return [queue, owned] { retire(queue, owned); };
}

// VkFormat for single-plane RGB layouts. Multi-planar YUV (NV12/P010/I420) maps to VK_FORMAT_UNDEFINED: QRhi would wrap
// a multi-planar VkImage as one single-plane texture with a COLOR-aspect view (invalid usage, no chroma), so those
// layouts take the CPU path until a per-plane import exists.
VkFormat vkFormatForVideoInfo(const GstVideoInfo& videoInfo)
{
    if (GST_VIDEO_INFO_N_PLANES(&videoInfo) != 1) {
        return VK_FORMAT_UNDEFINED;
    }
    switch (GstHw::drmFourccForPlane(videoInfo, 0)) {
        case DRM_FORMAT_ABGR8888:
        case DRM_FORMAT_XBGR8888:
            return VK_FORMAT_R8G8B8A8_UNORM;
        case DRM_FORMAT_ARGB8888:
        case DRM_FORMAT_XRGB8888:
            return VK_FORMAT_B8G8R8A8_UNORM;
        default:
            return VK_FORMAT_UNDEFINED;
    }
}

quint64 dmaMemoryBound(GstMemory* memory, const GstVideoInfo& videoInfo) noexcept
{
    gsize maxSize = 0;
    const gsize size = memory ? gst_memory_get_sizes(memory, nullptr, &maxSize) : 0;
    return std::max({static_cast<quint64>(GST_VIDEO_INFO_SIZE(&videoInfo)), static_cast<quint64>(size),
                     static_cast<quint64>(maxSize)});
}

/// @p planeLayout for the single plane vkFormatForVideoInfo() accepts; false (warned) when it can't lie inside
/// @p memoryBound.
bool validateVulkanPlaneLayout(const GstVideoInfo& videoInfo, GstVideoMeta* vmeta, quint64 memoryBound,
                               VkSubresourceLayout& planeLayout)
{
    const int width = GST_VIDEO_INFO_WIDTH(&videoInfo);
    const int height = GST_VIDEO_INFO_HEIGHT(&videoInfo);
    if (width <= 0 || height <= 0) {
        qCWarning(GstDmaBufVulkanLog) << "Vulkan import: invalid DMABuf dimensions" << width << "x" << height;
        return false;
    }

    const auto [offset, stride] = GstHw::planeLayout(videoInfo, vmeta, 0);
    const int pixelStride = GST_VIDEO_INFO_COMP_PSTRIDE(&videoInfo, 0);
    if (stride <= 0 || pixelStride <= 0) {
        qCWarning(GstDmaBufVulkanLog) << "Vulkan import: invalid DMABuf plane layout"
                                      << "stride" << stride << "pixelStride" << pixelStride;
        return false;
    }

    constexpr quint64 kMaxVkDeviceSize = static_cast<quint64>((std::numeric_limits<VkDeviceSize>::max)());
    const quint64 rowPitch = static_cast<quint64>(stride);
    const quint64 minRowBytes = static_cast<quint64>(width) * static_cast<quint64>(pixelStride);
    if (rowPitch < minRowBytes || offset > kMaxVkDeviceSize || rowPitch > kMaxVkDeviceSize) {
        qCWarning(GstDmaBufVulkanLog) << "Vulkan import: implausible DMABuf plane layout"
                                      << "offset" << static_cast<quint64>(offset) << "stride" << stride << "minRowBytes"
                                      << minRowBytes;
        return false;
    }

    if (rowPitch > std::numeric_limits<quint64>::max() / static_cast<quint64>(height)) {
        qCWarning(GstDmaBufVulkanLog) << "Vulkan import: DMABuf plane size overflow"
                                      << "stride" << stride << "height" << height;
        return false;
    }
    const quint64 planeSize = rowPitch * static_cast<quint64>(height);
    if (static_cast<quint64>(offset) > std::numeric_limits<quint64>::max() - planeSize) {
        qCWarning(GstDmaBufVulkanLog) << "Vulkan import: DMABuf plane end overflow"
                                      << "offset" << static_cast<quint64>(offset) << "planeSize" << planeSize;
        return false;
    }
    const quint64 planeEnd = static_cast<quint64>(offset) + planeSize;
    if (memoryBound > 0 && planeEnd > memoryBound) {
        qCWarning(GstDmaBufVulkanLog) << "Vulkan import: DMABuf plane exceeds buffer bounds"
                                      << "end" << planeEnd << "bound" << memoryBound;
        return false;
    }

    planeLayout.offset = static_cast<VkDeviceSize>(offset);
    planeLayout.rowPitch = static_cast<VkDeviceSize>(rowPitch);
    return true;
}

}  // namespace

namespace GstDmaBufVulkan {

bool canImport(const GstVideoInfo& videoInfo) noexcept
{
    return vkFormatForVideoInfo(videoInfo) != VK_FORMAT_UNDEFINED;
}

void resetCachedState() noexcept
{
    s_loggedVulkanUnimpl.store(false, std::memory_order_release);
    s_deviceFnsGeneration.fetch_add(1, std::memory_order_acq_rel);
}

}  // namespace GstDmaBufVulkan

QVideoFrameTexturesUPtr GstDmaBufVideoBuffer::importVulkan(QRhi& rhi, QVideoFrameTexturesUPtr& old)
{
    // dmabuf-fd -> VkImage via VK_EXT_external_memory_dma_buf + VkImageDrmFormatModifierExplicitCreateInfoEXT, wrapped
    // with QRhiTexture::createFrom. Every step is capability-gated; any miss returns fail(). Single-plane RGB layouts
    // only (see vkFormatForVideoInfo); validatePlaneHandles() already routed other layouts to the CPU path.
    const auto* nh = static_cast<const QRhiVulkanNativeHandles*>(rhi.nativeHandles());
    if (!nh || nh->dev == VK_NULL_HANDLE || nh->physDev == VK_NULL_HANDLE || nh->gfxQueue == VK_NULL_HANDLE ||
        !nh->inst) {
        QGC_HW_WARN_ONCE(GstDmaBufVulkanLog, s_loggedVulkanUnimpl, "Vulkan import: native device handles unavailable");
        return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
    }
    const VkDevice dev = nh->dev;
    const DeviceFns& fns = deviceFns(*nh->inst, dev);
    if (!fns.ok()) {
        QGC_HW_WARN_ONCE(GstDmaBufVulkanLog, s_loggedVulkanUnimpl,
                         "Vulkan import: required device functions (VK_EXT_external_memory_dma_buf) unavailable");
        return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
    }
    GstBuffer* buffer = gst_sample_get_buffer(_sample);
    if (!buffer || gst_buffer_n_memory(buffer) != 1) {
        // Multi-fd disjoint planes: out of scope for this conservative single-VkImage import.
        QGC_HW_WARN_ONCE(GstDmaBufVulkanLog, s_loggedVulkanUnimpl,
                         "Vulkan import: multi-fd DMABuf not supported — CPU upload");
        return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
    }
    const VkFormat vkFormat = vkFormatForVideoInfo(_videoInfo);
    if (vkFormat == VK_FORMAT_UNDEFINED) {
        QGC_HW_WARN_ONCE(GstDmaBufVulkanLog, s_loggedVulkanUnimpl,
                         "Vulkan import: no VkFormat for DMABuf layout — CPU upload");
        return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
    }

    // Pre-flight RHI format/size support before building the VkImage so an unsupported import demotes to CPU on a
    // query.
    if (!GstHwImportPreflight::preflightOrRecord(&rhi, HwVideoBufferPath::DmaBuf, _format.pixelFormat(),
                                                 _format.frameSize())) {
        return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
    }

    GstMemory* gstMemory = gst_buffer_peek_memory(buffer, 0);
    const int fd = gst_dmabuf_memory_get_fd(gstMemory);
    if (fd < 0) {
        return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
    }
    VkSubresourceLayout planeLayout = {};
    if (!validateVulkanPlaneLayout(_videoInfo, gst_buffer_get_video_meta(buffer), dmaMemoryBound(gstMemory, _videoInfo),
                                   planeLayout)) {
        return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
    }

    VkImageDrmFormatModifierExplicitCreateInfoEXT modInfo = {};
    modInfo.sType = VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_EXPLICIT_CREATE_INFO_EXT;
    modInfo.drmFormatModifier = _drmModifier;
    modInfo.drmFormatModifierPlaneCount = 1;
    modInfo.pPlaneLayouts = &planeLayout;

    VkExternalMemoryImageCreateInfo extImg = {};
    extImg.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
    extImg.pNext = &modInfo;
    extImg.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;

    VkImageCreateInfo imgInfo = {};
    imgInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imgInfo.pNext = &extImg;
    imgInfo.imageType = VK_IMAGE_TYPE_2D;
    imgInfo.format = vkFormat;
    imgInfo.extent = {static_cast<uint32_t>(GST_VIDEO_INFO_WIDTH(&_videoInfo)),
                      static_cast<uint32_t>(GST_VIDEO_INFO_HEIGHT(&_videoInfo)), 1};
    imgInfo.mipLevels = 1;
    imgInfo.arrayLayers = 1;
    imgInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imgInfo.tiling = VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT;
    imgInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    imgInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkImage image = VK_NULL_HANDLE;
    if (fns.createImage(dev, &imgInfo, nullptr, &image) != VK_SUCCESS || image == VK_NULL_HANDLE) {
        QGC_HW_WARN_ONCE(GstDmaBufVulkanLog, s_loggedVulkanUnimpl, "Vulkan import: vkCreateImage failed — CPU upload");
        return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
    }
    const auto destroyImage = qScopeGuard([&] {
        if (image != VK_NULL_HANDLE)
            fns.destroyImage(dev, image, nullptr);
    });

    // dup the fd: Vulkan takes ownership of the imported fd on successful allocate.
    const int dupFd = ::fcntl(fd, F_DUPFD_CLOEXEC, 0);
    if (dupFd < 0) {
        return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
    }
    bool fdConsumed = false;
    const auto closeFd = qScopeGuard([&] {
        if (!fdConsumed && dupFd >= 0)
            ::close(dupFd);
    });

    VkMemoryFdPropertiesKHR fdProps = {};
    fdProps.sType = VK_STRUCTURE_TYPE_MEMORY_FD_PROPERTIES_KHR;
    if (fns.getMemoryFdProperties(dev, VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT, dupFd, &fdProps) != VK_SUCCESS) {
        return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
    }

    VkMemoryRequirements memReq = {};
    fns.getImageMemoryRequirements(dev, image, &memReq);
    VkPhysicalDeviceMemoryProperties physMem = {};
    fns.getPhysicalDeviceMemoryProperties(nh->physDev, &physMem);
    uint32_t memTypeIndex = UINT32_MAX;
    const uint32_t allowed = memReq.memoryTypeBits & fdProps.memoryTypeBits;
    for (uint32_t i = 0; i < physMem.memoryTypeCount; ++i) {
        if (allowed & (1u << i)) {
            memTypeIndex = i;
            break;
        }
    }
    if (memTypeIndex == UINT32_MAX) {
        return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
    }

    VkImportMemoryFdInfoKHR importInfo = {};
    importInfo.sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR;
    importInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;
    importInfo.fd = dupFd;

    VkMemoryDedicatedAllocateInfo dedicated = {};
    dedicated.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO;
    dedicated.image = image;
    dedicated.pNext = &importInfo;

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.pNext = &dedicated;
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = memTypeIndex;

    // VK_KHR_external_memory_fd transfers fd ownership to the implementation only on a SUCCESSFUL import; on failure
    // the application still owns the fd and must close it. Disarm the close guard only after VK_SUCCESS.
    VkDeviceMemory memory = VK_NULL_HANDLE;
    if (fns.allocateMemory(dev, &allocInfo, nullptr, &memory) != VK_SUCCESS || memory == VK_NULL_HANDLE) {
        QGC_HW_WARN_ONCE(GstDmaBufVulkanLog, s_loggedVulkanUnimpl,
                         "Vulkan import: vkAllocateMemory failed — CPU upload");
        return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
    }
    fdConsumed = true;
    const auto freeMemory = qScopeGuard([&] {
        if (memory != VK_NULL_HANDLE)
            fns.freeMemory(dev, memory, nullptr);
    });

    if (const VkResult bindRes = fns.bindImageMemory(dev, image, memory, 0); bindRes != VK_SUCCESS) {
        QGC_HW_WARN_ONCE(GstDmaBufVulkanLog, s_loggedVulkanUnimpl,
                         "Vulkan import: vkBindImageMemory failed (VkResult=" << bindRes << ") — CPU upload");
        return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
    }

    const GstHwMapTail tail{.path = HwVideoBufferPath::DmaBuf,
                            .frameSize = _format.frameSize(),
                            .pixelFormat = _format.pixelFormat(),
                            .log = &GstDmaBufVulkanLog(),
                            .diag = &s_diag,
                            .preflight = false,
                            .successTag = "DMABuf->Vulkan"};
    return GstHwFrameTexturesBase::finishMap<GstVulkanOwnedFrameTextures>(*this, rhi, old, tail, nullptr, [&] {
        auto bundle = std::make_unique<GstVulkanOwnedFrameTextures>(&rhi, tail.frameSize, tail.pixelFormat, image);
        if (bundle->allPlanesValid()) {
            // Ownership of image/memory passes to the bundle; disarm the scope guards.
            bundle->adoptVulkanResources(deferredRelease(rhi, {dev, image, memory, fns}));
            image = VK_NULL_HANDLE;
            memory = VK_NULL_HANDLE;
        }
        return bundle;
    });
}

#endif  // QGC_HAS_GST_DMABUF_GPU_PATH && QGC_HAS_GST_VULKAN_GPU_PATH
