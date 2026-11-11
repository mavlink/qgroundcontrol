#include "GstVulkanContextBridge.h"

#include <QtGui/qtguiglobal.h>

#if defined(QGC_HAS_GST_VULKAN_GPU_PATH) && QT_CONFIG(vulkan)

#include <array>
#include <cstring>
#include <memory>
#include <rhi/qrhi.h>
#include <vulkan/vulkan.h>

#include <QtCore/QLoggingCategory>

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

/// What names a physical device in any VkInstance. A VkPhysicalDevice handle doesn't: it belongs to the instance that
/// enumerated it, and GStreamer enumerates through one of its own.
struct DeviceIdentity
{
    quint32 vendorId = 0;
    quint32 deviceId = 0;
    std::array<quint64, 2> pipelineCacheUuid{};  ///< 8 bytes each in memory order, as QGCRhiCapture publishes it.

    friend bool operator==(const DeviceIdentity&, const DeviceIdentity&) = default;
};

DeviceIdentity identityOf(const VkPhysicalDeviceProperties& properties)
{
    DeviceIdentity identity{.vendorId = properties.vendorID, .deviceId = properties.deviceID};
    static_assert(sizeof(identity.pipelineCacheUuid) == sizeof(properties.pipelineCacheUUID));
    std::memcpy(identity.pipelineCacheUuid.data(), properties.pipelineCacheUUID, sizeof(identity.pipelineCacheUuid));
    return identity;
}

/// The device Qt's Vulkan scene graph renders on, as QGCRhiCapture published it from the render thread; false while Qt
/// doesn't render with Vulkan. primeLocked() runs on streaming threads, where the QRhi itself must not be touched.
bool readQtDevice(DeviceIdentity& out)
{
    auto& snap = QGCRhiCapture::deviceSnapshot();
    // backend is published last (release); acquire-loading it first guarantees the identity fields are visible.
    if (snap.backend.load(std::memory_order_acquire) != static_cast<int>(QRhi::Vulkan)) {
        return false;
    }
    out.vendorId = snap.vkVendorId.load(std::memory_order_acquire);
    out.deviceId = snap.vkDeviceId.load(std::memory_order_acquire);
    for (std::size_t i = 0; i < out.pipelineCacheUuid.size(); ++i) {
        out.pipelineCacheUuid[i] = snap.vkPipelineCacheUuid[i].load(std::memory_order_acquire);
    }
    return true;
}

using PhysicalDevicePtr = std::unique_ptr<GstVulkanPhysicalDevice, GStreamer::GstObjectDeleter>;

/// The physical device of @p instance with the properties @p want; the first one when identical GPUs share them.
PhysicalDevicePtr findPhysicalDevice(GstVulkanInstance* instance, const DeviceIdentity& want)
{
    const guint n = instance->n_physical_devices;
    for (guint i = 0; i < n; ++i) {
        PhysicalDevicePtr phys{gst_vulkan_physical_device_new(instance, i)};
        if (phys && (identityOf(phys->properties) == want)) {
            return phys;
        }
    }
    return {};
}

/// GstVulkanDeviceForEachQueueFunc: keeps the first graphics queue in the GstVulkanQueue* @p userData points to. The
/// queues are those GStreamer created on its own VkDevice, where Qt's queue indices mean nothing.
gboolean takeGraphicsQueue(GstVulkanDevice* device, GstVulkanQueue* queue, gpointer userData)
{
    const GstVulkanPhysicalDevice* const phys = device->physical_device;
    if ((queue->family >= phys->n_queue_families) ||
        !(phys->queue_family_props[queue->family].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
        return TRUE;
    }
    *static_cast<GstVulkanQueue**>(userData) = GST_VULKAN_QUEUE(gst_object_ref(queue));
    return FALSE;
}

/// GstVulkanInstance::create-device handler: the bridge's device, @p bridgeDevice, for an element of the bridge's
/// instance that found no device on its pads.
GstVulkanDevice* provideBridgeDevice(GstVulkanInstance* /*instance*/, guint /*deviceIndex*/, gpointer bridgeDevice)
{
    return GST_VULKAN_DEVICE(gst_object_ref(bridgeDevice));
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
    /// provideBridgeDevice() on _instance; 0 where gst-vulkan has no create-device signal.
    gulong _createDeviceHandler = 0;
};

constinit VulkanContextBridge s_bridge;

bool VulkanContextBridge::primeLocked()
{
    DeviceIdentity qtDevice;
    if (!readQtDevice(qtDevice)) {
        // Expected on the default GL RHI build; retry in case the Vulkan RHI initializes later.
        if (!retryWithinBudget() && retriesJustExhausted()) {
            qCInfo(GstVulkanBridgeLog) << "active RHI is not Vulkan after" << kMaxPrimeRetries
                                       << "retries; Vulkan bridge inactive";
        }
        return false;
    }

    // Nothing below changes while Qt renders on this device, so a failure latches the bridge off until reset() instead
    // of repeating the work and the warning for every context request. The elements then open GStreamer's default
    // device; QRhi can sample their images no more than this bridge's.
    auto disable = [this](const char* what) -> bool {
        qCWarning(GstVulkanBridgeLog) << "Vulkan bridge disabled:" << what;
        clearLocked();
        return false;
    };

    _instance = gst_vulkan_instance_new();
    if (!_instance || !gst_vulkan_instance_open(_instance, nullptr)) {
        return disable("gst_vulkan_instance_open failed");
    }

    const PhysicalDevicePtr phys = findPhysicalDevice(_instance, qtDevice);
    if (!phys) {
        return disable("GStreamer's Vulkan instance lists no physical device with the properties of Qt's");
    }
    _device = gst_vulkan_device_new(phys.get());
    if (!_device || !gst_vulkan_device_open(_device, nullptr)) {
        return disable("gst_vulkan_device_open failed");
    }
    gst_vulkan_device_foreach_queue(_device, &takeGraphicsQueue, &_queue);
    if (!_queue) {
        return disable("GStreamer's Vulkan device has no graphics queue");
    }

    // gst-vulkan asks for an instance on the bus, but for a device on its pads only: an element that isn't linked to
    // the sink yet opens one of its own, on the first physical device. From 1.26 its instance's create-device signal
    // makes that device instead. Looked up by name, so this builds against older releases too. The handler holds
    // the device, which holds the instance: clearLocked() disconnects it.
    if (g_signal_lookup("create-device", GST_TYPE_VULKAN_INSTANCE) != 0) {
        _createDeviceHandler = g_signal_connect_data(
            _instance, "create-device", G_CALLBACK(&provideBridgeDevice), gst_object_ref(_device),
            +[](gpointer device, GClosure*) { gst_object_unref(device); }, GConnectFlags(0));
    }

    qCInfo(GstVulkanBridgeLog) << "Vulkan bridge primed on Qt's physical device" << phys->properties.deviceName
                               << "with a VkDevice of GStreamer's own: gst-vulkan can't wrap Qt's, so QRhi can't "
                                  "sample VulkanImage frames";
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
    if (_createDeviceHandler != 0) {
        g_signal_handler_disconnect(_instance, _createDeviceHandler);
        _createDeviceHandler = 0;
    }
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
