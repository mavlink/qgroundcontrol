#pragma once

#include <QtCore/qglobal.h>

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH) && defined(QGC_HAS_GST_VULKAN_GPU_PATH)

#include <functional>
#include <gst/video/video-info.h>
#include <vulkan/vulkan.h>

#include <QtCore/QSize>
#include <QtMultimedia/QVideoFrameFormat>

#include "GstHwFrameTexturesBase.h"

namespace GstDmaBufVulkan {

/// True when @p videoInfo's layout can be imported as a single QRhi Vulkan texture; safe from any thread.
bool canImport(const GstVideoInfo& videoInfo) noexcept;

/// Re-arm the once-logged warnings and force every render thread to re-resolve its Vulkan device functions.
void resetCachedState() noexcept;

}  // namespace GstDmaBufVulkan

/// Owns the VkImage + backing VkDeviceMemory a DMABuf import created; released with the bundle.
class GstVulkanOwnedFrameTextures final : public GstHwFrameTexturesBase
{
public:
    GstVulkanOwnedFrameTextures(QRhi* rhi, QSize size, QVideoFrameFormat::PixelFormat pixelFormat, VkImage image)
    {
        // QRhi's first barrier transitions from this layout; UNDEFINED would let the driver discard the imported
        // contents, GENERAL preserves them.
        wrapPlanes(rhi, size, pixelFormat, std::span<const VkImage>(&image, 1), VK_IMAGE_LAYOUT_GENERAL);
    }

    ~GstVulkanOwnedFrameTextures() override
    {
        // QRhiTexture must be gone before the VkImage it wraps; reset it first.
        _textures[0].reset();
        if (_release) {
            _release();
        }
    }

    /// Transfers VkImage/VkDeviceMemory ownership into the bundle: @p release runs once, when Qt destroys the bundle.
    void adoptVulkanResources(std::function<void()> release) noexcept { _release = std::move(release); }

private:
    std::function<void()> _release;
};

#endif  // QGC_HAS_GST_DMABUF_GPU_PATH && QGC_HAS_GST_VULKAN_GPU_PATH
