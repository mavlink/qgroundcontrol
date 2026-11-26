#pragma once

#include <QtCore/qglobal.h>

#if defined(QGC_HAS_GST_DMABUF_VULKAN_IMPORT)

#include <array>
#include <functional>
#include <gst/gst.h>
#include <memory>
#include <optional>
#include <span>
#include <vector>
#include <vulkan/vulkan.h>

#include <QtCore/QSize>
#include <QtMultimedia/QVideoFrameFormat>

#include "GstHwVideoBuffer.h"
#include "GstQtDmaBufImport.h"

/// DMABuf frames under a Vulkan QRhi. Each plane becomes a single-plane VkImage on Qt's device over the imported
/// dma-buf (VK_EXT_external_memory_dma_buf, VK_EXT_image_drm_format_modifier), in the format Qt samples that plane as,
/// which is what Qt's EGL importer does with one EGLImage per plane. Qt's device must have the extensions enabled, so
/// they are requested before Qt creates it, and what the device then imports is asked of it once.
namespace GstDmaBufVulkan {

/// Asks Qt to enable the device extensions the import needs on the Vulkan devices it creates, by adding them to
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
/// textures. Without textures, @p failure tells why: NotImportable, Rejected or FdDupFailed. @p rhi's thread, Vulkan
/// only.
QVideoFrameTexturesUPtr importTextures(GstHwVideoBuffer& source, QRhi& rhi,
                                       std::span<const GstQtDmaBufImport::Plane> planes,
                                       QVideoFrameFormat::PixelFormat pixelFormat, QSize frameSize, GstBuffer* buffer,
                                       GstQtDmaBufImport::ImportOutcome& failure);

/// Re-arms the once-logged warnings and makes every render thread resolve its Vulkan device functions again.
void resetCachedState() noexcept;

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

#ifdef QGC_UNITTEST_BUILD
/// Imports @p plane as importTextures() does each plane of a frame, on a device that is @p fns alone, and destroys
/// what it created. Why the import failed; nothing when it didn't.
std::optional<GstQtDmaBufImport::ImportOutcome> importPlaneForTest(const DeviceFns& fns,
                                                                   const GstQtDmaBufImport::Plane& plane);
/// The release callback importTextures() gives a frame bundle, for a bundle whose one image and its memory are on a
/// device that is @p fns alone and of @p rhi, and which holds a new reference to @p buffer.
std::function<void()> deferredReleaseForTest(QRhi& rhi, const DeviceFns& fns, GstBuffer* buffer);
#endif

}  // namespace GstDmaBufVulkan

/// Owns the VkImages and VkDeviceMemory a DMABuf import created, one per plane, and the frame's sample; released with
/// the textures. Wraps the images itself, where every other path has Qt wrap its handles: QRhi must take them in
/// VK_IMAGE_LAYOUT_GENERAL, and Qt's helper passes layout 0.
class GstVulkanOwnedFrameTextures final : public QVideoFrameTextures
{
public:
    /// Planes QRhi rejects stay null, as do null images.
    GstVulkanOwnedFrameTextures(QRhi* rhi, QSize size, QVideoFrameFormat::PixelFormat pixelFormat,
                                std::span<const VkImage> images);
    ~GstVulkanOwnedFrameTextures() override;

    QRhiTexture* texture(uint plane) const override;

    /// True when the textures have planes and every one of them wrapped its image.
    bool allPlanesValid() const noexcept;

    /// Transfers the VkImages and their memory into the textures: @p release runs once, when Qt destroys them.
    void adoptVulkanResources(std::function<void()> release) noexcept { _release = std::move(release); }

    /// Takes over @p sample's reference, held until destruction.
    void holdSample(GstSample* sample) noexcept;

private:
    std::array<std::unique_ptr<QRhiTexture>, GstHw::kMaxPlanes> _textures;
    int _count = 0;
    std::function<void()> _release;
    GstSample* _sample = nullptr;
};

#endif  // QGC_HAS_GST_DMABUF_VULKAN_IMPORT
