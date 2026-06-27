#pragma once

#include <QtMultimedia/QVideoFrameFormat>
#include <gst/video/video-format.h>
#include <string>

namespace GstQgc {

/// Single source of truth linking a GStreamer video format to its Qt pixel format and (optionally) its
/// negotiation caps token. Both the advertised caps string (GstQgcCaps) and toQtPixelFormat()
/// (GStreamerFrameMap) derive from this table, so a format can never be advertised without a Qt mapping,
/// nor mapped-and-renderable without being offered.
struct VideoFormatEntry
{
    GstVideoFormat gst;
    QVideoFrameFormat::PixelFormat qt;
    const char* capsToken;  ///< non-null: advertised in negotiation caps; null: accepted-only (defensive map).
};

inline constexpr VideoFormatEntry kVideoFormatTable[] = {
    // Advertised set — order is preserved in the generated caps string (negotiation preference order).
    {GST_VIDEO_FORMAT_NV12, QVideoFrameFormat::Format_NV12, "NV12"},
    {GST_VIDEO_FORMAT_NV21, QVideoFrameFormat::Format_NV21, "NV21"},
    {GST_VIDEO_FORMAT_I420, QVideoFrameFormat::Format_YUV420P, "I420"},
    {GST_VIDEO_FORMAT_YV12, QVideoFrameFormat::Format_YV12, "YV12"},
    {GST_VIDEO_FORMAT_Y42B, QVideoFrameFormat::Format_YUV422P, "Y42B"},
    {GST_VIDEO_FORMAT_P010_10LE, QVideoFrameFormat::Format_P010, "P010_10LE"},
    {GST_VIDEO_FORMAT_AYUV, QVideoFrameFormat::Format_AYUV, "AYUV"},
    {GST_VIDEO_FORMAT_YUY2, QVideoFrameFormat::Format_YUYV, "YUY2"},
    {GST_VIDEO_FORMAT_UYVY, QVideoFrameFormat::Format_UYVY, "UYVY"},
    {GST_VIDEO_FORMAT_GRAY8, QVideoFrameFormat::Format_Y8, "GRAY8"},
    {GST_VIDEO_FORMAT_GRAY16_LE, QVideoFrameFormat::Format_Y16, "GRAY16_LE"},
    {GST_VIDEO_FORMAT_BGRA, QVideoFrameFormat::Format_BGRA8888, "BGRA"},
    {GST_VIDEO_FORMAT_RGBA, QVideoFrameFormat::Format_RGBA8888, "RGBA"},
    // Accepted but not advertised: mapped defensively if upstream or a GPU path delivers them anyway.
    {GST_VIDEO_FORMAT_BGRx, QVideoFrameFormat::Format_BGRX8888, nullptr},
    {GST_VIDEO_FORMAT_RGBx, QVideoFrameFormat::Format_RGBX8888, nullptr},
    {GST_VIDEO_FORMAT_ARGB, QVideoFrameFormat::Format_ARGB8888, nullptr},
    {GST_VIDEO_FORMAT_xRGB, QVideoFrameFormat::Format_XRGB8888, nullptr},
    {GST_VIDEO_FORMAT_I420_10LE, QVideoFrameFormat::Format_YUV420P10, nullptr},
    {GST_VIDEO_FORMAT_P016_LE, QVideoFrameFormat::Format_P016, nullptr},
    // GST_VIDEO_FORMAT_BGR/RGB intentionally absent: Qt6 has no 24-bit packed format, so they resolve to
    // Format_Invalid (the lookup default) rather than corrupting stride arithmetic.
};

/// "{ NV12, NV21, ... }" — the advertised format list for a caps string. Built once at caps setup.
inline std::string advertisedFormatList()
{
    std::string out = "{ ";
    bool first = true;
    for (const auto& e : kVideoFormatTable) {
        if (!e.capsToken) {
            continue;
        }
        if (!first) {
            out += ", ";
        }
        out += e.capsToken;
        first = false;
    }
    out += " }";
    return out;
}

}  // namespace GstQgc
