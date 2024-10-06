/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_TYPE_QGC_VIDEO_SINK_BIN (gst_qgc_video_sink_bin_get_type())
G_DECLARE_FINAL_TYPE (GstQgcVideoSinkBin, gst_qgc_video_sink_bin, GST, QGC_VIDEO_SINK_BIN, GstBin)
#define GST_QGC_VIDEO_SINK_BIN_CAST(obj) ((GstQgcVideoSinkBin *)(obj))
#define GST_QGC_VIDEO_SINK_BIN(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_QGC_VIDEO_SINK_BIN, GstQgcVideoSinkBin))
#define GST_QGC_VIDEO_SINK_BIN_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_QGC_VIDEO_SINK_BIN, GstQgcVideoSinkBinClass))
#define GST_IS_QGC_VIDEO_SINK_BIN(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_QGC_VIDEO_SINK_BIN))
#define GST_IS_QGC_VIDEO_SINK_BIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_QGC_VIDEO_SINK_BIN))

struct _GstQgcVideoSinkBin {
    GstBin bin;
    GstElement *glupload;
    GstElement *qmlglsink;
};

struct _GstQgcVideoSinkBinClass {
    GstBinClass parent_class;
};

GstQgcVideoSinkBin* gst_qgc_video_sink_bin_new(void);

G_END_DECLS
