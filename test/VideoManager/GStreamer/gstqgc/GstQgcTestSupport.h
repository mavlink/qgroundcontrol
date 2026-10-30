#pragma once

#ifdef QGC_GST_STREAMING

#include <gst/gst.h>

#include <QtCore/QString>
#include <QtCore/QtTypes>
#include <QtGui/QColor>
#include <QtMultimedia/QVideoFrameFormat>

#include "GstScoped.h"
#include "gstqgc/gstqgcqvideosink.h"
#include "gstqgc/gstqgcvideosinkbin.h"

class QVideoSink;

/// qgcqvideosink helpers shared by the test classes that drive it.
namespace GstQgcTest {

/// What reached the QVideoSink in runPipelineThroughQVideoSink().
struct PipelineRunResult
{
    int frameCount = 0;
    QVideoFrameFormat lastFormat;
    qint64 lastStartTime = -1;
    QColor centerPixel;
    bool centerPixelValid = false;
    bool eos = false;
    QString errorMessage;
};

/// Plays "videotestsrc @p sourceProperties ! @p capsLine ! qgcvideosinkbin" to EOS with the bin bound to @p videoSink,
/// as VideoManager binds it, and records the frames that reach the QVideoSink.
PipelineRunResult runPipelineThroughQVideoSink(QVideoSink& videoSink, const char* capsLine, int numBuffers = 5,
                                               bool gpuZerocopy = false, const char* sourceProperties = "",
                                               bool captureCenterPixel = false);

/// The qgcqvideosink inside the qgcvideosinkbin @p sinkBin.
inline GStreamer::GstElementPtr qvideosink(GstElement* sinkBin)
{
    return GStreamer::adoptElement(gst_qgc_video_sink_bin_get_qvideosink(GST_QGC_VIDEO_SINK_BIN(sinkBin)));
}

inline guint64 framesDelivered(GstElement* qvideosink)
{
    guint64 delivered = 0;
    g_object_get(qvideosink, "frames-delivered", &delivered, nullptr);
    return delivered;
}

/// A guint64 field of the sink's "frame-stats" structure; -1 if the field is missing.
inline qint64 sinkStat(GstElement* qvideosink, const char* field)
{
    const GStreamer::GstStructurePtr stats = GstQgc::frameStats(qvideosink);
    guint64 value = 0;
    const bool found = stats && gst_structure_get_uint64(stats.get(), field, &value);
    return found ? static_cast<qint64>(value) : -1;
}

}  // namespace GstQgcTest

#endif
