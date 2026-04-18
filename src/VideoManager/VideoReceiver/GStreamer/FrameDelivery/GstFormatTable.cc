#include "GstFormatTable.h"

#include <QtCore/QByteArray>
#include <QtCore/QSysInfo>
#include <gst/video/video-hdr.h>
#include <gst/video/video-info.h>

#ifdef QGC_GST_DMABUF
#include <drm_fourcc.h>
#endif

// ═════════════════════════════════════════════════════════════════════
// Format table — THE single source of truth
// ═════════════════════════════════════════════════════════════════════
//
// Columns: GstVideoFormat, QVideoFrameFormat::PixelFormat,
//          supported on CPU, DMA-BUF, D3D11, AndroidHwBuf
//
// To add a format, add one row here. All consumers (caps builders,
// format converters, DRM fourcc lookup) pick it up automatically.

namespace {

struct FormatEntry
{
    GstVideoFormat gst;
    QVideoFrameFormat::PixelFormat qt;
    bool cpu;      // advertise in CPU caps
    bool dmaBuf;   // advertise in DMA-BUF caps
    bool d3d11;    // advertise in D3D11 caps
    bool android;  // advertise in Android HWB caps
    bool gl;       // advertise in GLMemory caps
    bool va;       // advertise in VASurface caps
};

// clang-format off
static constexpr FormatEntry kFormatTable[] = {
    //                                                                   cpu    dma    d3d    android gl     va
    // ── YUV planar ──────────────────────────────────────────────────
    { GST_VIDEO_FORMAT_NV12,       QVideoFrameFormat::Format_NV12,      true,  true,  true,  true,  true,  true  },
    { GST_VIDEO_FORMAT_NV21,       QVideoFrameFormat::Format_NV21,      true,  true,  false, true,  false, false },
    { GST_VIDEO_FORMAT_I420,       QVideoFrameFormat::Format_YUV420P,   true,  true,  false, true,  true,  true  },
    // YV12 plane order (Y-V-U) differs from Qt's Format_YUV420P (Y-U-V). Keep off CPU
    // caps and let videoconvert coerce to I420; DMA-BUF uses its own YVU420 fourcc.
    { GST_VIDEO_FORMAT_YV12,       QVideoFrameFormat::Format_YUV420P,   false, true,  false, false, false, false },
    { GST_VIDEO_FORMAT_Y42B,       QVideoFrameFormat::Format_YUV422P,   true,  false, false, false, false, false },

    // ── YUV packed ──────────────────────────────────────────────────
    { GST_VIDEO_FORMAT_UYVY,       QVideoFrameFormat::Format_UYVY,      true,  true,  false, false, false, false },
    { GST_VIDEO_FORMAT_YUY2,       QVideoFrameFormat::Format_YUYV,      true,  true,  false, false, false, false },

    // ── 10-bit / HDR ────────────────────────────────────────────────
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
    { GST_VIDEO_FORMAT_P010_10LE,  QVideoFrameFormat::Format_P010,      true,  true,  true,  false, false, true  },
#else
    { GST_VIDEO_FORMAT_P010_10BE,  QVideoFrameFormat::Format_P010,      true,  true,  true,  false, false, true  },
#endif

    // ── Grayscale (thermal cameras) ─────────────────────────────────
    { GST_VIDEO_FORMAT_GRAY8,      QVideoFrameFormat::Format_Y8,        true,  true,  false, false, false, false },
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
    { GST_VIDEO_FORMAT_GRAY16_LE,  QVideoFrameFormat::Format_Y16,       true,  false, false, false, false, false },
#else
    { GST_VIDEO_FORMAT_GRAY16_BE,  QVideoFrameFormat::Format_Y16,       true,  false, false, false, false, false },
#endif

    // ── RGBA family (with alpha) ────────────────────────────────────
    { GST_VIDEO_FORMAT_BGRA,       QVideoFrameFormat::Format_BGRA8888,  true,  true,  true,  true,  true,  true  },
    { GST_VIDEO_FORMAT_RGBA,       QVideoFrameFormat::Format_RGBA8888,  true,  true,  true,  true,  true,  true  },
    { GST_VIDEO_FORMAT_ARGB,       QVideoFrameFormat::Format_ARGB8888,  true,  true,  false, false, false, false },
    { GST_VIDEO_FORMAT_ABGR,       QVideoFrameFormat::Format_ABGR8888,  true,  false, false, false, false, false },

    // ── RGBx family (padded, no alpha) ──────────────────────────────
    { GST_VIDEO_FORMAT_BGRx,       QVideoFrameFormat::Format_BGRX8888,  true,  true,  true,  false, true,  true  },
    { GST_VIDEO_FORMAT_RGBx,       QVideoFrameFormat::Format_RGBX8888,  true,  false, false, false, false, false },
    { GST_VIDEO_FORMAT_xRGB,       QVideoFrameFormat::Format_XRGB8888,  true,  true,  false, false, false, false },
    { GST_VIDEO_FORMAT_xBGR,       QVideoFrameFormat::Format_XBGR8888,  true,  false, false, false, false, false },
};
// clang-format on

static constexpr int kTableSize = sizeof(kFormatTable) / sizeof(kFormatTable[0]);

/// Build a comma-separated format list from a predicate.
template <typename Pred>
QByteArray buildFormatList(Pred pred)
{
    QByteArray list;
    for (int i = 0; i < kTableSize; ++i) {
        if (!pred(kFormatTable[i]))
            continue;
        if (!list.isEmpty())
            list += ',';
        list += gst_video_format_to_string(kFormatTable[i].gst);
    }
    return list;
}

}  // anonymous namespace

