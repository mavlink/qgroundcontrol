#pragma once

#include <gst/video/gstvideosink.h>

G_BEGIN_DECLS

#define GST_TYPE_QGC_Q_VIDEO_SINK (gst_qgc_q_video_sink_get_type())

/// \brief GstVideoSink that pushes decoded frames into a Qt QVideoSink; sole sink in qgcvideosinkbin.
/// Streaming-thread vfuncs MUST NOT touch QObject members directly (thread-affinity asserts) — hand
/// off via QMetaObject::invokeMethod / bus messages.
G_DECLARE_FINAL_TYPE(GstQgcQVideoSink, gst_qgc_q_video_sink, GST, QGC_Q_VIDEO_SINK, GstVideoSink)

G_END_DECLS
