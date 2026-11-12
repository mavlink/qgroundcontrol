#include "GstQgcFrameMap.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <gst/gst.h>
#include <gst/video/gstvideoaffinetransformationmeta.h>
#include <gst/video/gstvideometa.h>
#include <gst/video/video-hdr.h>
#include <utility>

#include <QtCore/QtNumeric>
#include <QtMultimedia/QAbstractVideoBuffer>
#include <QtMultimedia/QVideoFrame>

#include "GstQgcVideoFormats.h"
#include "HwBuffers/common/CpuVideoFramePool.h"
#include "HwBuffers/common/GstHwVideoInfo.h"
#include "QGCLoggingCategory.h"

#if defined(QGC_HAS_ANY_GPU_PATH)
#include <private/qhwvideobuffer_p.h>
#endif

QGC_LOGGING_CATEGORY(GstQgcFrameMapLog, "Video.GStreamer.FrameMap")

QVideoFrameFormat::ColorSpace toQtColorSpace(GstVideoColorMatrix matrix)
{
    switch (matrix) {
        case GST_VIDEO_COLOR_MATRIX_BT601:
            return QVideoFrameFormat::ColorSpace_BT601;
        case GST_VIDEO_COLOR_MATRIX_BT709:
            return QVideoFrameFormat::ColorSpace_BT709;
        case GST_VIDEO_COLOR_MATRIX_BT2020:
            return QVideoFrameFormat::ColorSpace_BT2020;
        case GST_VIDEO_COLOR_MATRIX_SMPTE240M:
            // Matches Qt qgst.cpp convention (shared D65 white point), not full colorimetric equivalence.
            return QVideoFrameFormat::ColorSpace_AdobeRgb;
        // Qt groups FCC with UNKNOWN/RGB and leaves it Undefined (qgst.cpp); the
        // resolution heuristic below then picks BT601/BT709.
        default:
            return QVideoFrameFormat::ColorSpace_Undefined;
    }
}

QVideoFrameFormat::ColorTransfer toQtColorTransfer(GstVideoTransferFunction transfer)
{
    // Mirrors Qt's qgst.cpp QGstCaps::formatAndVideoInfo() (cross-checked Qt 6.12.0).
    switch (transfer) {
        case GST_VIDEO_TRANSFER_BT601:
            return QVideoFrameFormat::ColorTransfer_BT601;
        case GST_VIDEO_TRANSFER_BT2020_10:
        case GST_VIDEO_TRANSFER_BT2020_12:
        case GST_VIDEO_TRANSFER_BT709:
            return QVideoFrameFormat::ColorTransfer_BT709;
        case GST_VIDEO_TRANSFER_GAMMA20:
            return QVideoFrameFormat::ColorTransfer_BT709;  // best fit per Qt
        // SMPTE 240M uses a ~2.2 power curve, not BT.709 piecewise-linear; Qt groups it with Gamma22.
        case GST_VIDEO_TRANSFER_SMPTE240M:
        case GST_VIDEO_TRANSFER_GAMMA22:
        case GST_VIDEO_TRANSFER_SRGB:
        case GST_VIDEO_TRANSFER_ADOBERGB:
            return QVideoFrameFormat::ColorTransfer_Gamma22;
        case GST_VIDEO_TRANSFER_GAMMA18:
            return QVideoFrameFormat::ColorTransfer_BT709;  // matches Qt qgst.cpp GAMMA18 mapping
        case GST_VIDEO_TRANSFER_GAMMA28:
            return QVideoFrameFormat::ColorTransfer_Gamma28;
        case GST_VIDEO_TRANSFER_GAMMA10:
            return QVideoFrameFormat::ColorTransfer_Linear;
        case GST_VIDEO_TRANSFER_SMPTE2084:
            return QVideoFrameFormat::ColorTransfer_ST2084;
        case GST_VIDEO_TRANSFER_ARIB_STD_B67:
            return QVideoFrameFormat::ColorTransfer_STD_B67;
        // GST_VIDEO_TRANSFER_LOG100 / LOG316 have no Qt equivalent — leave as Unknown
        default:
            return QVideoFrameFormat::ColorTransfer_Unknown;
    }
}

QVideoFrameFormat::PixelFormat toQtPixelFormat(GstVideoFormat fmt)
{
    for (const auto& e : GstQgc::kVideoFormatTable) {
        if (e.gst == fmt) {
            return e.qt;
        }
    }
    return QVideoFrameFormat::Format_Invalid;
}

QVideoFrameFormat::ColorRange toQtColorRange(GstVideoColorRange range)
{
    switch (range) {
        case GST_VIDEO_COLOR_RANGE_0_255:
            return QVideoFrameFormat::ColorRange_Full;
        case GST_VIDEO_COLOR_RANGE_16_235:
            return QVideoFrameFormat::ColorRange_Video;
        default:
            return QVideoFrameFormat::ColorRange_Unknown;
    }
}

