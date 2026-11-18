#include "GstDmaBufVulkanImport.h"

#if defined(QGC_HAS_GST_DMABUF_VULKAN_IMPORT)

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <drm_fourcc.h>
#include <fcntl.h>
#include <limits>
#include <memory>
#include <private/qvideotexturehelper_p.h>
#include <rhi/qrhi.h>
#include <rhi/qrhi_platform.h>
#include <unistd.h>
#include <utility>

#include <QtCore/QByteArray>
#include <QtCore/QByteArrayList>
#include <QtCore/QLoggingCategory>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QVersionNumber>
#include <QtCore/QtEnvironmentVariables>
#include <QtGui/QVulkanFunctions>
#include <QtGui/QVulkanInstance>

#include "GstHwPathTelemetry.h"
#include "GstHwVideoBuffer.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GstDmaBufVulkanLog, "Video.GStreamer.HwBuffers.GstDmaBufVulkan")

namespace {

using GstDmaBufVulkan::DeviceFns;
using GstDmaBufVulkan::ImportFailure;
using GstHw::kMaxPlanes;

constexpr char kDeviceExtensionsVariable[] = "QT_VULKAN_DEVICE_EXTENSIONS";

// VK_EXT_image_drm_format_modifier also needs VK_KHR_image_format_list, which is core in the Vulkan 1.2 the import
// requires.
constexpr const char* kRequiredDeviceExtensions[] = {
    VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME,
    VK_EXT_EXTERNAL_MEMORY_DMA_BUF_EXTENSION_NAME,
    VK_EXT_IMAGE_DRM_FORMAT_MODIFIER_EXTENSION_NAME,
};

/// The formats planes are imported in: the VkFormat of the QRhiTexture Qt samples a plane through, keyed by the DRM
/// fourcc GstQtDmaBufImport::planeFourccs() names it with. Bytes are in memory order, which Qt's shaders swizzle.
struct PlaneFormat
{
    quint32 drmFourcc;
    QRhiTexture::Format rhiFormat;
    VkFormat vkFormat;
};

constexpr PlaneFormat kPlaneFormats[] = {
    {DRM_FORMAT_R8, QRhiTexture::R8, VK_FORMAT_R8_UNORM},
    {DRM_FORMAT_GR88, QRhiTexture::RG8, VK_FORMAT_R8G8_UNORM},
    {DRM_FORMAT_R16, QRhiTexture::R16, VK_FORMAT_R16_UNORM},
    {DRM_FORMAT_GR1616, QRhiTexture::RG16, VK_FORMAT_R16G16_UNORM},
    {DRM_FORMAT_ABGR8888, QRhiTexture::RGBA8, VK_FORMAT_R8G8B8A8_UNORM},
    {DRM_FORMAT_ARGB8888, QRhiTexture::BGRA8, VK_FORMAT_B8G8R8A8_UNORM},
};
constexpr std::size_t kPlaneFormatCount = std::size(kPlaneFormats);

/// Index into kPlaneFormats of the entry @p matches accepts; kPlaneFormatCount when there is none.
template <class Matches>
std::size_t planeFormatIndex(Matches&& matches)
{
    return static_cast<std::size_t>(std::find_if(std::begin(kPlaneFormats), std::end(kPlaneFormats), matches) -
                                    std::begin(kPlaneFormats));
}

constexpr quint64 explicitModifier(quint64 modifier) noexcept
{
    return (modifier == DRM_FORMAT_MOD_INVALID) ? DRM_FORMAT_MOD_LINEAR : modifier;
}

/// What the QRhi probed last imports; empty lists when its device can't import at all.
struct ImportDevice
{
    const QRhi* rhi = nullptr;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    /// Per kPlaneFormats entry, the modifiers the device takes it with as a sampled, single-plane dma-buf image.
    std::array<std::vector<quint64>, kPlaneFormatCount> modifiers;

    bool imports() const
    {
        return std::any_of(modifiers.begin(), modifiers.end(), [](const auto& list) { return !list.empty(); });
    }

