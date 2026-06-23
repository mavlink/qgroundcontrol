#pragma once

#include <glib.h>
#include <gst/gst.h>

#include "GStreamer.h"

namespace GStreamer
{
    gboolean isValidRtspUri(const gchar *uri_str);

    bool isHardwareDecoderFactory(GstElementFactory *factory);

    void setCodecPriorities(VideoDecoderOptions option);
}
