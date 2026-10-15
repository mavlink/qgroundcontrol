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
#include <vulkan/vulkan.h>

#include <QtCore/QLoggingCategory>
#include <QtCore/QScopeGuard>
#include <QtGui/QVulkanInstance>

#include "GstDmaBufVideoBuffer.h"
#include "GstDmaFourcc.h"
#include "GstHwFrameTexturesBase.h"
#include "GstHwImportPreflight.h"
#include "GstHwPathTelemetry.h"
#include "GstHwVideoBuffer.h"
#include "GstVulkanFrameTextures.h"
#include "GstVulkanImport.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GstDmaBufVulkanLog, "Video.GStreamer.HwBuffers.GstDmaBufVulkan")

namespace {

GstHw::MapDiagnostics s_diag;
std::atomic<bool> s_loggedVulkanUnimpl{false};

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

bool validateVulkanPlaneLayouts(const GstVideoInfo& videoInfo, GstVideoMeta* vmeta, int planeCount, quint64 memoryBound,
                                VkSubresourceLayout* planeLayouts)
{
    const int width = GST_VIDEO_INFO_WIDTH(&videoInfo);
    const int height = GST_VIDEO_INFO_HEIGHT(&videoInfo);
    if (width <= 0 || height <= 0) {
        qCWarning(GstDmaBufVulkanLog) << "Vulkan import: invalid DMABuf dimensions" << width << "x" << height;
        return false;
    }

    const int componentCount = GST_VIDEO_INFO_N_COMPONENTS(&videoInfo);
    if (componentCount <= 0 || planeCount > componentCount) {
        qCWarning(GstDmaBufVulkanLog) << "Vulkan import: invalid DMABuf plane count" << planeCount << "components"
                                      << componentCount;
        return false;
    }
    if (vmeta && static_cast<int>(vmeta->n_planes) < planeCount) {
        qCWarning(GstDmaBufVulkanLog) << "Vulkan import: GstVideoMeta has too few planes" << vmeta->n_planes
                                      << "expected" << planeCount;
        return false;
    }

    constexpr quint64 kMaxVkDeviceSize = static_cast<quint64>((std::numeric_limits<VkDeviceSize>::max)());
    for (int p = 0; p < planeCount; ++p) {
        const auto [offset, stride] = GstHw::planeLayout(videoInfo, vmeta, p);
        const int planeWidth = GST_VIDEO_INFO_COMP_WIDTH(&videoInfo, p);
        const int planeHeight = GST_VIDEO_INFO_COMP_HEIGHT(&videoInfo, p);
        const int pixelStride = GST_VIDEO_INFO_COMP_PSTRIDE(&videoInfo, p);

        if (stride <= 0 || planeWidth <= 0 || planeHeight <= 0 || pixelStride <= 0) {
            qCWarning(GstDmaBufVulkanLog) << "Vulkan import: invalid DMABuf plane layout"
                                          << "plane" << p << "stride" << stride << "width" << planeWidth << "height"
                                          << planeHeight << "pixelStride" << pixelStride;
            return false;
        }

        const quint64 rowPitch = static_cast<quint64>(stride);
        const quint64 minRowBytes = static_cast<quint64>(planeWidth) * static_cast<quint64>(pixelStride);
        if (rowPitch < minRowBytes || offset > kMaxVkDeviceSize || rowPitch > kMaxVkDeviceSize) {
            qCWarning(GstDmaBufVulkanLog) << "Vulkan import: implausible DMABuf plane layout"
                                          << "plane" << p << "offset" << static_cast<quint64>(offset) << "stride"
                                          << stride << "minRowBytes" << minRowBytes;
            return false;
        }

        if (rowPitch > std::numeric_limits<quint64>::max() / static_cast<quint64>(planeHeight)) {
            qCWarning(GstDmaBufVulkanLog) << "Vulkan import: DMABuf plane size overflow"
                                          << "plane" << p << "stride" << stride << "height" << planeHeight;
            return false;
        }
        const quint64 planeSize = rowPitch * static_cast<quint64>(planeHeight);
        if (static_cast<quint64>(offset) > std::numeric_limits<quint64>::max() - planeSize) {
            qCWarning(GstDmaBufVulkanLog)
                << "Vulkan import: DMABuf plane end overflow"
                << "plane" << p << "offset" << static_cast<quint64>(offset) << "planeSize" << planeSize;
            return false;
        }
        const quint64 planeEnd = static_cast<quint64>(offset) + planeSize;
        if (memoryBound > 0 && planeEnd > memoryBound) {
            qCWarning(GstDmaBufVulkanLog) << "Vulkan import: DMABuf plane exceeds buffer bounds"
                                          << "plane" << p << "end" << planeEnd << "bound" << memoryBound;
            return false;
        }

        planeLayouts[p].offset = static_cast<VkDeviceSize>(offset);
        planeLayouts[p].rowPitch = static_cast<VkDeviceSize>(rowPitch);
    }
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
    GstVulkanImport::resetDeviceFns();
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
    const GstVulkanImport::DeviceFns& fns = GstVulkanImport::deviceFns(*nh->inst, dev);
    if (!fns.ok()) {
        QGC_HW_WARN_ONCE(GstDmaBufVulkanLog, s_loggedVulkanUnimpl,
                         "Vulkan import: required device functions (VK_EXT_external_memory_dma_buf) unavailable");
        return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
    }
    const auto pfnCreateImage = fns.createImage;
    const auto pfnDestroyImage = fns.destroyImage;
    const auto pfnAllocateMemory = fns.allocateMemory;
    const auto pfnFreeMemory = fns.freeMemory;
    const auto pfnBindImageMemory = fns.bindImageMemory;
    const auto pfnGetMemReq = fns.getImageMemoryRequirements;
    const auto pfnGetFdProps = fns.getMemoryFdProperties;
    const auto pfnGetPhysMemProps = fns.getPhysicalDeviceMemoryProperties;

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
    // memory-plane count == video-plane count only for the uncompressed modifiers this path accepts; a compressed
    // modifier (e.g. CCS) adds metadata planes and would need the modifier's own plane count here.
    const int planeCount = std::clamp(int(GST_VIDEO_INFO_N_PLANES(&_videoInfo)), 1, GstHw::kMaxPlanes);
    GstVideoMeta* vmeta = gst_buffer_get_video_meta(buffer);
    VkSubresourceLayout planeLayouts[GstHw::kMaxPlanes] = {};
    if (!validateVulkanPlaneLayouts(_videoInfo, vmeta, planeCount, dmaMemoryBound(gstMemory, _videoInfo), planeLayouts)) {
        return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
    }

    VkImageDrmFormatModifierExplicitCreateInfoEXT modInfo = {};
    modInfo.sType = VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_EXPLICIT_CREATE_INFO_EXT;
    modInfo.drmFormatModifier = _drmModifier;
    modInfo.drmFormatModifierPlaneCount = static_cast<uint32_t>(planeCount);
    modInfo.pPlaneLayouts = planeLayouts;

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
    if (pfnCreateImage(dev, &imgInfo, nullptr, &image) != VK_SUCCESS || image == VK_NULL_HANDLE) {
        QGC_HW_WARN_ONCE(GstDmaBufVulkanLog, s_loggedVulkanUnimpl, "Vulkan import: vkCreateImage failed — CPU upload");
        return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
    }
    const auto destroyImage = qScopeGuard([&] {
        if (image != VK_NULL_HANDLE)
            pfnDestroyImage(dev, image, nullptr);
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
    if (pfnGetFdProps(dev, VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT, dupFd, &fdProps) != VK_SUCCESS) {
        return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
    }

    VkMemoryRequirements memReq = {};
    pfnGetMemReq(dev, image, &memReq);
    VkPhysicalDeviceMemoryProperties physMem = {};
    pfnGetPhysMemProps(nh->physDev, &physMem);
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
    if (pfnAllocateMemory(dev, &allocInfo, nullptr, &memory) != VK_SUCCESS || memory == VK_NULL_HANDLE) {
        QGC_HW_WARN_ONCE(GstDmaBufVulkanLog, s_loggedVulkanUnimpl,
                         "Vulkan import: vkAllocateMemory failed — CPU upload");
        return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
    }
    fdConsumed = true;
    const auto freeMemory = qScopeGuard([&] {
        if (memory != VK_NULL_HANDLE)
            pfnFreeMemory(dev, memory, nullptr);
    });

    if (const VkResult bindRes = pfnBindImageMemory(dev, image, memory, 0); bindRes != VK_SUCCESS) {
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
            bundle->adoptVulkanResources(GstVulkanImport::deferredRelease(rhi, {dev, image, memory, fns}));
            image = VK_NULL_HANDLE;
            memory = VK_NULL_HANDLE;
        }
        return bundle;
    });
}

#endif  // QGC_HAS_GST_DMABUF_GPU_PATH && QGC_HAS_GST_VULKAN_GPU_PATH