    bool imports(std::size_t planeFormat, quint64 modifier) const
    {
        const auto& list = modifiers[planeFormat];
        return std::find(list.begin(), list.end(), modifier) != list.end();
    }
};

QMutex s_deviceMutex;
ImportDevice s_device;
// s_device.physicalDevice while that device imports: the streaming threads' check, without the lock.
std::atomic<void*> s_importDevice{nullptr};
const char s_deviceCleanupKey = 0;

GstHw::MapDiagnostics s_diag;
std::atomic<bool> s_loggedImportFailed{false};

/// True when @p physicalDevice has every extension of kRequiredDeviceExtensions and Qt was asked to enable them: QRhi
/// enables exactly the extensions QT_VULKAN_DEVICE_EXTENSIONS names that the device has, and Vulkan can't tell which
/// are enabled on a device somebody else created.
bool deviceExtensionsEnabled(QVulkanFunctions& functions, VkPhysicalDevice physicalDevice, QByteArray& missing)
{
    uint32_t count = 0;
    functions.vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &count, nullptr);
    std::vector<VkExtensionProperties> supported(count);
    if (functions.vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &count, supported.data()) !=
        VK_SUCCESS) {
        count = 0;
    }
    supported.resize(count);
    const QByteArrayList requested = qgetenv(kDeviceExtensionsVariable).split(';');
    for (const char* const extension : kRequiredDeviceExtensions) {
        const bool deviceHasIt = std::any_of(supported.begin(), supported.end(), [extension](const auto& candidate) {
            return qstrcmp(candidate.extensionName, extension) == 0;
        });
        if (!deviceHasIt || !requested.contains(extension)) {
            missing = QByteArray(extension) + (deviceHasIt ? " (not requested)" : " (not supported)");
            return false;
        }
    }
    return true;
}

/// The modifiers @p physicalDevice imports a dma-buf with as a sampled image of @p format with one memory plane, which
/// leaves out the layouts with auxiliary (compression) planes.
std::vector<quint64> queryImportableModifiers(QVulkanFunctions& functions, VkPhysicalDevice physicalDevice,
                                              VkFormat format)
{
    VkDrmFormatModifierPropertiesListEXT list = {};
    list.sType = VK_STRUCTURE_TYPE_DRM_FORMAT_MODIFIER_PROPERTIES_LIST_EXT;
    VkFormatProperties2 properties = {};
    properties.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2;
    properties.pNext = &list;
    functions.vkGetPhysicalDeviceFormatProperties2(physicalDevice, format, &properties);
    std::vector<VkDrmFormatModifierPropertiesEXT> candidates(list.drmFormatModifierCount);
    list.pDrmFormatModifierProperties = candidates.data();
    functions.vkGetPhysicalDeviceFormatProperties2(physicalDevice, format, &properties);
    candidates.resize(list.drmFormatModifierCount);

    std::vector<quint64> importable;
    for (const VkDrmFormatModifierPropertiesEXT& candidate : candidates) {
        if ((candidate.drmFormatModifierPlaneCount != 1) ||
            !(candidate.drmFormatModifierTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)) {
            continue;
        }
        VkPhysicalDeviceImageDrmFormatModifierInfoEXT modifierInfo = {};
        modifierInfo.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_DRM_FORMAT_MODIFIER_INFO_EXT;
        modifierInfo.drmFormatModifier = candidate.drmFormatModifier;
        modifierInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VkPhysicalDeviceExternalImageFormatInfo externalInfo = {};
        externalInfo.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_IMAGE_FORMAT_INFO;
        externalInfo.pNext = &modifierInfo;
        externalInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;
        VkPhysicalDeviceImageFormatInfo2 imageInfo = {};
        imageInfo.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2;
        imageInfo.pNext = &externalInfo;
        imageInfo.format = format;
        imageInfo.type = VK_IMAGE_TYPE_2D;
        imageInfo.tiling = VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT;
        imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
        VkExternalImageFormatProperties externalProperties = {};
        externalProperties.sType = VK_STRUCTURE_TYPE_EXTERNAL_IMAGE_FORMAT_PROPERTIES;
        VkImageFormatProperties2 imageProperties = {};
        imageProperties.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2;
        imageProperties.pNext = &externalProperties;
        if ((functions.vkGetPhysicalDeviceImageFormatProperties2(physicalDevice, &imageInfo, &imageProperties) ==
             VK_SUCCESS) &&
            (externalProperties.externalMemoryProperties.externalMemoryFeatures &
             VK_EXTERNAL_MEMORY_FEATURE_IMPORTABLE_BIT)) {
            importable.push_back(candidate.drmFormatModifier);
        }
    }
    return importable;
}

