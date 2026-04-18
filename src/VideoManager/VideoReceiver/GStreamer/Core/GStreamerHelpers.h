#pragma once

#include <glib.h>
#include <gst/gst.h>

#include "GStreamer.h"
#include "GstObjectPtr.h"

namespace GStreamer {
gboolean isValidRtspUri(const gchar* uri_str);

bool isHardwareDecoderFactory(GstElementFactory* factory);

void setCodecPriorities(VideoDecoderOptions option);

/// Remove an element from its parent bin, setting it to NULL state first.
/// No-op if the element has no parent.
inline void gstRemoveFromParent(GstElement* element)
{
    if (!element)
        return;
    GstObject* parent = gst_element_get_parent(element);
    if (!parent)
        return;
    gst_element_set_state(element, GST_STATE_NULL);
    gst_bin_remove(GST_BIN(parent), element);
    gst_object_unref(parent);
}

/// Get the first src pad of an element, or an empty wrapper if none exist yet.
/// Caller owns the returned ref (wrapped in GstNonFloatingPtr for RAII).
inline GstNonFloatingPtr<GstPad> gstFirstSrcPad(GstElement* element)
{
    GstIterator* it = gst_element_iterate_src_pads(element);
    GValue vpad = G_VALUE_INIT;
    GstPad* pad = nullptr;
    if (gst_iterator_next(it, &vpad) == GST_ITERATOR_OK) {
        pad = GST_PAD(g_value_get_object(&vpad));
        gst_object_ref(pad);
        g_value_reset(&vpad);
    }
    g_value_unset(&vpad);
    gst_iterator_free(it);
    return GstNonFloatingPtr<GstPad>(pad);
}
}  // namespace GStreamer
