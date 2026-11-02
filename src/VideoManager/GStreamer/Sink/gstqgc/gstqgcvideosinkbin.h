#pragma once

#include <gst/gstbin.h>

G_BEGIN_DECLS

#define GST_TYPE_QGC_VIDEO_SINK_BIN (gst_qgc_video_sink_bin_get_type())
G_DECLARE_FINAL_TYPE(GstQgcVideoSinkBin, gst_qgc_video_sink_bin, GST, QGC_VIDEO_SINK_BIN, GstBin)

/// Returns the internal qgcqvideosink element, transfer-full (caller unrefs); NULL if not yet constructed.
GstElement* gst_qgc_video_sink_bin_get_qvideosink(GstQgcVideoSinkBin* self);

G_END_DECLS