void forgetDevice(const QRhi* rhi)
{
    const QMutexLocker lock(&s_deviceMutex);
    if (s_device.rhi == rhi) {
        s_importDevice.store(nullptr, std::memory_order_release);
        s_device = {};
    }
}

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

/// The images of one frame and their memory, owned by one frame bundle, and the buffer they sample.
struct OwnedPlanes
{
    VkDevice dev = VK_NULL_HANDLE;
    std::array<VkImage, kMaxPlanes> images{};
    std::array<VkDeviceMemory, kMaxPlanes> memories{};
    /// Referenced until the GPU is done with the images, so the decoder can't write the next frame into it meanwhile.
    GstBuffer* buffer = nullptr;
    DeviceFns fns;
};

void destroyPlanes(const OwnedPlanes& owned)
{
    for (int i = 0; i < kMaxPlanes; ++i) {
        if (owned.images[i] != VK_NULL_HANDLE) {
            owned.fns.destroyImage(owned.dev, owned.images[i], nullptr);
        }
        if (owned.memories[i] != VK_NULL_HANDLE) {
            owned.fns.freeMemory(owned.dev, owned.memories[i], nullptr);
        }
    }
    if (owned.buffer) {
        gst_buffer_unref(owned.buffer);
    }
}

/// What a waiter thread takes of a retired frame: its buffer, and the fence of the GPU work that sampled its images.
struct RetiredBuffer
{
    VkDevice dev = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
    PFN_vkWaitForFences waitForFences = nullptr;
    GstBuffer* buffer = nullptr;
    /// Set once the waiter has let go of the buffer and is through with the fence.
    std::shared_ptr<std::atomic<bool>> done;
};

/// GFunc of retire()'s pool: lets go of the RetiredBuffer @p data's buffer once its fence signalled. vkWaitForFences
/// needs no external synchronization, so it runs beside the render thread's use of the device.
void waitForRetiredFence(gpointer data, gpointer /*userData*/)
{
    const std::unique_ptr<RetiredBuffer> retired(static_cast<RetiredBuffer*>(data));
    (void) retired->waitForFences(retired->dev, 1, &retired->fence, VK_TRUE, UINT64_MAX);
    if (retired->buffer) {
        gst_buffer_unref(retired->buffer);
    }
    retired->done->store(true, std::memory_order_release);
    retired->done->notify_all();
}

// Render-thread confined, like the QRhi whose queue the fences are submitted to.
struct RetiredPlanes
{
    OwnedPlanes owned;                              ///< Without its buffer, which the fence's waiter holds.
    VkFence fence = VK_NULL_HANDLE;
    std::shared_ptr<std::atomic<bool>> waiterDone;  ///< RetiredBuffer::done of the fence's waiter.
};

thread_local std::vector<RetiredPlanes> t_retiredPlanes;
const char s_retiredPlanesCleanupKey = 0;

void destroyRetired(const RetiredPlanes& retired)
{
    destroyPlanes(retired.owned);
    if (retired.fence != VK_NULL_HANDLE) {
        retired.owned.fns.destroyFence(retired.owned.dev, retired.fence, nullptr);
    }
}

// Destroys @p dev's retired planes whose fence signalled and whose waiter is through with it: vkDestroyFence needs the
// fence to itself. With @p waitAll (QRhi teardown) waits for both first, which also leaves no waiter in a call on the
// device that is about to be destroyed.
void reapRetired(VkDevice dev, bool waitAll)
{
    for (auto it = t_retiredPlanes.begin(); it != t_retiredPlanes.end();) {
        const OwnedPlanes& o = it->owned;
        if (o.dev != dev) {
            ++it;
            continue;
        }
        if (waitAll) {
            it->waiterDone->wait(false, std::memory_order_acquire);
            (void) o.fns.waitForFences(dev, 1, &it->fence, VK_TRUE, UINT64_MAX);
        } else if (!it->waiterDone->load(std::memory_order_acquire) ||
                   (o.fns.getFenceStatus(dev, it->fence) != VK_SUCCESS)) {
            ++it;
            continue;
        }
        destroyRetired(*it);
        it = t_retiredPlanes.erase(it);
    }
}

