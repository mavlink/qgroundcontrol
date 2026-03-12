#pragma once

#include <gst/gst.h>

#include "GStreamer.h"

namespace GStreamer
{
    gboolean isValidRtspUri(const gchar *uri_str);

    bool isHardwareDecoderFactory(GstElementFactory *factory);

    void logDecoderRanks();

    void setCodecPriorities(VideoDecoderOptions option);
}
