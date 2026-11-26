#include "GstQgcFrameMap.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <gst/gst.h>
#include <gst/video/gstvideoaffinetransformationmeta.h>
#include <gst/video/gstvideometa.h>
#include <memory>
#include <utility>

#include <QtCore/QtNumeric>
#include <QtMultimedia/QAbstractVideoBuffer>
#include <QtMultimedia/QVideoFrame>

#include "QGCLoggingCategory.h"

#if defined(QGC_HAS_ANY_GPU_PATH)
#include <private/qhwvideobuffer_p.h>
#endif

QGC_LOGGING_CATEGORY(GstQgcFrameMapLog, "Video.GStreamer.FrameMap")

namespace {

/// Refs the source GstBuffer and maps its system memory through to Qt. The decoder buffer stays pinned for the frame's
/// lifetime, as the GPU paths pin theirs by anchoring the sample.
class MapThroughGstVideoBuffer final : public QAbstractVideoBuffer
{
public:
    MapThroughGstVideoBuffer(GstBuffer* buffer, const GstVideoInfo& info, QVideoFrameFormat format)
        : _buffer(gst_buffer_ref(buffer))
        , _info(info)
        , _format(std::move(format))
    {}

    ~MapThroughGstVideoBuffer() override
    {
        _cpuView.unmap();
        gst_buffer_unref(_buffer);
    }

    MapData map(QVideoFrame::MapMode mode) override
    {
        return (mode == QVideoFrame::ReadOnly) ? _cpuView.map(_buffer, _info) : MapData{};
    }

    void unmap() override { _cpuView.unmap(); }

    QVideoFrameFormat format() const override { return _format; }

private:
    GstBuffer* _buffer;
    GstVideoInfo _info;
    QVideoFrameFormat _format;
    GstHw::FrameMapping _cpuView;
};

/// Narrows @p geometry's picture to the part @p matrix shows of it, and notes its flips. The matrix moves the picture's
/// corners, in a unit square with y up (GstVideoAffineTransformationMeta); the texture stays where it is. So the
/// display, top left to bottom right, shows the picture from bx to bx + ax across and from by to by + ay down, as
/// fractions of the picture; a negative extent is a flip. False for a matrix that does more than scale, move and flip.
bool applyAffineMatrix(FrameGeometry& geometry, const gfloat* matrix)
{
    if (!qFuzzyIsNull(matrix[1]) || !qFuzzyIsNull(matrix[4]) || !qFuzzyIsNull(matrix[3]) || !qFuzzyIsNull(matrix[7]) ||
        !qFuzzyCompare(matrix[15], 1.0F) || qFuzzyIsNull(matrix[0]) || qFuzzyIsNull(matrix[5])) {
        return false;
    }
    const double scaleX = matrix[0];
    const double scaleY = matrix[5];
    const double ax = 1.0 / scaleX;
    const double bx = -matrix[12] / scaleX;
    const double ay = 1.0 / scaleY;
    const double by = 1.0 - ((1.0 - matrix[13]) / scaleY);

    // Whole pixels, outwards: SurfaceTexture's matrix already keeps half a texel or more off the crop's edges. The
    // margin absorbs rounding in the matrix.
    constexpr double kMargin = 1e-3;
    const QRect picture = geometry.picture;
    const auto span = [](double from, double to, int origin, int extent) {
        const int first = origin + static_cast<int>(std::floor((std::min(from, to) * extent) + kMargin));
        const int end = origin + static_cast<int>(std::ceil((std::max(from, to) * extent) - kMargin));
        return std::pair(std::clamp(first, origin, origin + extent), std::clamp(end, origin, origin + extent));
    };
    const auto [left, right] = span(bx, bx + ax, picture.x(), picture.width());
    const auto [top, bottom] = span(by, by + ay, picture.y(), picture.height());
    if ((right <= left) || (bottom <= top)) {
        return false;
    }
    geometry.picture = QRect(left, top, right - left, bottom - top);
    geometry.flipHorizontally = ax < 0.0;
    geometry.flipVertically = ay < 0.0;
    return true;
}

}  // namespace

