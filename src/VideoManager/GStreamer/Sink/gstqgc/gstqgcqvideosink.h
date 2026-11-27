#pragma once

#include <gst/video/gstvideosink.h>

#include <QtCore/QSize>

G_BEGIN_DECLS

#define GST_TYPE_QGC_Q_VIDEO_SINK (gst_qgc_q_video_sink_get_type())

/// \brief GstVideoSink that pushes decoded frames into a Qt QVideoSink; the sink of the bin GstQgc::makeSinkBin()
/// builds. Streaming-thread vfuncs MUST NOT touch QObject members directly (thread-affinity asserts) — hand off via
/// QMetaObject::invokeMethod / bus messages. It has no properties of its own: QGC drives it through the functions
/// below, which leave any other element alone.
G_DECLARE_FINAL_TYPE(GstQgcQVideoSink, gst_qgc_q_video_sink, GST, QGC_Q_VIDEO_SINK, GstVideoSink)

G_END_DECLS

class QVideoSink;

namespace GstQgc {

enum class GpuBinMode;
struct SinkEnvironment;
struct SinkStats;

/// A new qgcqvideosink, floating. With @p gpuZeroCopy it imports frames in GPU memory without a copy where a zero-copy
/// path takes them, else it maps every frame through the CPU; fixed for the sink's life.
GstElement* makeVideoSink(bool gpuZeroCopy);

/// Whether @p sink was made with gpuZeroCopy.
bool sinkGpuZeroCopy(GstElement* sink);

/// For the sink bin, before it links @p sink: the GPU variant in front of the sink, and whether the sink asks for
/// square pixels in system memory. Both shape the caps the sink offers.
void configureSinkCaps(GstElement* sink, GpuBinMode gpuBin, bool squarePixels);

/// Has @p sink read @p environment from now on, in place of the process's (currentSinkEnvironment()), which it
/// follows otherwise. For a sink bin built with one (SinkBinConfig::environment).
void setSinkEnvironment(GstElement* sink, const SinkEnvironment& environment);

/// Has @p sink deliver its frames to @p target from now on; nullptr stops delivering. @p target is the caller's, and is
/// never dereferenced but to queue a frame to it.
void setSinkTarget(GstElement* sink, QVideoSink* target);
/// The QVideoSink @p sink delivers to; null when none.
QVideoSink* sinkTarget(GstElement* sink);

/// An inactive @p sink drops its frames instead of delivering them. Active to begin with.
void setSinkActive(GstElement* sink, bool active);
bool sinkActive(GstElement* sink);

/// The size of the picture in @p sink's last buffer, before its orientation: the crop meta's, else the caps'. Invalid
/// before caps. A frame's viewport is smaller where a transformation meta leaves out the padding of a squeezed texture.
QSize sinkPictureSize(GstElement* sink);

/// @p sink's frame accounting now; all zero for another element.
SinkStats sinkStats(GstElement* sink);

/// Populate an ALLOCATION query with qgcqvideosink's consumed metas and its pool entry: the buffers the sink holds at
/// once, with a pool when the query needs one. False, with the query untouched, when it has no caps that describe
/// video. Called from the sink's propose_allocation vmethod.
[[nodiscard]] bool populateAllocationQuery(GstQuery* query);

}  // namespace GstQgc