// ═════════════════════════════════════════════════════════════════════
// Format conversion
// ═════════════════════════════════════════════════════════════════════

QVideoFrameFormat::PixelFormat GstFormatTable::gstFormatToQt(GstVideoFormat gstFmt)
{
    for (int i = 0; i < kTableSize; ++i) {
        if (kFormatTable[i].gst == gstFmt)
            return kFormatTable[i].qt;
    }
    return QVideoFrameFormat::Format_Invalid;
}

GstVideoFormat GstFormatTable::qtFormatToGst(QVideoFrameFormat::PixelFormat qtFmt)
{
    for (int i = 0; i < kTableSize; ++i) {
        if (kFormatTable[i].qt == qtFmt)
            return kFormatTable[i].gst;
    }
    return GST_VIDEO_FORMAT_UNKNOWN;
}

// ═════════════════════════════════════════════════════════════════════
// Colorimetry conversion
// ═════════════════════════════════════════════════════════════════════

namespace {

QVideoFrameFormat::ColorSpace gstColorSpaceToQt(GstVideoColorMatrix matrix)
{
    switch (matrix) {
        case GST_VIDEO_COLOR_MATRIX_BT709:
            return QVideoFrameFormat::ColorSpace_BT709;
        case GST_VIDEO_COLOR_MATRIX_BT601:
            return QVideoFrameFormat::ColorSpace_BT601;
        case GST_VIDEO_COLOR_MATRIX_BT2020:
            return QVideoFrameFormat::ColorSpace_BT2020;
        case GST_VIDEO_COLOR_MATRIX_RGB:
            // No YUV matrix — returning Undefined skips Qt's YCbCr→RGB conversion.
            return QVideoFrameFormat::ColorSpace_Undefined;
        default:
            return QVideoFrameFormat::ColorSpace_Undefined;
    }
}

QVideoFrameFormat::ColorTransfer gstTransferToQt(GstVideoTransferFunction transfer)
{
    switch (transfer) {
        case GST_VIDEO_TRANSFER_BT709:
            return QVideoFrameFormat::ColorTransfer_BT709;
        case GST_VIDEO_TRANSFER_BT601:
            return QVideoFrameFormat::ColorTransfer_BT601;
        case GST_VIDEO_TRANSFER_GAMMA22:
            return QVideoFrameFormat::ColorTransfer_Gamma22;
        case GST_VIDEO_TRANSFER_GAMMA28:
            return QVideoFrameFormat::ColorTransfer_Gamma28;
        case GST_VIDEO_TRANSFER_SMPTE2084:
            return QVideoFrameFormat::ColorTransfer_ST2084;
        case GST_VIDEO_TRANSFER_ARIB_STD_B67:
            return QVideoFrameFormat::ColorTransfer_STD_B67;
        case GST_VIDEO_TRANSFER_GAMMA10:
            return QVideoFrameFormat::ColorTransfer_Linear;
        default:
            return QVideoFrameFormat::ColorTransfer_Unknown;
    }
}

QVideoFrameFormat::ColorRange gstRangeToQt(GstVideoColorRange range)
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

}  // anonymous namespace

