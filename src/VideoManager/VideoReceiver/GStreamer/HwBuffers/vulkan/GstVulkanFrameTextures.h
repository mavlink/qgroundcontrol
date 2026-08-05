#pragma once

#include <QtGui/qtguiglobal.h>

#if defined(QGC_HAS_GST_VULKAN_GPU_PATH) && QT_CONFIG(vulkan)

#include <QtCore/QSize>
#include <QtMultimedia/QVideoFrameFormat>
#include <private/qvideotexturehelper_p.h>
#include <rhi/qrhi.h>
#include <vulkan/vulkan.h>

#include "GstHwFrameTexturesBase.h"

/// Wraps a single multi-plane VkImage as a QRhiTexture (NV12/P010/RGBA). Owning and borrowing variants share this
/// wrap; only the image lifetime differs. `_textures[0]` is null when the wrap fails, surfaced via valid().
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

/// Owns the imported VkImage + backing VkDeviceMemory (DMABuf zero-copy import); destroyed when Qt finishes the frame.
class GstVulkanOwnedFrameTextures final : public GstVulkanFrameTexturesBase
{
public:
    GstVulkanOwnedFrameTextures(QRhi* rhi, QSize size, QVideoFrameFormat::PixelFormat pixelFormat, VkImage image)
    {
        wrapImage(rhi, size, pixelFormat, image, VK_IMAGE_LAYOUT_UNDEFINED);
    }

    ~GstVulkanOwnedFrameTextures() override { releaseVulkan(); }

    void onFrameEndInvoked() override
    {
        releaseVulkan();
        GstHwFrameTexturesBase::onFrameEndInvoked();
    }

    HwVideoBufferPath sourcePath() const override { return HwVideoBufferPath::DmaBuf; }

    /// Transfers VkImage/VkDeviceMemory ownership into the bundle for deferred destruction.
    void adoptVulkanResources(VkDevice dev, VkImage image, VkDeviceMemory memory, PFN_vkDestroyImage destroyImage,
                              PFN_vkFreeMemory freeMemory) noexcept
    {
        _dev = dev;
        _image = image;
        _memory = memory;
        _destroyImage = destroyImage;
        _freeMemory = freeMemory;
    }

private:
    void releaseVulkan()
    {
        // QRhiTexture must be gone before the VkImage it wraps; reset it first.
        _textures[0].reset();
        if (_image != VK_NULL_HANDLE && _destroyImage) {
            _destroyImage(_dev, _image, nullptr);
        }
        if (_memory != VK_NULL_HANDLE && _freeMemory) {
            _freeMemory(_dev, _memory, nullptr);
        }
        _image = VK_NULL_HANDLE;
        _memory = VK_NULL_HANDLE;
    }

    VkDevice _dev = VK_NULL_HANDLE;
    VkImage _image = VK_NULL_HANDLE;
    VkDeviceMemory _memory = VK_NULL_HANDLE;
    PFN_vkDestroyImage _destroyImage = nullptr;
    PFN_vkFreeMemory _freeMemory = nullptr;
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
