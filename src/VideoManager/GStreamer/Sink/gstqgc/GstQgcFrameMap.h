#pragma once

#include <gst/gst.h>
#include <gst/video/video-info.h>
#include <gst/video/video.h>

#include <QtCore/QRect>
#include <QtCore/QSize>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoFrameFormat>

#include "HwBuffers/common/CpuVideoFramePool.h"
#include "HwBuffers/common/HwBuffers.h"

/// Sample-to-frame helpers for qgcqvideosink's show_frame; pure functions, streaming-thread safe.

QVideoFrameFormat::ColorSpace toQtColorSpace(GstVideoColorMatrix matrix);
QVideoFrameFormat::ColorTransfer toQtColorTransfer(GstVideoTransferFunction transfer);
QVideoFrameFormat::PixelFormat toQtPixelFormat(GstVideoFormat fmt);
QVideoFrameFormat::ColorRange toQtColorRange(GstVideoColorRange range);

/// Apply rotation + mirror flags derived from GstVideoOrientationMethod.
void applyOrientationToFrame(QVideoFrame& frame, GstVideoOrientationMethod method);

/// The stream's orientation + PTS/duration timing; @p streamOrientation is GstVideoOrientationMethod cast to int.
void applyOrientationAndTiming(QVideoFrame& frame, GstBuffer* buffer, int streamOrientation);

/// Set color-space/transfer/range from gst-video colorimetry + HDR (MaxCLL preferred);
/// infers BT.601/709 from height when caps omit it.
void applyColorimetry(QVideoFrameFormat& format, const GstVideoInfo& info, GstCaps* caps);

/// The frame a buffer holds and the picture in it, as upstream's sinks read them (glimagesink, waylandsink): the frame
/// is as large as the buffer's GstVideoMeta, which a decoder pads beyond the caps (1088 rows for 1080) or leaves
/// uncropped, and the picture is its GstVideoCropMeta, else the caps' size at the frame's origin. A
/// GstVideoAffineTransformationMeta of scales, translations and flips, as Android's amcvideodec attaches for its
/// SurfaceTexture's crop and orientation, narrows the picture to the part it shows and tells which way to flip it.
/// That part is the whole picture, squeezed with the decoder's padding into a texture of the caps' size (1080 rows
/// and 8 of padding into 1080, of which 1072 show), so the picture keeps the size it had: pictureSize.
struct FrameGeometry
{
    QSize frameSize;
    QRect picture;
    QSize pictureSize;
    bool flipHorizontally = false;
    bool flipVertically = false;

    friend bool operator==(const FrameGeometry&, const FrameGeometry&) = default;
};

/// @p buffer's geometry under caps of @p capsInfo; the caps' own without any of those metas. An affine meta that also
/// turns, shears or projects is left out.
FrameGeometry frameGeometry(GstBuffer* buffer, const GstVideoInfo& capsInfo);

/// Flips @p frame's picture as @p geometry tells, ahead of the rotation and mirroring the frame already has.
void applyFlips(QVideoFrame& frame, const FrameGeometry& geometry);

/// @p format for frames of @p geometry: its frame size, with the picture as the viewport.
QVideoFrameFormat withFrameGeometry(QVideoFrameFormat format, const FrameGeometry& geometry);

/// Sample-to-frame result (GPU wins when it can, CPU fallback). mapSampleToFrame() does NOT take the caller's refs on
/// @p buffer/@p caps: a GPU frame holds its own GstSample, a zero-copy CPU frame its own buffer ref. A frame no GPU
/// path takes whose layout the CPU can't read (a tiled DMA_DRM modifier) comes back invalid, with noCpuLayout set.
struct MappedFrame
{
    QVideoFrame frame;
    HwVideoBufferPath gpuPath = HwVideoBufferPath::None;  ///< None for a CPU frame, and for one that failed to map
    bool demoted = false;      ///< A GPU path was requested for this stream but this frame fell back to CPU.
    bool noCpuLayout = false;  ///< Dropped instead of read as linear rows, which would show it scrambled.
};

MappedFrame mapSampleToFrame(GstBuffer* buffer, GstCaps* caps, const GstVideoInfo& info,
                             const QVideoFrameFormat& format, const HwVideoBufferContext& hwContext,
                             const CpuVideoFramePool::PinnedFrameCounter& pinnedCpuFrames,
                             HwResolvedPathCache* pathCache = nullptr);
