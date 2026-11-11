#pragma once

#include <QtCore/qglobal.h>

#if defined(QGC_HAS_GST_DMABUF_VULKAN_IMPORT)

#include <array>
#include <functional>
#include <gst/gst.h>
#include <span>
#include <vector>
#include <vulkan/vulkan.h>

#include <QtCore/QSize>
#include <QtMultimedia/QVideoFrameFormat>

#include "GstHwFrameTexturesBase.h"
#include "GstQtDmaBufImport.h"

/// DMABuf frames under a Vulkan QRhi. Each plane becomes a single-plane VkImage on Qt's device over the imported
/// dma-buf (VK_EXT_external_memory_dma_buf, VK_EXT_image_drm_format_modifier), in the format Qt samples that plane as,
/// which is what Qt's EGL importer does with one EGLImage per plane. Qt's device must have the extensions enabled, so
/// they are requested before Qt creates it, and what the device then imports is asked of it once.
namespace GstDmaBufVulkan {

/// The device extensions the import needs enabled on the VkDevice Qt renders with.
std::span<const char* const> requiredDeviceExtensions() noexcept;

/// Asks Qt to enable requiredDeviceExtensions() on the Vulkan devices it creates, by adding them to
/// QT_VULKAN_DEVICE_EXTENSIONS: QRhi enables the ones a device has. Call before Qt creates a Vulkan device.
void requestDeviceExtensions();

/// Learns what @p rhi's device imports, which holds until @p rhi is destroyed; a no-op for a QRhi of another backend.
/// The device imports when it has Vulkan 1.2 and the required extensions, QT_VULKAN_DEVICE_EXTENSIONS asked for them,
/// and it takes at least one plane format as a sampled dma-buf image. @p rhi's thread.
void probeDevice(QRhi& rhi);

/// The VkPhysicalDevice of the probed QRhi, as HwVideoBufferContext::dmaBufImporter stores it, while its device imports
/// dma-bufs; else null. Any thread.
void* importDevice() noexcept;

/// True when the probed device imports every plane of a frame sampled as @p planeFourccs
/// (GstQtDmaBufImport::planeFourccs()) in the layout @p modifier; no modifier (GstQtDmaBufImport::kImplicitModifier)
/// stands for LINEAR, as Vulkan takes no implicit layout. Any thread.
bool canImport(const GstQtDmaBufImport::PlaneFourccs& planeFourccs, quint64 modifier);

/// The modifiers the probed device imports a plane sampled as DRM @p planeFourcc with. Any thread.
std::vector<quint64> importableModifiers(quint32 planeFourcc);

/// Imports @p planes, which must match GstQtDmaBufImport::planeFourccs(@p pixelFormat), as textures of @p rhi that
/// sample the dma-bufs in place. @p source's sample and @p buffer stay referenced until the GPU has finished with the
/// textures. Without textures, @p rejected tells whether the driver refused the import. @p rhi's thread, Vulkan only.
QVideoFrameTexturesUPtr importTextures(GstHwVideoBuffer& source, QRhi& rhi, QVideoFrameTexturesUPtr& old,
                                       std::span<const GstQtDmaBufImport::Plane> planes,
                                       QVideoFrameFormat::PixelFormat pixelFormat, QSize frameSize, GstBuffer* buffer,
                                       bool& rejected);

/// Re-arms the once-logged warnings and makes every render thread resolve its Vulkan device functions again.
void resetCachedState() noexcept;

}  // namespace GstDmaBufVulkan

/// Owns the VkImages and VkDeviceMemory a DMABuf import created, one per plane; released with the bundle.
class GstVulkanOwnedFrameTextures final : public GstHwFrameTexturesBase
{
public:
    GstVulkanOwnedFrameTextures(QRhi* rhi, QSize size, QVideoFrameFormat::PixelFormat pixelFormat,
                                std::span<const VkImage> images)
    {
        // QRhi's first barrier transitions from this layout; UNDEFINED would let the driver discard the imported
        // contents, GENERAL preserves them.
        wrapPlanes(rhi, size, pixelFormat, images, VK_IMAGE_LAYOUT_GENERAL);
    }

    ~GstVulkanOwnedFrameTextures() override
    {
        // The QRhiTextures must be gone before the VkImages they wrap.
        for (auto& texture : _textures) {
            texture.reset();
        }
        if (_release) {
            _release();
        }
    }

    /// Transfers the VkImages and their memory into the bundle: @p release runs once, when Qt destroys the bundle.
    void adoptVulkanResources(std::function<void()> release) noexcept { _release = std::move(release); }

private:
    std::function<void()> _release;
};

#endif  // QGC_HAS_GST_DMABUF_VULKAN_IMPORT
