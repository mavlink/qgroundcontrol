#pragma once

#include <gst/gstbin.h>

G_BEGIN_DECLS

#define GST_TYPE_QGC_VIDEO_SINK_BIN (gst_qgc_video_sink_bin_get_type())
G_DECLARE_FINAL_TYPE(GstQgcVideoSinkBin, gst_qgc_video_sink_bin, GST, QGC_VIDEO_SINK_BIN, GstBin)

struct _GstQgcVideoSinkBin
{
    GstBin parent;
    GstElement* videoconvert;
    GstElement* glupload;
    /// qgcqvideosink terminal element; caller-set "qvideosink" property targets a QVideoSink.
    GstElement* videosink;
    /// Format-restriction capsfilter before videosink; qgcqvideosink's pad template is CAPS_ANY,
    /// so without it upstream could negotiate formats Qt cannot render.
    GstElement* format_capsfilter;
    /// PAR=1/1 capsfilter on the CPU branch; suppressed when disable-par is TRUE (v4l2 drivers
    /// without VIDIOC_CROPCAP deadlock when PAR is forced).
    GstElement* par_capsfilter;
    gboolean gpu_zerocopy;
    /// Construct-only videoconvert factory override; empty/NULL auto-probes
    /// (imxvideoconvert_g2d/nvvidconv -> videoconvert).
    gchar* conversion_element;
    gboolean disable_par;
    /// Proxied to the inner basesink's "sync" so callers can configure clock sync on the bin.
    gboolean sync;
    /// Proxied to the inner basesink's "qos"; default FALSE (off) preserves the existing no-QoS behavior.
    gboolean qos;
    /// Proxied to the inner basesink's "processing-deadline" (ns); default matches the basesink default.
    guint64 processing_deadline;
};

/// Returns the internal qgcqvideosink element, transfer-full (caller unrefs); NULL if not yet constructed.
GstElement* gst_qgc_video_sink_bin_get_qvideosink(GstQgcVideoSinkBin* self);

/// Whether the bin built its GPU zero-copy pipeline (mirrors "gpu-zerocopy"); NULL-safe (FALSE).
gboolean gst_qgc_video_sink_bin_get_gpu_zerocopy(GstElement* bin);

#ifdef QGC_GST_BUILD_TESTING
gboolean gst_qgc_video_sink_bin_rejects_failed_adopt_for_test();
#endif

G_END_DECLS
