#include "GstQgcVideoFormats.h"

#include <algorithm>
#include <atomic>
#include <gst/video/video-hdr.h>

#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(GstQgcFrameMapLog)

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
    struct Orientation
    {
        GstVideoOrientationMethod method;
        QtVideo::Rotation rotation;
        bool mirrored;
    };

    static constexpr Orientation kOrientations[] = {
        {GST_VIDEO_ORIENTATION_IDENTITY, QtVideo::Rotation::None, false},
        {GST_VIDEO_ORIENTATION_90R, QtVideo::Rotation::Clockwise90, false},
        {GST_VIDEO_ORIENTATION_180, QtVideo::Rotation::Clockwise180, false},
        {GST_VIDEO_ORIENTATION_90L, QtVideo::Rotation::Clockwise270, false},
        {GST_VIDEO_ORIENTATION_HORIZ, QtVideo::Rotation::None, true},
        {GST_VIDEO_ORIENTATION_VERT, QtVideo::Rotation::Clockwise180, true},
        {GST_VIDEO_ORIENTATION_UL_LR, QtVideo::Rotation::Clockwise90, true},
        {GST_VIDEO_ORIENTATION_UR_LL, QtVideo::Rotation::Clockwise270, true},
    };
    const Orientation* known = std::ranges::find(kOrientations, method, &Orientation::method);
    if (known == std::end(kOrientations)) {
        static std::atomic<bool> s_warnedUnhandled{false};
        if (!s_warnedUnhandled.exchange(true, std::memory_order_relaxed)) {
            qCWarning(GstQgcFrameMapLog) << "Unhandled GstVideoOrientationMethod" << method << "— treating as identity";
        }
        known = std::begin(kOrientations);
    }
    frame.setRotation(known->rotation);
    frame.setMirrored(known->mirrored);
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