void GstFormatTable::applyColorimetry(QVideoFrameFormat& format, const GstVideoInfo& videoInfo)
{
    const GstVideoColorimetry& c = videoInfo.colorimetry;
    format.setColorSpace(gstColorSpaceToQt(c.matrix));
    format.setColorTransfer(gstTransferToQt(c.transfer));
    format.setColorRange(gstRangeToQt(c.range));
}

void GstFormatTable::applyHdrMetadata(QVideoFrameFormat& format, const GstCaps* caps)
{
    if (!caps)
        return;

    // Mastering display info (max luminance in 1/10000 cd/m²)
    GstVideoMasteringDisplayInfo mdi;
    gst_video_mastering_display_info_init(&mdi);
    if (gst_video_mastering_display_info_from_caps(&mdi, caps)) {
        const float maxLum = static_cast<float>(mdi.max_display_mastering_luminance) / 10000.0F;
        if (maxLum > 0.0F) {
            format.setMaxLuminance(maxLum);
        }
    }

    // Content light level (max CLL in cd/m²)
    GstVideoContentLightLevel cll;
    gst_video_content_light_level_init(&cll);
    if (gst_video_content_light_level_from_caps(&cll, caps)) {
        // Qt only exposes maxLuminance; use max CLL if higher than mastering max
        const float maxCll = static_cast<float>(cll.max_content_light_level);
        if (maxCll > format.maxLuminance()) {
            format.setMaxLuminance(maxCll);
        }
    }
}

bool GstFormatTable::isHdrContent(const GstVideoInfo& videoInfo)
{
    const GstVideoTransferFunction tf = videoInfo.colorimetry.transfer;
    return tf == GST_VIDEO_TRANSFER_SMPTE2084         // PQ (HDR10)
           || tf == GST_VIDEO_TRANSFER_ARIB_STD_B67;  // HLG
}

// ═════════════════════════════════════════════════════════════════════
// Caps string builders
// ═════════════════════════════════════════════════════════════════════

QByteArray GstFormatTable::cpuCapsFormats()
{
    QByteArray fmts = buildFormatList([](const FormatEntry& e) { return e.cpu; });
    return "video/x-raw,format={" + fmts + "}";
}

QByteArray GstFormatTable::dmaBufCapsFormats()
{
    QByteArray fmts = buildFormatList([](const FormatEntry& e) { return e.dmaBuf; });
    return "video/x-raw(memory:DMABuf),format={" + fmts + "}";
}

QByteArray GstFormatTable::dmaBufDrmCapsFormats()
{
    // GStreamer's DRM caps use the pixel format string suffixed by ":<modifier>".
    // Hardcoded to :0 (DRM_FORMAT_MOD_LINEAR) so only linear buffers negotiate;
    // see GstFormatTable.h for rationale.
    QByteArray fmts = buildFormatList([](const FormatEntry& e) {
        return e.dmaBuf;  // same format set as legacy DMA-BUF path
    });
    // Suffix each comma-separated format with :0 (linear modifier).
    QByteArray drmList;
    for (const QByteArray& fmt : fmts.split(',')) {
        if (!drmList.isEmpty())
            drmList += ',';
        drmList += fmt + ":0";
    }
    return "video/x-raw(memory:DMABuf),format=DMA_DRM,drm-format={" + drmList + "}";
}

QByteArray GstFormatTable::d3d11CapsFormats()
{
    QByteArray fmts = buildFormatList([](const FormatEntry& e) { return e.d3d11; });
    return "video/x-raw(memory:D3D11Memory),format={" + fmts + "}";
}

QByteArray GstFormatTable::androidHwBufCapsFormats()
{
    QByteArray fmts = buildFormatList([](const FormatEntry& e) { return e.android; });
    return "video/x-raw(memory:AndroidHardwareBuffer),format={" + fmts + "}";
}

QByteArray GstFormatTable::glMemoryCapsFormats()
{
    QByteArray fmts = buildFormatList([](const FormatEntry& e) { return e.gl; });
    return "video/x-raw(memory:GLMemory),format={" + fmts + "}";
}

QByteArray GstFormatTable::vaSurfaceCapsFormats()
{
    QByteArray fmts = buildFormatList([](const FormatEntry& e) { return e.va; });
    return "video/x-raw(memory:VASurface),format={" + fmts + "}";
}

// ═════════════════════════════════════════════════════════════════════
// DRM fourcc mapping
// ═════════════════════════════════════════════════════════════════════

