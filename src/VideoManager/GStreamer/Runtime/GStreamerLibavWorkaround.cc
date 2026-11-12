#include "GStreamerLibavWorkaround.h"

#include <gst/gst.h>

namespace GStreamer {

QLatin1StringView libavcodecVersion()
{
#ifdef QGC_GST_LIBAVCODEC_VERSION
    return QLatin1StringView(QGC_GST_LIBAVCODEC_VERSION);
#else
    return {};
#endif
}

bool avoidHevcWavefrontDeadlock(GstElement* decoder, QLatin1StringView version)
{
    if (version != QLatin1StringView("61.19.100")) {
        return false;
    }

    GstElementFactory* const factory = decoder ? gst_element_get_factory(decoder) : nullptr;
    if (!factory || (g_strcmp0(gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(factory)), "avdec_h265") != 0)) {
        return false;
    }

    g_object_set(decoder, "max-threads", 1, nullptr);
    return true;
}

}  // namespace GStreamer
