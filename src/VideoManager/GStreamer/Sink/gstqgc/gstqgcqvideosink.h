#pragma once

#include <gst/video/gstvideosink.h>

G_BEGIN_DECLS

#define GST_TYPE_QGC_Q_VIDEO_SINK (gst_qgc_q_video_sink_get_type())

/// \brief GstVideoSink that pushes decoded frames into a Qt QVideoSink; the sink of the bin GstQgc::makeSinkBin()
/// builds. Streaming-thread vfuncs MUST NOT touch QObject members directly (thread-affinity asserts) — hand off via
/// QMetaObject::invokeMethod / bus messages.
G_DECLARE_FINAL_TYPE(GstQgcQVideoSink, gst_qgc_q_video_sink, GST, QGC_Q_VIDEO_SINK, GstVideoSink)

G_END_DECLS

namespace GstQgc {

enum class GpuBinMode;
struct SinkEnvironment;

/// For the sink bin, before it links @p sink: the GPU variant in front of the sink, and whether the sink asks for
/// square pixels in system memory. Both shape the caps the sink offers.
void configureSinkCaps(GstElement* sink, GpuBinMode gpuBin, bool squarePixels);

/// Has @p sink read @p environment from now on, in place of the process's (currentSinkEnvironment()), which it
/// follows otherwise. For a sink bin built with one (SinkBinConfig::environment).
void setSinkEnvironment(GstElement* sink, const SinkEnvironment& environment);

/// Populate an ALLOCATION query with qgcqvideosink's consumed metas and its pool entry: the buffers the sink holds at
/// once, with a pool when the query needs one. False, with the query untouched, when it has no caps that describe
/// video. Called from the sink's propose_allocation vmethod.
[[nodiscard]] bool populateAllocationQuery(GstQuery* query);

}  // namespace GstQgc