FrameGeometry frameGeometry(GstBuffer* buffer, const GstVideoInfo& capsInfo)
{
    const QSize capsSize(GST_VIDEO_INFO_WIDTH(&capsInfo), GST_VIDEO_INFO_HEIGHT(&capsInfo));
    FrameGeometry geometry{.frameSize = capsSize, .picture = QRect(QPoint(0, 0), capsSize), .pictureSize = capsSize};
    // A meta smaller than the caps is no frame gst_video_frame_map() maps: left to fail there.
    if (const GstVideoMeta* const meta = gst_buffer_get_video_meta(buffer);
        meta && (meta->width >= static_cast<guint>(capsSize.width())) &&
        (meta->height >= static_cast<guint>(capsSize.height()))) {
        geometry.frameSize = QSize(static_cast<int>(meta->width), static_cast<int>(meta->height));
    }
    if (const GstVideoCropMeta* const crop = gst_buffer_get_video_crop_meta(buffer)) {
        geometry.picture = QRect(static_cast<int>(crop->x), static_cast<int>(crop->y), static_cast<int>(crop->width),
                                 static_cast<int>(crop->height));
        geometry.pictureSize = geometry.picture.size();
    }
    if (const GstVideoAffineTransformationMeta* const affine = gst_buffer_get_video_affine_transformation_meta(buffer);
        affine && !applyAffineMatrix(geometry, affine->matrix)) {
        static std::atomic<bool> s_warnedUnapplied{false};
        if (!s_warnedUnapplied.exchange(true, std::memory_order_relaxed)) {
            qCWarning(GstQgcFrameMapLog) << "GstVideoAffineTransformationMeta is no viewport with flips (a scale, a "
                                            "translation, mirroring) — not applied";
        }
    }
    return geometry;
}

void applyFlips(QVideoFrame& frame, const FrameGeometry& geometry)
{
    if (!geometry.flipHorizontally && !geometry.flipVertically) {
        return;
    }
    // Qt turns a frame clockwise, then mirrors it left to right. A left-to-right flip ahead of a turn is the opposite
    // turn, mirrored; a top-to-bottom flip is a left-to-right one after half a turn.
    int degrees = static_cast<int>(frame.rotation());
    bool mirrored = frame.mirrored();
    if (geometry.flipHorizontally != geometry.flipVertically) {
        degrees = (geometry.flipVertically ? 540 : 360) - degrees;
        mirrored = !mirrored;
    } else {
        degrees += 180;
    }
    frame.setRotation(static_cast<QtVideo::Rotation>(degrees % 360));
    frame.setMirrored(mirrored);
}

// QQuickVideoOutput computes sample rect as viewport/frameSize (QQuickVideoOutput::updateGeometry());
// externalTextureMatrix is only used for Format_SamplerExternalOES, so can't crop standard formats.
QVideoFrameFormat withFrameGeometry(QVideoFrameFormat format, const FrameGeometry& geometry)
{
    // Resets the viewport to the whole frame.
    format.setFrameSize(geometry.frameSize);
    format.setViewport(geometry.picture);
    return format;
}

QVideoFrame mapCpuFrame(GstBuffer* buffer, const GstVideoInfo& info, const QVideoFrameFormat& format)
{
    if (!buffer || !format.isValid()) {
        return {};
    }
    auto cpuBuffer = std::make_unique<MapThroughGstVideoBuffer>(buffer, info, format);
    // Mapped here, so a buffer no CPU can read is dropped on the streaming thread; Qt's map() reuses the mapping.
    if (cpuBuffer->map(QVideoFrame::ReadOnly).planeCount <= 0) {
        return {};
    }
    return QVideoFrame(std::move(cpuBuffer));
}

MappedFrame mapSampleToFrame(GstBuffer* buffer, GstCaps* caps, const GstVideoInfo& info,
                             const QVideoFrameFormat& format, [[maybe_unused]] const HwVideoBufferContext& hwContext,
                             [[maybe_unused]] HwResolvedPathCache* pathCache)
{
    MappedFrame out;
#if defined(QGC_HAS_ANY_GPU_PATH)
    HwVideoBufferPath matchedPath = HwVideoBufferPath::None;
    if (hwContext.gpuEnabled) {
        if (auto hwBuf = makeHwVideoBuffer(buffer, caps, info, format, hwContext, matchedPath, pathCache)) {
            out.frame = QVideoFrame(std::move(hwBuf));
            out.gpuPath = matchedPath;
            return out;
        }
        // HW selection failed though GPU was requested: signal the demotion. show_frame owns the
        // once-per-epoch latch and the counters.
        out.demoted = true;
    }
#endif
    if (!GstHw::hasCpuLayout(caps)) {
        out.noCpuLayout = true;
        return out;
    }
    out.frame = mapCpuFrame(buffer, info, format);
    return out;
}