void retire(VkQueue queue, const OwnedPlanes& owned)
{
    // GLib's threads, as many as there are fences pending, as for the buffers of the GL import (unrefAfterGpuReads()
    // of GstDmaBufVideoBuffer.cc).
    static GThreadPool* const waiters = g_thread_pool_new(&waitForRetiredFence, nullptr, -1, FALSE, nullptr);
    RetiredPlanes retired;
    retired.owned = owned;
    const VkFenceCreateInfo fenceInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, 0};
    // A submit without batches signals its fence once all work already submitted to the queue has completed.
    if (!waiters || owned.fns.createFence(owned.dev, &fenceInfo, nullptr, &retired.fence) != VK_SUCCESS ||
        owned.fns.queueSubmit(queue, 0, nullptr, retired.fence) != VK_SUCCESS) {
        (void) owned.fns.queueWaitIdle(queue);
        destroyRetired(retired);
        return;
    }
    retired.owned.buffer = nullptr;
    retired.waiterDone = std::make_shared<std::atomic<bool>>(false);
    // A false return only says no thread could be started now: the wait stays queued.
    (void) g_thread_pool_push(waiters,
                              new RetiredBuffer{.dev = owned.dev,
                                                .fence = retired.fence,
                                                .waitForFences = owned.fns.waitForFences,
                                                .buffer = owned.buffer,
                                                .done = retired.waiterDone},
                              nullptr);
    t_retiredPlanes.push_back(std::move(retired));
    reapRetired(owned.dev, /*waitAll=*/false);
}

/// Release callback for a frame bundle owning @p owned (see GstVulkanOwnedFrameTextures::adoptVulkanResources). Qt
/// destroys a bundle when its pool slot is reused, which can be right after the frame that last sampled it was
/// submitted, so the callback queues a fence behind QRhi's submitted work. Another thread waits for the fence and then
/// lets go of the buffer, so the decoder gets it back without a later frame: one with a fixed pool may need it to
/// decode that frame. The images, their memory and the fence are destroyed on the render thread, which Vulkan requires
/// to be the only thread using them then: when a later bundle is released, and by a keyed QRhi cleanup callback, which
/// destroys what is left while the device still exists. Render thread only.
std::function<void()> deferredRelease(QRhi& rhi, VkQueue queue, const OwnedPlanes& owned)
{
    const VkDevice dev = owned.dev;
    // Keyed: one teardown hook per QRhi, run while its device is still alive.
    rhi.addCleanupCallback(&s_retiredPlanesCleanupKey, [dev](QRhi*) { reapRetired(dev, /*waitAll=*/true); });
    return [queue, owned] { retire(queue, owned); };
}

