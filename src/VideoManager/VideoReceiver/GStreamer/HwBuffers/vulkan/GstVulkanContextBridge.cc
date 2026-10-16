#include "GstVulkanContextBridge.h"

#include <QtGui/qtguiglobal.h>

#if defined(QGC_HAS_GST_VULKAN_GPU_PATH) && QT_CONFIG(vulkan)

#include <QtCore/QLoggingCategory>
#include <rhi/qrhi.h>
#include <vulkan/vulkan.h>

// gstvkdecoder.h's `slots` member collides with Qt's `slots` keyword macro — see GstVulkanVideoBuffer.cc.
#pragma push_macro("slots")
#undef slots
// gstvkqueue.h (gst-vulkan 1.24.x) omits G_BEGIN_DECLS, so gst_context_set_vulkan_queue would get C++
// linkage and fail to link against the C symbol; the explicit extern "C" restores it (harmless for the
// sibling headers that already self-guard).
extern "C" {
#include <gst/vulkan/gstvkdevice.h>
#include <gst/vulkan/gstvkimagememory.h>
#include <gst/vulkan/gstvkinstance.h>
#include <gst/vulkan/gstvkphysicaldevice.h>
#include <gst/vulkan/gstvkqueue.h>
}
#pragma pop_macro("slots")

#include "QGCLoggingCategory.h"
#include "QGCRhiCapture.h"

QGC_LOGGING_CATEGORY(GstVulkanBridgeLog, "Video.GStreamer.HwBuffers.GstVulkanBridge")

namespace {

// Vulkan handles snapshotted render-side by QGCRhiCapture::populateSnapshot(); primeLocked() runs on the bus-sync
// streaming thread, so it must never dereference the cached QRhi* (races render-thread QRhi teardown). Desktop Linux
// pins GL in Platform::initialize (Vulkan import is dormant), so on Linux this is the expected no-op path.
struct SnapshotHandles
{
    VkPhysicalDevice physDev = VK_NULL_HANDLE;
    quint32 queueFamilyIdx = 0;
    quint32 queueIdx = 0;
};

bool readSnapshotHandles(SnapshotHandles& out)
{
    auto& snap = QGCRhiCapture::deviceSnapshot();
    // backend is published last (release); acquire-loading it first guarantees the handle fields are visible.
    if (snap.backend.load(std::memory_order_acquire) != static_cast<int>(QRhi::Vulkan)) {
        return false;
    }
    out.physDev = static_cast<VkPhysicalDevice>(snap.vkPhysicalDevice.load(std::memory_order_acquire));
    out.queueFamilyIdx = snap.vkQueueFamilyIdx.load(std::memory_order_acquire);
    out.queueIdx = snap.vkQueueIdx.load(std::memory_order_acquire);
    return out.physDev != VK_NULL_HANDLE;
}

// gst-vulkan 1.24 exposes no wrapped-instance/device constructor, so we cannot hand vulkanh26xdec QRhi's *own*
// VkInstance/VkDevice. We build a gst instance, then bind a GstVulkanDevice to the GstVulkanPhysicalDevice matching
// QRhi's physDev. That yields same-physical-device but a distinct VkDevice; the importer's device-match guard then
// routes those (foreign-VkDevice) frames to CPU. True same-VkDevice zero-copy needs a wrap API not present here.
using PhysicalDevicePtr = std::unique_ptr<GstVulkanPhysicalDevice, GStreamer::GstObjectDeleter>;

PhysicalDevicePtr matchPhysicalDevice(GstVulkanInstance* instance, VkPhysicalDevice want)
{
    const guint n = instance->n_physical_devices;
    for (guint i = 0; i < n; ++i) {
        PhysicalDevicePtr phys{gst_vulkan_physical_device_new(instance, i)};
        if (phys && (phys->device == want)) {
            return phys;
        }
    }
    return {};
}

constexpr const char* kContextTypes[] = {
    GST_VULKAN_INSTANCE_CONTEXT_TYPE_STR,
    GST_VULKAN_DEVICE_CONTEXT_TYPE_STR,
    GST_VULKAN_QUEUE_CONTEXT_TYPE_STR,
};

class VulkanContextBridge final : public GstContextBridgeBase
{
public:
    constexpr VulkanContextBridge() noexcept
        : GstContextBridgeBase("Vulkan", kContextTypes, &GstVulkanBridgeLog)
    {}

private:
    bool primeLocked() override;
    GstObject* refObject(const char* contextType) override;
    GstContext* buildContext(const char* contextType, GstObject* object) override;
    void clearLocked() override;

