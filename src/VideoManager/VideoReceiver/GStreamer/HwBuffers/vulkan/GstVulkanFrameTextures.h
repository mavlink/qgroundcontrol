#pragma once

#include <QtGui/qtguiglobal.h>

#if defined(QGC_HAS_GST_VULKAN_GPU_PATH) && QT_CONFIG(vulkan)

#include <functional>
#include <span>
#include <vulkan/vulkan.h>

#include <QtCore/QSize>
#include <QtMultimedia/QVideoFrameFormat>

#include "GstHwFrameTexturesBase.h"

/// Wraps a single VkImage as one QRhiTexture. Owning and borrowing variants share this wrap; only the image lifetime
/// differs. The plane is null when the wrap fails, surfaced via allPlanesValid().
class GstVulkanFrameTexturesBase : public GstHwFrameTexturesBase
{
protected:
    void wrapImage(QRhi* rhi, QSize size, QVideoFrameFormat::PixelFormat pixelFormat, VkImage image, int layout)
    {
        const quint64 handle = reinterpret_cast<quint64>(image);
        wrapPlanes(rhi, size, pixelFormat, std::span<const quint64>(&handle, 1), layout);
    }
};

/// Owns the imported VkImage + backing VkDeviceMemory (DMABuf zero-copy import); released with the bundle.
class GstVulkanOwnedFrameTextures final : public GstVulkanFrameTexturesBase
{
public:
    GstVulkanOwnedFrameTextures(QRhi* rhi, QSize size, QVideoFrameFormat::PixelFormat pixelFormat, VkImage image)
    {
        // QRhi's first barrier transitions from this layout; UNDEFINED would let the driver discard the imported
        // contents, GENERAL preserves them.
        wrapImage(rhi, size, pixelFormat, image, VK_IMAGE_LAYOUT_GENERAL);
    }

    ~GstVulkanOwnedFrameTextures() override
    {
        // QRhiTexture must be gone before the VkImage it wraps; reset it first.
        _textures[0].reset();
        if (_release) {
            _release();
        }
    }

    HwVideoBufferPath sourcePath() const override { return HwVideoBufferPath::DmaBuf; }

    /// Transfers VkImage/VkDeviceMemory ownership into the bundle: @p release runs once, when Qt destroys the bundle.
    void adoptVulkanResources(std::function<void()> release) noexcept { _release = std::move(release); }

private:
    std::function<void()> _release;
};

/// Borrows a VkImage owned by GstVulkanImageMemory and kept alive via the held GstSample (setSourceSample). Never
/// destroys the image: createFrom() does not take ownership.
class GstVulkanBorrowedFrameTextures final : public GstVulkanFrameTexturesBase
{
public:
    GstVulkanBorrowedFrameTextures(QRhi* rhi, QSize size, QVideoFrameFormat::PixelFormat pixelFormat, VkImage image,
                                   int layout)
    {
        wrapImage(rhi, size, pixelFormat, image, layout);
    }

    HwVideoBufferPath sourcePath() const override { return HwVideoBufferPath::Vulkan; }
};

#endif  // QGC_HAS_GST_VULKAN_GPU_PATH
