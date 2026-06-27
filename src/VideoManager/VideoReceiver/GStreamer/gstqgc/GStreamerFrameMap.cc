#include "GStreamerFrameMap.h"

#include <QtMultimedia/QAbstractVideoBuffer>
#include <QtMultimedia/QVideoFrame>
#include <atomic>
#include <gst/gst.h>
#include <gst/video/gstvideometa.h>
#include <gst/video/video-hdr.h>

#include "GstQgcVideoFormats.h"
#include "HwBuffers/common/CpuVideoFramePool.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GStreamerFrameMapLog, "Video.GStreamer.FrameMap")

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
    // Mirrors Qt's qgst.cpp QGstCaps::formatAndVideoInfo() (cross-checked Qt 6.10.3).
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

// Operates on the GstVideoOrientationMethod enum, which is in <gst/video/video.h>'s
// always-present subset — independent of QGC_HAS_GST_VIDEO_ORIENTATION_META.
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
                qCWarning(GStreamerFrameMapLog)
                    << "Unhandled GstVideoOrientationMethod" << method << "— treating as identity";
            }
            frame.setRotation(QtVideo::Rotation::None);
            frame.setMirrored(false);
            break;
    }
}

void applyOrientationAndTiming(QVideoFrame& frame, [[maybe_unused]] GstBuffer* buffer, int streamOrientation)
{
    // Per-buffer meta wins (per-frame override) when gst-video exports it; stream-level fallback
    // works on every install.
#ifdef QGC_HAS_GST_VIDEO_ORIENTATION_META
    if (GstVideoOrientationMeta* meta = gst_buffer_get_video_orientation_meta(buffer)) {
        applyOrientationToFrame(frame, meta->orientation);
    } else
#endif
        if (streamOrientation != static_cast<int>(GST_VIDEO_ORIENTATION_IDENTITY)) {
        applyOrientationToFrame(frame, static_cast<GstVideoOrientationMethod>(streamOrientation));
    }
    if (GST_BUFFER_PTS_IS_VALID(buffer)) {
        // GstClockTime is ns; QVideoFrame timestamps are µs.
        frame.setStartTime(GST_BUFFER_PTS(buffer) / GST_USECOND);
        if (GST_BUFFER_DURATION_IS_VALID(buffer)) {
            frame.setEndTime((GST_BUFFER_PTS(buffer) + GST_BUFFER_DURATION(buffer)) / GST_USECOND);
        } else {
            // No duration: collapse the interval so consumers never see a stale/zero endTime.
            frame.setEndTime(GST_BUFFER_PTS(buffer) / GST_USECOND);
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

// QQuickVideoOutput computes sample rect as viewport/frameSize (qquickvideooutput.cpp:498);
// externalTextureMatrix is only used for Format_SamplerExternalOES, so can't crop standard formats.
QVideoFrameFormat applyCropMeta(QVideoFrameFormat format, GstBuffer* buffer)
{
    if (GstVideoCropMeta* crop = gst_buffer_get_video_crop_meta(buffer)) {
        format.setViewport(QRect(crop->x, crop->y, crop->width, crop->height));
    }
    return format;
}

MappedFrame mapSampleToFrame(GstBuffer* buffer, [[maybe_unused]] GstCaps* caps, const GstVideoInfo& info,
                             const QVideoFrameFormat& format, [[maybe_unused]] const HwVideoBufferContext& hwContext,
                             [[maybe_unused]] HwResolvedPathCache* pathCache) noexcept
{
    MappedFrame out;
#if defined(QGC_HAS_ANY_GPU_PATH)
    HwVideoBufferPath matchedPath = HwVideoBufferPath::None;
    if (hwContext.gpuEnabled) {
        // GPU-only: GstHwVideoBuffer holds the sample for the frame's lifetime; makeHwVideoBuffer
        // takes its own ref, so drop ours immediately after.
        if (GstSample* sample = gst_sample_new(buffer, caps, nullptr, nullptr)) {
            auto hwBuf = makeHwVideoBuffer(sample, info, format, hwContext, matchedPath, pathCache);
            gst_sample_unref(sample);
            if (hwBuf) {
                out.frame = QVideoFrame(std::move(hwBuf));
                out.source = MappedFrame::Source::Gpu;
                out.gpuPath = matchedPath;
                return out;
            }
        }
        // HW selection failed though GPU was requested: signal the demotion. show_frame owns the
        // once-per-epoch latch and telemetry so this mapper stays free of the telemetry singleton.
        out.demoted = true;
    }
    out.gpuPath = matchedPath;
#endif
    if (auto cpuBuf = CpuVideoFramePool::wrapZeroCopy(buffer, info, format)) {
        out.frame = QVideoFrame(std::move(cpuBuf));
    } else {
        out.frame = CpuVideoFramePool::copyFromBuffer(buffer, info, format);
    }
    out.source = MappedFrame::Source::Cpu;
    return out;
}
