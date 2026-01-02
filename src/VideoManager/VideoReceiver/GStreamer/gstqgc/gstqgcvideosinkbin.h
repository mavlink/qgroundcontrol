#pragma once

#include <gst/gstbin.h>

G_BEGIN_DECLS

#define GST_TYPE_QGC_VIDEO_SINK_BIN (gst_qgc_video_sink_bin_get_type())
G_DECLARE_FINAL_TYPE (GstQgcVideoSinkBin, gst_qgc_video_sink_bin, GST, QGC_VIDEO_SINK_BIN, GstBin)

struct _GstQgcVideoSinkBin
{
    GstBin parent;
    GstElement *glsinkbin;
    GstElement *qmlglsink;
};

G_END_DECLS
