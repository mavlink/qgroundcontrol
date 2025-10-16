/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <glib.h>
// Needed for GstElementFactory
#include <gst/gst.h>

namespace GStreamer
{
    gboolean is_valid_rtsp_uri(const gchar *uri_str);

    // Returns true if the given factory likely represents a hardware-accelerated decoder.
    // Heuristics: checks metadata/klass for "Hardware" and common vendor tags in the factory name.
    bool is_hardware_decoder_factory(GstElementFactory *factory);
}
