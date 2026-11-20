#include "GStreamerLibavWorkaround.h"

#include <gst/gst.h>

#include "GStreamerHelpers.h"

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

    if (g_strcmp0(factoryNameOf(GST_OBJECT_CAST(decoder)), "avdec_h265") != 0) {
        return false;
    }

    g_object_set(decoder, "max-threads", 1, nullptr);
    return true;
}

}  // namespace GStreamer