void applyOrientationToFrame(QVideoFrame& frame, GstVideoOrientationMethod method)
{
    switch (method) {
        case GST_VIDEO_ORIENTATION_IDENTITY:
            frame.setRotation(QtVideo::Rotation::None);
            frame.setMirrored(false);
            break;
        case GST_VIDEO_ORIENTATION_90R:
            frame.setRotation(QtVideo::Rotation::Clockwise90);
            frame.setMirrored(false);
            break;
        case GST_VIDEO_ORIENTATION_180:
            frame.setRotation(QtVideo::Rotation::Clockwise180);
            frame.setMirrored(false);
            break;
        case GST_VIDEO_ORIENTATION_90L:
            frame.setRotation(QtVideo::Rotation::Clockwise270);
            frame.setMirrored(false);
            break;
        case GST_VIDEO_ORIENTATION_HORIZ:
            frame.setRotation(QtVideo::Rotation::None);
            frame.setMirrored(true);
            break;
        case GST_VIDEO_ORIENTATION_VERT:
            frame.setRotation(QtVideo::Rotation::Clockwise180);
            frame.setMirrored(true);
            break;
        case GST_VIDEO_ORIENTATION_UL_LR:
            frame.setRotation(QtVideo::Rotation::Clockwise90);
            frame.setMirrored(true);
            break;
        case GST_VIDEO_ORIENTATION_UR_LL:
            frame.setRotation(QtVideo::Rotation::Clockwise270);
            frame.setMirrored(true);
            break;
        default:
            static std::atomic<bool> s_warnedUnhandled{false};
            if (!s_warnedUnhandled.exchange(true, std::memory_order_relaxed)) {
                qCWarning(GstQgcFrameMapLog)
                    << "Unhandled GstVideoOrientationMethod" << method << "— treating as identity";
            }
            frame.setRotation(QtVideo::Rotation::None);
            frame.setMirrored(false);
            break;
    }
}

void applyOrientationAndTiming(QVideoFrame& frame, GstBuffer* buffer, int streamOrientation)
{
    if (streamOrientation != static_cast<int>(GST_VIDEO_ORIENTATION_IDENTITY)) {
        applyOrientationToFrame(frame, static_cast<GstVideoOrientationMethod>(streamOrientation));
    }
    if (GST_BUFFER_PTS_IS_VALID(buffer)) {
        // GstClockTime is ns; QVideoFrame timestamps are µs.
        frame.setStartTime(GST_BUFFER_PTS(buffer) / GST_USECOND);
        if (GST_BUFFER_DURATION_IS_VALID(buffer)) {
            frame.setEndTime((GST_BUFFER_PTS(buffer) + GST_BUFFER_DURATION(buffer)) / GST_USECOND);
        }
    }
}

void applyColorimetry(QVideoFrameFormat& format, const GstVideoInfo& info, GstCaps* caps)
{
    const GstVideoColorimetry& colorimetry = GST_VIDEO_INFO_COLORIMETRY(&info);
    QVideoFrameFormat::ColorSpace colorSpace = toQtColorSpace(colorimetry.matrix);
    // Live RTSP sources often omit colorimetry caps; match Qt's renderer fallback
    // (qvideotexturehelper.cpp): height > 576 is HD/BT.709, otherwise SD/BT.601.
    if (colorSpace == QVideoFrameFormat::ColorSpace_Undefined) {
        const int height = GST_VIDEO_INFO_HEIGHT(&info);
        if (height > 0) {
            colorSpace = (height > 576) ? QVideoFrameFormat::ColorSpace_BT709 : QVideoFrameFormat::ColorSpace_BT601;
        }
    }
    format.setColorSpace(colorSpace);
    format.setColorTransfer(toQtColorTransfer(colorimetry.transfer));
    QVideoFrameFormat::ColorRange range = toQtColorRange(colorimetry.range);
    // H.264/H.265 omit VUI range but encode limited per spec — else Qt skips its limited->full offset.
    // Only infer for a known YUV matrix: an UNKNOWN matrix tells us nothing, and RGB is always full-range.
    const bool knownYuvMatrix =
        (colorimetry.matrix == GST_VIDEO_COLOR_MATRIX_BT601) || (colorimetry.matrix == GST_VIDEO_COLOR_MATRIX_BT709) ||
        (colorimetry.matrix == GST_VIDEO_COLOR_MATRIX_BT2020) ||
        (colorimetry.matrix == GST_VIDEO_COLOR_MATRIX_SMPTE240M) || (colorimetry.matrix == GST_VIDEO_COLOR_MATRIX_FCC);
    if (range == QVideoFrameFormat::ColorRange_Unknown && knownYuvMatrix) {
        range = QVideoFrameFormat::ColorRange_Video;
    }
    format.setColorRange(range);

    // Prefer MaxCLL (tighter tone-mapping target) over mastering-display max-luminance.
    GstVideoContentLightLevel cll;
    bool clipApplied = false;
    if (caps && gst_video_content_light_level_from_caps(&cll, caps) && cll.max_content_light_level > 0) {
        format.setMaxLuminance(static_cast<float>(cll.max_content_light_level));
        clipApplied = true;
    }
    if (!clipApplied) {
        GstVideoMasteringDisplayInfo masteringInfo;
        if (caps && gst_video_mastering_display_info_from_caps(&masteringInfo, caps)) {
            // GstVideoMasteringDisplayColorVolume max_luma is in 0.0001 cd/m².
            const double maxLuminance = static_cast<double>(masteringInfo.max_display_mastering_luminance) / 10000.0;
            if (maxLuminance > 0.0) {
                format.setMaxLuminance(static_cast<float>(maxLuminance));
            }
        }
    }
}

namespace {

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

MappedFrame mapSampleToFrame(GstBuffer* buffer, GstCaps* caps, const GstVideoInfo& info,
                             const QVideoFrameFormat& format, [[maybe_unused]] const HwVideoBufferContext& hwContext,
                             const CpuVideoFramePool::PinnedFrameCounter& pinnedCpuFrames,
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
    if (auto cpuBuf = CpuVideoFramePool::wrapZeroCopy(buffer, info, format, pinnedCpuFrames)) {
        out.frame = QVideoFrame(std::move(cpuBuf));
    } else {
        out.frame = CpuVideoFramePool::copyFromBuffer(buffer, info, format);
    }
    return out;
}
