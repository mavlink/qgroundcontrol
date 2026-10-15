#pragma once

#include <QtGui/qtguiglobal.h>

#if defined(QGC_HAS_GST_VULKAN_GPU_PATH) && QT_CONFIG(vulkan)

#include <atomic>
#include <functional>
#include <vulkan/vulkan.h>

#include <QtCore/QLoggingCategory>

#include "GstHwVideoBufferFactory.h"

class QRhi;
class QVulkanInstance;

/// Helpers shared by the Vulkan imports (gst-vulkan images in vulkan/, DMABuf fds in dmabuf/).
namespace GstVulkanImport {

/// Entry points an import needs. Device-level ones come from vkGetDeviceProcAddr for the QRhi's VkDevice: an
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

    bool ok() const noexcept;
};

/// @p dev's entry points, cached per render thread (each window's QRhi owns its VkDevice) and re-resolved when that
/// thread's device changes or after resetDeviceFns().
const DeviceFns& deviceFns(QVulkanInstance& inst, VkDevice dev);

/// Forces every render thread to re-resolve its entry points (device reset).
void resetDeviceFns() noexcept;

/// An imported image and its memory, owned by one frame bundle.
struct OwnedImage
{
    VkDevice dev = VK_NULL_HANDLE;
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    DeviceFns fns;
};

/// Release callback for a frame bundle owning @p owned (see GstVulkanOwnedFrameTextures::adoptVulkanResources). Qt
/// destroys a bundle when its pool slot is reused, which can be right after the frame that last sampled it was
/// submitted, so the callback queues a fence behind QRhi's submitted work and destroys the image once it signals; a
/// keyed QRhi cleanup callback destroys what is left while the device still exists. Render thread only.
std::function<void()> deferredRelease(QRhi& rhi, OwnedImage owned);

/// Shared "no import without producer sync" policy: QRhi's queue does not wait on a foreign producer's writes, so an
/// import whose writes are not known complete is refused (VulkanNoSync fallback, warned once via @p warned).
bool admitImport(HwVideoBufferPath path, bool producerWritesComplete, const QLoggingCategory& log,
                 std::atomic<bool>& warned);

}  // namespace GstVulkanImport

#endif  // QGC_HAS_GST_VULKAN_GPU_PATH && QT_CONFIG(vulkan)
