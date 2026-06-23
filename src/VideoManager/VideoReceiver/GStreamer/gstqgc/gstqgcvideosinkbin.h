#pragma once

#include <gst/gstbin.h>

G_BEGIN_DECLS

#define GST_TYPE_QGC_VIDEO_SINK_BIN (gst_qgc_video_sink_bin_get_type())
G_DECLARE_FINAL_TYPE (GstQgcVideoSinkBin, gst_qgc_video_sink_bin, GST, QGC_VIDEO_SINK_BIN, GstBin)

struct _GstQgcVideoSinkBin
{
    GstBin parent;
    GstElement *videoconvert;
    GstElement *glupload;
    GstElement *appsink;
    /// PAR=1/1 capsfilter inserted between videoconvert and appsink on the CPU branch.
    /// Suppressed when the disable-par construct property is TRUE (workaround for v4l2
    /// drivers without VIDIOC_CROPCAP that deadlock negotiation when PAR is forced).
    GstElement *par_capsfilter;
    gboolean gpu_zerocopy;
    /// Construct-only override for the CPU branch's videoconvert factory. Empty/NULL
    /// means auto-probe (SoC-native imxvideoconvert_g2d / nvvidconv → videoconvert).
    gchar *conversion_element;
    gboolean disable_par;
};

/// Returns the internal appsink element with a ref. Caller unrefs (transfer-full,
/// matching gst_bin_get_by_name semantics). Returns NULL if the bin is not
/// fully constructed yet.
GstElement *gst_qgc_video_sink_bin_get_appsink(GstQgcVideoSinkBin *self);

G_END_DECLS
