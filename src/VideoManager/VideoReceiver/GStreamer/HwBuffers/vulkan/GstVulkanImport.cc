#include "GstVulkanImport.h"

#if defined(QGC_HAS_GST_VULKAN_GPU_PATH) && QT_CONFIG(vulkan)

#include <rhi/qrhi.h>
#include <rhi/qrhi_platform.h>
#include <vector>

#include <QtGui/QVulkanInstance>

#include "GstHwPathTelemetry.h"
#include "GstHwVideoBuffer.h"

namespace GstVulkanImport {

namespace {

// Bumped by resetDeviceFns() so every render thread re-resolves after a device reset.
std::atomic<quint64> s_deviceFnsGeneration{0};

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

}  // namespace

bool DeviceFns::ok() const noexcept
{
    return createImage && destroyImage && allocateMemory && freeMemory && bindImageMemory &&
           getImageMemoryRequirements && getMemoryFdProperties && getPhysicalDeviceMemoryProperties && createFence &&
           destroyFence && getFenceStatus && waitForFences && queueSubmit && queueWaitIdle;
}

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

void resetDeviceFns() noexcept
{
    s_deviceFnsGeneration.fetch_add(1, std::memory_order_acq_rel);
}

std::function<void()> deferredRelease(QRhi& rhi, OwnedImage owned)
{
    const auto* nh = static_cast<const QRhiVulkanNativeHandles*>(rhi.nativeHandles());
    const VkQueue queue = nh ? nh->gfxQueue : VK_NULL_HANDLE;
    const VkDevice dev = owned.dev;
    // Keyed: one teardown hook per QRhi, run while its device is still alive.
    rhi.addCleanupCallback(&s_retiredImagesCleanupKey, [dev](QRhi*) { reapRetired(dev, /*waitAll=*/true); });
    return [queue, owned] { retire(queue, owned); };
}

bool admitImport(HwVideoBufferPath path, bool producerWritesComplete, const QLoggingCategory& log,
                 std::atomic<bool>& warned)
{
    if (producerWritesComplete) {
        return true;
    }
    GstHwPathTelemetry::recordFallbackReason(path, GstHwPathTelemetry::HwFallbackReason::VulkanNoSync);
    QGC_HW_WARN_ONCE(log, warned, "Vulkan import refused: producer writes not known complete — CPU fallback");
    return false;
}

}  // namespace GstVulkanImport

#endif  // QGC_HAS_GST_VULKAN_GPU_PATH && QT_CONFIG(vulkan)