int GstFormatTable::drmFourccForFormat(GstVideoFormat gstFmt, int plane)
{
#ifdef QGC_GST_DMABUF

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
    constexpr int kArgb = DRM_FORMAT_ARGB8888;
    constexpr int kRgba = DRM_FORMAT_ABGR8888;  // RGBA in little-endian memory
    constexpr int kRg = DRM_FORMAT_GR88;
    constexpr int kBgrx = DRM_FORMAT_XRGB8888;  // BGRx as stored == XRGB in DRM
#else
    constexpr int kArgb = DRM_FORMAT_BGRA8888;
    constexpr int kRgba = DRM_FORMAT_RGBA8888;
    constexpr int kRg = DRM_FORMAT_RG88;
    constexpr int kBgrx = DRM_FORMAT_BGRX8888;
#endif

    switch (gstFmt) {
        // RGBA/BGRA with alpha
        case GST_VIDEO_FORMAT_BGRA:
        case GST_VIDEO_FORMAT_BGRx:
            return kBgrx;
        case GST_VIDEO_FORMAT_RGBA:
        case GST_VIDEO_FORMAT_RGBx:
            return kRgba;
        case GST_VIDEO_FORMAT_ARGB:
        case GST_VIDEO_FORMAT_xRGB:
            return kArgb;
        case GST_VIDEO_FORMAT_ABGR:
        case GST_VIDEO_FORMAT_xBGR:
            return kRgba;  // ABGR matches RGBA fourcc in swapped endian

        // Semi-planar YUV
        case GST_VIDEO_FORMAT_NV12:
        case GST_VIDEO_FORMAT_NV21:
            return (plane == 0) ? DRM_FORMAT_R8 : kRg;

        // Planar YUV
        case GST_VIDEO_FORMAT_I420:
        case GST_VIDEO_FORMAT_YV12:
            return DRM_FORMAT_R8;

        // Packed YUV
        case GST_VIDEO_FORMAT_YUY2:
            return DRM_FORMAT_YUYV;
        case GST_VIDEO_FORMAT_UYVY:
            return DRM_FORMAT_UYVY;

            // 10-bit
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
        case GST_VIDEO_FORMAT_P010_10LE:
#else
        case GST_VIDEO_FORMAT_P010_10BE:
#endif
            return (plane == 0) ? DRM_FORMAT_R16 : DRM_FORMAT_GR1616;

        // Grayscale
        case GST_VIDEO_FORMAT_GRAY8:
            return DRM_FORMAT_R8;

        default:
            break;
    }
#else
    Q_UNUSED(gstFmt)
    Q_UNUSED(plane)
#endif  // QGC_GST_DMABUF

    return -1;
}

int GstFormatTable::drmFourccForWholeFrame(GstVideoFormat gstFmt)
{
#ifdef QGC_GST_DMABUF

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
    constexpr int kArgb = DRM_FORMAT_ARGB8888;
    constexpr int kRgba = DRM_FORMAT_ABGR8888;
#else
    constexpr int kArgb = DRM_FORMAT_BGRA8888;
    constexpr int kRgba = DRM_FORMAT_RGBA8888;
#endif

    switch (gstFmt) {
        case GST_VIDEO_FORMAT_BGRA:
        case GST_VIDEO_FORMAT_BGRx:
            return kArgb;
        case GST_VIDEO_FORMAT_RGBA:
        case GST_VIDEO_FORMAT_RGBx:
            return kRgba;
        case GST_VIDEO_FORMAT_NV12:
            return DRM_FORMAT_NV12;
        case GST_VIDEO_FORMAT_NV21:
            return DRM_FORMAT_NV21;
        case GST_VIDEO_FORMAT_I420:
            return DRM_FORMAT_YUV420;
        case GST_VIDEO_FORMAT_YV12:
            return DRM_FORMAT_YVU420;
        case GST_VIDEO_FORMAT_YUY2:
            return DRM_FORMAT_YUYV;
        case GST_VIDEO_FORMAT_UYVY:
            return DRM_FORMAT_UYVY;
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
        case GST_VIDEO_FORMAT_P010_10LE:
#else
        case GST_VIDEO_FORMAT_P010_10BE:
#endif
            return DRM_FORMAT_P010;
        default:
            break;
    }
#else
    Q_UNUSED(gstFmt)
#endif  // QGC_GST_DMABUF

    return -1;
}
