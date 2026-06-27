#include "GstVulkanContextBridge.h"

#include <QtGui/qtguiglobal.h>

#include "GstBridgePrimeRetry.h"
#include "GstContextBridgeCommon.h"

#if defined(QGC_HAS_GST_VULKAN_GPU_PATH) && QT_CONFIG(vulkan)

#include <QtCore/QLoggingCategory>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
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

namespace GstVulkanContextBridge {
namespace {

QMutex s_mutex;
GstVulkanInstance* s_instance = nullptr;
GstVulkanDevice* s_device = nullptr;
GstVulkanQueue* s_queue = nullptr;
GstBridgePrimeRetry::PrimeRetryState s_retry;

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
GstVulkanPhysicalDevice* matchPhysicalDevice(GstVulkanInstance* instance, VkPhysicalDevice want)
{
    const guint n = instance->n_physical_devices;
    for (guint i = 0; i < n; ++i) {
        GstVulkanPhysicalDevice* phys = gst_vulkan_physical_device_new(instance, i);
        if (!phys) {
            continue;
        }
        if (phys->device == want) {
            return phys;
        }
        gst_object_unref(phys);
    }
    return nullptr;
}

bool primeLocked()
{
    switch (GstBridgePrimeRetry::primeRetryGuard(s_retry)) {
        case GstBridgePrimeRetry::Decision::AlreadyPrimed:
            return true;
        case GstBridgePrimeRetry::Decision::GiveUp:
            return false;
        case GstBridgePrimeRetry::Decision::ShouldRetry:
            break;
    }

    SnapshotHandles nh;
    if (!readSnapshotHandles(nh)) {
        // Expected on the default GL RHI build; retry in case the Vulkan RHI initializes later.
        if (!GstBridgePrimeRetry::rearmRetry(s_retry) && GstBridgePrimeRetry::justGaveUp(s_retry)) {
            qCInfo(GstVulkanBridgeLog) << "active RHI is not Vulkan after" << s_retry.maxRetries
                                       << "retries; Vulkan bridge inactive";
        }
        return false;
    }

    auto bail = [](const char* what) -> bool {
        qCWarning(GstVulkanBridgeLog) << "Vulkan bridge prime failed:" << what;
        gst_clear_object(&s_queue);
        gst_clear_object(&s_device);
        gst_clear_object(&s_instance);
        s_retry.primeAttempted = false;
        return false;
    };

    s_instance = gst_vulkan_instance_new();
    if (!s_instance || !gst_vulkan_instance_open(s_instance, nullptr)) {
        return bail("gst_vulkan_instance_open");
    }

    GstVulkanPhysicalDevice* phys = matchPhysicalDevice(s_instance, nh.physDev);
    if (!phys) {
        return bail("no GstVulkanPhysicalDevice matches QRhi physDev");
    }
    s_device = gst_vulkan_device_new(phys);
    gst_object_unref(phys);
    if (!s_device || !gst_vulkan_device_open(s_device, nullptr)) {
        return bail("gst_vulkan_device_open");
    }
    s_queue = gst_vulkan_device_get_queue(s_device, nh.queueFamilyIdx, nh.queueIdx);
    if (!s_queue) {
        return bail("gst_vulkan_device_get_queue");
    }

    s_retry.primed = true;
    // Distinct-VkDevice limitation is by design until a wrap API lands; flag it once so the CPU fallback isn't mistaken
    // for a bug.
    qCInfo(GstVulkanBridgeLog) << "Vulkan bridge primed on QRhi physical device — note: gst VkDevice differs from "
                                  "QRhi VkDevice, so import falls back to CPU until same-device wrapping is available";
    return true;
}

GstVulkanInstance* refInstance()
{
    return s_instance ? GST_VULKAN_INSTANCE(gst_object_ref(s_instance)) : nullptr;
}

GstVulkanDevice* refDevice()
{
    return s_device ? GST_VULKAN_DEVICE(gst_object_ref(s_device)) : nullptr;
}

GstVulkanQueue* refQueue()
{
    return s_queue ? GST_VULKAN_QUEUE(gst_object_ref(s_queue)) : nullptr;
}

}  // namespace

namespace {

const char* const kContextTypes[] = {
    GST_VULKAN_INSTANCE_CONTEXT_TYPE_STR,
    GST_VULKAN_DEVICE_CONTEXT_TYPE_STR,
    GST_VULKAN_QUEUE_CONTEXT_TYPE_STR,
};

const QLoggingCategory& vtCat(void*)
{
    return GstVulkanBridgeLog();
}

QMutex& vtMutex(void*)
{
    return s_mutex;
}

bool vtPrime(void*)
{
    return primeLocked();
}

GstObject* vtRefObject(void*, const char* contextType)
{
    if (g_strcmp0(contextType, GST_VULKAN_INSTANCE_CONTEXT_TYPE_STR) == 0) {
        return GST_OBJECT(refInstance());
    }
    if (g_strcmp0(contextType, GST_VULKAN_DEVICE_CONTEXT_TYPE_STR) == 0) {
        return GST_OBJECT(refDevice());
    }
    return GST_OBJECT(refQueue());
}

GstContext* vtBuildContext(void*, const char* contextType, GstObject* object)
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

const GstContextBridge::BridgeVTable s_vtable = {
    "Vulkan", kContextTypes, 3, &vtCat, &vtMutex, &vtPrime, &vtRefObject, &vtBuildContext, nullptr,
};

}  // namespace

bool prime()
{
    QMutexLocker lock(&s_mutex);
    return primeLocked();
}

GstBusSyncReply handleSyncMessage(GstMessage* message)
{
    return GstContextBridge::handleSyncMessage(s_vtable, nullptr, message);
}

bool answerContextQuery(GstQuery* query)
{
    return GstContextBridge::answerContextQuery(s_vtable, nullptr, query);
}

void reset()
{
    QMutexLocker lock(&s_mutex);
    gst_clear_object(&s_queue);
    gst_clear_object(&s_device);
    gst_clear_object(&s_instance);
    GstBridgePrimeRetry::resetRetry(s_retry);
    qCDebug(GstVulkanBridgeLog) << "Vulkan bridge reset";
}

void rearm()
{
    QMutexLocker lock(&s_mutex);
    GstBridgePrimeRetry::rearmAfterExhaustion(s_retry);
}

namespace {
struct VulkanBridgeRegistrar
{
    VulkanBridgeRegistrar()
    {
        GstContextBridge::registerBridge(GstVulkanBridgeLog(), "Vulkan", &GstVulkanContextBridge::handleSyncMessage,
                                         &GstVulkanContextBridge::reset);
    }
};

static VulkanBridgeRegistrar s_vulkanBridgeRegistrar;
}  // namespace

}  // namespace GstVulkanContextBridge

#endif  // QGC_HAS_GST_VULKAN_GPU_PATH