/// Creates on @p dev the image of one plane, @p size texels of @p format laid out as @p plane says, and binds it to the
/// plane's dma-buf, imported for it alone: a dedicated allocation, which some drivers require of dma-buf images.
/// Why it could not, with the cause logged once: the driver refused, or the plane's fd could not be duplicated. The
/// caller destroys what @p image and @p memory hold.
std::optional<ImportFailure> importPlane(const DeviceFns& fns, VkDevice dev, VkPhysicalDevice physicalDevice,
                                         const GstQtDmaBufImport::Plane& plane, VkFormat format, QSize size,
                                         VkImage& image, VkDeviceMemory& memory)
{
    VkSubresourceLayout layout = {};
    layout.offset = plane.offset;
    layout.rowPitch = plane.pitch;

    VkImageDrmFormatModifierExplicitCreateInfoEXT modifierInfo = {};
    modifierInfo.sType = VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_EXPLICIT_CREATE_INFO_EXT;
    modifierInfo.drmFormatModifier = explicitModifier(plane.modifier);
    modifierInfo.drmFormatModifierPlaneCount = 1;
    modifierInfo.pPlaneLayouts = &layout;

    VkExternalMemoryImageCreateInfo externalInfo = {};
    externalInfo.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
    externalInfo.pNext = &modifierInfo;
    externalInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;

    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.pNext = &externalInfo;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = format;
    imageInfo.extent = {static_cast<uint32_t>(size.width()), static_cast<uint32_t>(size.height()), 1};
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT;
    imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (const VkResult result = fns.createImage(dev, &imageInfo, nullptr, &image); result != VK_SUCCESS) {
        image = VK_NULL_HANDLE;
        QGC_HW_WARN_ONCE(GstDmaBufVulkanLog, s_loggedImportFailed,
                         "vkCreateImage failed (VkResult=" << result << "format=" << format << "modifier=" << Qt::hex
                                                           << modifierInfo.drmFormatModifier << Qt::dec << ")");
        return ImportFailure::Rejected;
    }

    // Vulkan owns an imported fd from a successful allocation on; until then it is ours to close.
    const int fd = ::fcntl(plane.fd, F_DUPFD_CLOEXEC, 0);
    if (fd < 0) {
        const int dupError = errno;
        QGC_HW_WARN_ONCE(GstDmaBufVulkanLog, s_loggedImportFailed,
                         "duplicating a plane's dma-buf fd failed: errno" << dupError
                                                                          << qUtf8Printable(qt_error_string(dupError))
                                                                          << "(the driver refused nothing)");
        return ImportFailure::FdDupFailed;
    }
    VkMemoryFdPropertiesKHR fdProperties = {};
    fdProperties.sType = VK_STRUCTURE_TYPE_MEMORY_FD_PROPERTIES_KHR;
    VkMemoryRequirements requirements = {};
    fns.getImageMemoryRequirements(dev, image, &requirements);
    VkPhysicalDeviceMemoryProperties memoryProperties = {};
    fns.getPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
    uint32_t memoryType = UINT32_MAX;
    if (fns.getMemoryFdProperties(dev, VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT, fd, &fdProperties) ==
        VK_SUCCESS) {
        const uint32_t allowed = requirements.memoryTypeBits & fdProperties.memoryTypeBits;
        for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
            if (allowed & (1u << i)) {
                memoryType = i;
                break;
            }
        }
    }

    VkImportMemoryFdInfoKHR importInfo = {};
    importInfo.sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR;
    importInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;
    importInfo.fd = fd;
    VkMemoryDedicatedAllocateInfo dedicatedInfo = {};
    dedicatedInfo.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO;
    dedicatedInfo.pNext = &importInfo;
    dedicatedInfo.image = image;
    VkMemoryAllocateInfo allocateInfo = {};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.pNext = &dedicatedInfo;
    allocateInfo.allocationSize = requirements.size;
    allocateInfo.memoryTypeIndex = memoryType;

    const VkResult allocated = (memoryType != UINT32_MAX) ? fns.allocateMemory(dev, &allocateInfo, nullptr, &memory)
                                                          : VK_ERROR_FORMAT_NOT_SUPPORTED;
    if (allocated != VK_SUCCESS) {
        memory = VK_NULL_HANDLE;
        ::close(fd);
        QGC_HW_WARN_ONCE(GstDmaBufVulkanLog, s_loggedImportFailed,
                         "importing a dma-buf as image memory failed (VkResult=" << allocated << ")");
        return ImportFailure::Rejected;
    }
    if (const VkResult bound = fns.bindImageMemory(dev, image, memory, 0); bound != VK_SUCCESS) {
        QGC_HW_WARN_ONCE(GstDmaBufVulkanLog, s_loggedImportFailed,
                         "vkBindImageMemory failed (VkResult=" << bound << ")");
        return ImportFailure::Rejected;
    }
    return std::nullopt;
}

}  // namespace

