#pragma once

#include <gst/video/gstvideosink.h>
#include <gst/video/video-info.h>

G_BEGIN_DECLS

#define GST_TYPE_QGC_Q_VIDEO_SINK (gst_qgc_q_video_sink_get_type())
G_DECLARE_FINAL_TYPE(GstQgcQVideoSink, gst_qgc_q_video_sink, GST, QGC_Q_VIDEO_SINK, GstVideoSink)

/// \brief GstVideoSink that pushes decoded frames into a Qt QVideoSink; sole sink in qgcvideosinkbin.
/// Streaming-thread vfuncs MUST NOT touch QObject members directly (thread-affinity asserts) — hand
/// off via QMetaObject::invokeMethod / bus messages.
struct _GstQgcQVideoSink
{
    GstVideoSink parent;

    // Properties backed inline; GObject's property system is the cross-thread boundary (GST_OBJECT_LOCK).
    gpointer qvideosink;  // QVideoSink* (caller-owned; never unref'd by us)
    gboolean active;
    gboolean gpu_zerocopy;

    // Cached negotiated state (set_caps writes, show_frame reads, both streaming-thread,
    // serialised by GstBaseSink). `priv` holds heap C++ non-POD state so this struct stays POD.
    gboolean caps_valid;
    GstVideoInfo video_info;
    gpointer priv;  // owned (new/delete in instance_init / finalize)
};

G_END_DECLS

#ifdef __cplusplus
struct HwVideoBufferContext;

/// Install the GPU zero-copy context; pushed once from the GUI thread under GST_OBJECT_LOCK
/// so the streaming-thread show_frame snapshot stays race-free.
void gst_qgc_q_video_sink_set_hw_context(GstQgcQVideoSink* self, const HwVideoBufferContext& ctx);
#endif
