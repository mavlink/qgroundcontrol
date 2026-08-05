#pragma once

#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoFrameFormat>
#include <gst/gst.h>
#include <gst/video/video-info.h>
#include <gst/video/video.h>

#include "HwBuffers/common/HwBuffers.h"

/// Sample-to-frame helpers for qgcqvideosink's show_frame; pure functions, streaming-thread safe.

QVideoFrameFormat::ColorSpace toQtColorSpace(GstVideoColorMatrix matrix);
QVideoFrameFormat::ColorTransfer toQtColorTransfer(GstVideoTransferFunction transfer);
QVideoFrameFormat::PixelFormat toQtPixelFormat(GstVideoFormat fmt);
QVideoFrameFormat::ColorRange toQtColorRange(GstVideoColorRange range);

/// Apply rotation + mirror flags derived from GstVideoOrientationMethod.
void applyOrientationToFrame(QVideoFrame& frame, GstVideoOrientationMethod method);

/// Per-frame orientation (meta wins, else stream fallback) + PTS/duration timing;
/// @p streamOrientation is GstVideoOrientationMethod cast to int.
void applyOrientationAndTiming(QVideoFrame& frame, GstBuffer* buffer, int streamOrientation);

/// Set color-space/transfer/range from gst-video colorimetry + HDR (MaxCLL preferred);
/// infers BT.601/709 from height when caps omit it.
void applyColorimetry(QVideoFrameFormat& format, const GstVideoInfo& info, GstCaps* caps);

/// Apply video crop meta to @p format's viewport. Pass-through when no crop meta present.
QVideoFrameFormat applyCropMeta(QVideoFrameFormat format, GstBuffer* buffer);

/// Sample-to-frame result (GPU wins when it can, CPU fallback). mapSampleToFrame() does NOT ref
/// @p buffer — GPU wraps it in a transient GstSample (own ref), CPU copies; caller keeps @p buffer/@p caps.
struct MappedFrame
{
    QVideoFrame frame;
    enum class Source
    {
        Cpu,
        Gpu
    } source = Source::Cpu;
#if defined(QGC_HAS_ANY_GPU_PATH)
    HwVideoBufferPath gpuPath = HwVideoBufferPath::None;
    bool demoted = false;  ///< A GPU path was requested for this stream but this frame fell back to CPU.
#endif
};

MappedFrame mapSampleToFrame(GstBuffer* buffer, GstCaps* caps, const GstVideoInfo& info,
                             const QVideoFrameFormat& format, const HwVideoBufferContext& hwContext,
                             HwResolvedPathCache* pathCache = nullptr) noexcept;
