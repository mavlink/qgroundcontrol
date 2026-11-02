#include "GStreamerUdpSrcWorkaround.h"

#include <atomic>
#include <gst/base/gstbasesrc.h>
#include <gst/gst.h>
#include <mutex>

#include "GstScoped.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GStreamerUdpSrcWorkaroundLog, "Video.GStreamer.UdpSrcWorkaround")

namespace GStreamer {

namespace {

using BaseSrcUnlockFunc = gboolean (*)(GstBaseSrc*);

BaseSrcUnlockFunc s_udpSrcUnlock = nullptr;
BaseSrcUnlockFunc s_udpSrcUnlockStop = nullptr;
// Not the object lock: a distro build that backported the upstream fix already takes it inside these calls.
std::mutex s_udpSrcUnlockMutex;

gboolean _udpSrcUnlockSerialized(GstBaseSrc* src)
{
    const std::lock_guard lock(s_udpSrcUnlockMutex);
    return s_udpSrcUnlock(src);
}

gboolean _udpSrcUnlockStopSerialized(GstBaseSrc* src)
{
    const std::lock_guard lock(s_udpSrcUnlockMutex);
    return s_udpSrcUnlockStop(src);
}

std::atomic_bool s_serialized{false};

}  // namespace

void serializeUdpSrcUnlock()
{
    static std::once_flag s_once;
    std::call_once(s_once, [] {
        const GstFactoryPtr factory = adoptFactory(gst_element_factory_find("udpsrc"));
        if (!factory || gst_plugin_feature_check_version(GST_PLUGIN_FEATURE(factory.get()), 1, 24, 10)) {
            return;
        }

        const GstFeaturePtr loaded = adoptFeature(gst_plugin_feature_load(GST_PLUGIN_FEATURE(factory.get())));
        const GType type =
            loaded ? gst_element_factory_get_element_type(GST_ELEMENT_FACTORY(loaded.get())) : G_TYPE_INVALID;
        if (!g_type_is_a(type, GST_TYPE_BASE_SRC)) {
            qCWarning(GStreamerUdpSrcWorkaroundLog)
                << "udpsrc failed to load; stopping a UDP stream right after it starts may crash";
            return;
        }

        // Never released, so the patched class outlives every udpsrc.
        auto* const klass = static_cast<GstBaseSrcClass*>(g_type_class_ref(type));
        s_udpSrcUnlock = klass->unlock;
        s_udpSrcUnlockStop = klass->unlock_stop;
        if (s_udpSrcUnlock && s_udpSrcUnlockStop) {
            klass->unlock = _udpSrcUnlockSerialized;
            klass->unlock_stop = _udpSrcUnlockStopSerialized;
            s_serialized.store(true, std::memory_order_release);
            qCDebug(GStreamerUdpSrcWorkaroundLog)
                << "Serialized udpsrc unlock/unlock_stop (gst-plugins-good before 1.24.10)";
        }
    });
}

bool udpSrcUnlockSerialized()
{
    return s_serialized.load(std::memory_order_acquire);
}

}  // namespace GStreamer