    GstVulkanInstance* _instance = nullptr;
    GstVulkanDevice* _device = nullptr;
    GstVulkanQueue* _queue = nullptr;
};

constinit VulkanContextBridge s_bridge;

bool VulkanContextBridge::primeLocked()
{
    SnapshotHandles nh;
    if (!readSnapshotHandles(nh)) {
        // Expected on the default GL RHI build; retry in case the Vulkan RHI initializes later.
        if (!retryWithinBudget() && retriesJustExhausted()) {
            qCInfo(GstVulkanBridgeLog) << "active RHI is not Vulkan after" << kMaxPrimeRetries
                                       << "retries; Vulkan bridge inactive";
        }
        return false;
    }

    auto bail = [this](const char* what) -> bool {
        qCWarning(GstVulkanBridgeLog) << "Vulkan bridge prime failed:" << what;
        clearLocked();
        return retryLater();
    };

    _instance = gst_vulkan_instance_new();
    if (!_instance || !gst_vulkan_instance_open(_instance, nullptr)) {
        return bail("gst_vulkan_instance_open");
    }

    const PhysicalDevicePtr phys = matchPhysicalDevice(_instance, nh.physDev);
    if (!phys) {
        return bail("no GstVulkanPhysicalDevice matches QRhi physDev");
    }
    _device = gst_vulkan_device_new(phys.get());
    if (!_device || !gst_vulkan_device_open(_device, nullptr)) {
        return bail("gst_vulkan_device_open");
    }
    _queue = gst_vulkan_device_get_queue(_device, nh.queueFamilyIdx, nh.queueIdx);
    if (!_queue) {
        return bail("gst_vulkan_device_get_queue");
    }

    // Distinct-VkDevice limitation is by design until a wrap API lands; flag it once so the CPU fallback isn't mistaken
    // for a bug.
    qCInfo(GstVulkanBridgeLog) << "Vulkan bridge primed on QRhi physical device — note: gst VkDevice differs from "
                                  "QRhi VkDevice, so import falls back to CPU until same-device wrapping is available";
    return true;
}

GstObject* VulkanContextBridge::refObject(const char* contextType)
{
    GstObject* object = GST_OBJECT(_queue);
    if (g_strcmp0(contextType, GST_VULKAN_INSTANCE_CONTEXT_TYPE_STR) == 0) {
        object = GST_OBJECT(_instance);
    } else if (g_strcmp0(contextType, GST_VULKAN_DEVICE_CONTEXT_TYPE_STR) == 0) {
        object = GST_OBJECT(_device);
    }
    return object ? GST_OBJECT(gst_object_ref(object)) : nullptr;
}

GstContext* VulkanContextBridge::buildContext(const char* contextType, GstObject* object)
{
    if (g_strcmp0(contextType, GST_VULKAN_INSTANCE_CONTEXT_TYPE_STR) == 0) {
        GstContext* ctx = gst_context_new(GST_VULKAN_INSTANCE_CONTEXT_TYPE_STR, TRUE);
        gst_context_set_vulkan_instance(ctx, GST_VULKAN_INSTANCE(object));
        return ctx;
    }
    if (g_strcmp0(contextType, GST_VULKAN_DEVICE_CONTEXT_TYPE_STR) == 0) {
        GstContext* ctx = gst_context_new(GST_VULKAN_DEVICE_CONTEXT_TYPE_STR, TRUE);
        gst_context_set_vulkan_device(ctx, GST_VULKAN_DEVICE(object));
        return ctx;
    }
    GstContext* ctx = gst_context_new(GST_VULKAN_QUEUE_CONTEXT_TYPE_STR, TRUE);
    gst_context_set_vulkan_queue(ctx, GST_VULKAN_QUEUE(object));
    return ctx;
}

void VulkanContextBridge::clearLocked()
{
    gst_clear_object(&_queue);
    gst_clear_object(&_device);
    gst_clear_object(&_instance);
}

}  // namespace

GstContextBridgeBase& GstVulkanContextBridge::instance()
{
    return s_bridge;
}

#endif  // QGC_HAS_GST_VULKAN_GPU_PATH
