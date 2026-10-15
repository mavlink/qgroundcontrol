#pragma once

#include <QtGui/qtguiglobal.h>

#if defined(QGC_HAS_GST_VULKAN_GPU_PATH) && QT_CONFIG(vulkan)

#include <QtCore/QSize>
#include <QtMultimedia/QVideoFrameFormat>
#include <private/qvideotexturehelper_p.h>
#include <rhi/qrhi.h>
#include <vulkan/vulkan.h>

#include "GstHwFrameTexturesBase.h"

/// Wraps a single VkImage as one QRhiTexture. Owning and borrowing variants share this wrap; only the image lifetime
/// differs. `_textures[0]` is null when the wrap fails, surfaced via valid().
class GstVulkanFrameTexturesBase : public GstHwFrameTexturesBase
{
public:
    bool valid() const noexcept { return _textures[0] != nullptr; }

protected:
    void wrapImage(QRhi* rhi, QSize size, QVideoFrameFormat::PixelFormat pixelFormat, VkImage image, int layout)
    {
        _count = 1;
        const auto* desc = QVideoTextureHelper::textureDescription(pixelFormat);
        if (!desc) {
            return;
        }
        _textures[0].reset(rhi->newTexture(
            desc->rhiTextureFormat(0, rhi, QVideoTextureHelper::TextureDescription::FallbackPolicy::Disable), size, 1,
            {}));
        if (_textures[0] && !_textures[0]->createFrom({reinterpret_cast<quint64>(image), layout})) {
            _textures[0].reset();
        }
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