namespace GstDmaBufVulkan {

void requestDeviceExtensions()
{
    QByteArrayList extensions = qgetenv(kDeviceExtensionsVariable).split(';');
    extensions.removeAll(QByteArray());
    for (const char* const extension : kRequiredDeviceExtensions) {
        if (!extensions.contains(extension)) {
            extensions.append(extension);
        }
    }
    (void) qputenv(kDeviceExtensionsVariable, extensions.join(';'));
}

void probeDevice(QRhi& rhi)
{
    if (rhi.backend() != QRhi::Vulkan) {
        return;
    }
    ImportDevice device;
    device.rhi = &rhi;
    QByteArray unusable;
    const auto* handles = static_cast<const QRhiVulkanNativeHandles*>(rhi.nativeHandles());
    if (!handles || !handles->inst || (handles->physDev == VK_NULL_HANDLE)) {
        unusable = "QRhi exposes no Vulkan device";
    } else {
        device.physicalDevice = handles->physDev;
        QVulkanFunctions& functions = *handles->inst->functions();
        VkPhysicalDeviceProperties properties = {};
        functions.vkGetPhysicalDeviceProperties(handles->physDev, &properties);
        const QVersionNumber deviceVersion(VK_VERSION_MAJOR(properties.apiVersion),
                                           VK_VERSION_MINOR(properties.apiVersion));
        // The instance's version bounds the device functions an application may call.
        if (std::min(deviceVersion, handles->inst->apiVersion()) < QVersionNumber(1, 2)) {
            unusable = "it needs Vulkan 1.2";
        } else if (QByteArray missing; !deviceExtensionsEnabled(functions, handles->physDev, missing)) {
            unusable = "device extension " + missing;
        } else {
            for (std::size_t i = 0; i < kPlaneFormatCount; ++i) {
                device.modifiers[i] = queryImportableModifiers(functions, handles->physDev, kPlaneFormats[i].vkFormat);
            }
            if (!device.imports()) {
                unusable = "the device imports no plane format from a dma-buf";
            }
        }
    }

    if (unusable.isEmpty()) {
        qCDebug(GstDmaBufVulkanLog) << "DMABuf import available on" << rhi.driverInfo().deviceName;
    } else {
        qCDebug(GstDmaBufVulkanLog) << "DMABuf frames take the CPU path under this Vulkan device:" << unusable;
    }
    {
        const QMutexLocker lock(&s_deviceMutex);
        s_importDevice.store(device.imports() ? static_cast<void*>(device.physicalDevice) : nullptr,
                             std::memory_order_release);
        s_device = std::move(device);
    }
    // Keyed, so probing a QRhi again replaces the callback.
    rhi.addCleanupCallback(&s_deviceCleanupKey, [](QRhi* dying) { forgetDevice(dying); });
}

void* importDevice() noexcept
{
    return s_importDevice.load(std::memory_order_acquire);
}

bool canImport(const GstQtDmaBufImport::PlaneFourccs& planeFourccs, quint64 modifier)
{
    if (planeFourccs.count <= 0) {
        return false;
    }
    const QMutexLocker lock(&s_deviceMutex);
    for (int plane = 0; plane < planeFourccs.count; ++plane) {
        const quint32 fourcc = planeFourccs.fourcc[std::size_t(plane)];
        const std::size_t format =
            planeFormatIndex([fourcc](const PlaneFormat& candidate) { return candidate.drmFourcc == fourcc; });
        if ((format == kPlaneFormatCount) || !s_device.imports(format, explicitModifier(modifier))) {
            return false;
        }
    }
    return true;
}

std::vector<quint64> importableModifiers(quint32 planeFourcc)
{
    const std::size_t format =
        planeFormatIndex([planeFourcc](const PlaneFormat& candidate) { return candidate.drmFourcc == planeFourcc; });
    const QMutexLocker lock(&s_deviceMutex);
    return (format != kPlaneFormatCount) ? s_device.modifiers[format] : std::vector<quint64>();
}

void resetCachedState() noexcept
{
    s_diag.reset();
    s_loggedImportFailed.store(false, std::memory_order_relaxed);
    s_deviceFnsGeneration.fetch_add(1, std::memory_order_acq_rel);
}

QVideoFrameTexturesUPtr importTextures(GstHwVideoBuffer& source, QRhi& rhi, QVideoFrameTexturesUPtr& old,
                                       std::span<const GstQtDmaBufImport::Plane> planes,
                                       QVideoFrameFormat::PixelFormat pixelFormat, QSize frameSize, GstBuffer* buffer,
                                       ImportFailure& failure)
{
    failure = ImportFailure::NotImportable;
    const auto* handles = static_cast<const QRhiVulkanNativeHandles*>(rhi.nativeHandles());
    const auto* description = QVideoTextureHelper::textureDescription(pixelFormat);
    // Another window's QRhi, whose device wasn't probed, may lack the extensions or the layouts.
    if (!handles || !handles->inst || (handles->dev == VK_NULL_HANDLE) || (handles->gfxQueue == VK_NULL_HANDLE) ||
        (handles->physDev != importDevice()) || !description || planes.empty() ||
        (std::ssize(planes) != description->nplanes) || (planes.size() > std::size_t(kMaxPlanes))) {
        return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
    }
    const DeviceFns& fns = deviceFns(*handles->inst, handles->dev);
    if (!fns.ok()) {
        return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
    }

    OwnedPlanes owned;
    owned.dev = handles->dev;
    owned.fns = fns;
    bool imported = true;
    for (std::size_t i = 0; imported && (i < planes.size()); ++i) {
        // The format of the image view QRhi creates for the plane, which the image must have too.
        const QRhiTexture::Format rhiFormat = description->rhiTextureFormat(
            int(i), &rhi, QVideoTextureHelper::TextureDescription::FallbackPolicy::Disable);
        const std::size_t format =
            planeFormatIndex([rhiFormat](const PlaneFormat& candidate) { return candidate.rhiFormat == rhiFormat; });
        const bool importable = [&] {
            const QMutexLocker lock(&s_deviceMutex);
            return (format != kPlaneFormatCount) && s_device.imports(format, explicitModifier(planes[i].modifier));
        }();
        if (!importable) {
            imported = false;
            break;
        }
        if (const std::optional<ImportFailure> planeFailure =
                importPlane(fns, handles->dev, handles->physDev, planes[i], kPlaneFormats[format].vkFormat,
                            description->rhiPlaneSize(frameSize, int(i), &rhi), owned.images[i], owned.memories[i])) {
            failure = *planeFailure;
            imported = false;
        }
    }
    if (!imported) {
        destroyPlanes(owned);
        return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
    }

    const GstHwMapTail tail{.path = HwVideoBufferPath::DmaBuf,
                            .frameSize = frameSize,
                            .pixelFormat = pixelFormat,
                            .log = &GstDmaBufVulkanLog(),
                            .diag = &s_diag,
                            .preflight = false,
                            .successTag = "DMABuf->Vulkan"};
    bool adopted = false;
    QVideoFrameTexturesUPtr textures =
        GstHwFrameTexturesBase::finishMap<GstVulkanOwnedFrameTextures>(source, rhi, old, tail, nullptr, [&] {
            auto bundle = std::make_unique<GstVulkanOwnedFrameTextures>(
                &rhi, frameSize, pixelFormat, std::span<const VkImage>(owned.images.data(), planes.size()));
            if (bundle->allPlanesValid()) {
                owned.buffer = gst_buffer_ref(buffer);
                bundle->adoptVulkanResources(deferredRelease(rhi, handles->gfxQueue, owned));
                adopted = true;
            }
            return bundle;
        });
    if (!adopted) {
        destroyPlanes(owned);
    }
    return textures;
}

#ifdef QGC_UNITTEST_BUILD
std::optional<ImportFailure> importPlaneForTest(const DeviceFns& fns, const GstQtDmaBufImport::Plane& plane)
{
    OwnedPlanes owned;
    owned.fns = fns;
    const std::optional<ImportFailure> failure =
        importPlane(fns, VK_NULL_HANDLE, VK_NULL_HANDLE, plane, VK_FORMAT_R8_UNORM, QSize(64, 64), owned.images[0],
                    owned.memories[0]);
    destroyPlanes(owned);
    return failure;
}

std::function<void()> deferredReleaseForTest(QRhi& rhi, const DeviceFns& fns, GstBuffer* buffer)
{
    OwnedPlanes owned;
    // Any non-null handles: only @p fns is given them.
    owned.dev = VkDevice(quintptr(1));
    owned.images[0] = VkImage(quintptr(1));
    owned.memories[0] = VkDeviceMemory(quintptr(1));
    owned.buffer = gst_buffer_ref(buffer);
    owned.fns = fns;
    return deferredRelease(rhi, VK_NULL_HANDLE, owned);
}
#endif

}  // namespace GstDmaBufVulkan

#endif  // QGC_HAS_GST_DMABUF_VULKAN_IMPORT
