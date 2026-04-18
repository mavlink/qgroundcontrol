#pragma once

#include <QtCore/QByteArray>
#include <QtMultimedia/QVideoFrameFormat>
#include <gst/gstcaps.h>
#include <gst/video/video-format.h>
#include <gst/video/video-info.h>

/// Single source of truth for GStreamer ↔ Qt video format mappings.
///
/// Used by GstAppsinkBridge (caps negotiation) and GstCpuVideoBuffer
/// (format conversion). Adding a format here automatically propagates
/// to all consumers.
namespace GstFormatTable {

// ─── Format conversion ──────────────────────────────────────────────

/// Map a GStreamer video format to the corresponding Qt pixel format.
/// Returns Format_Invalid for unsupported formats.
QVideoFrameFormat::PixelFormat gstFormatToQt(GstVideoFormat gstFmt);

/// Map a Qt pixel format to the corresponding GStreamer video format.
/// Returns GST_VIDEO_FORMAT_UNKNOWN for unsupported formats.
GstVideoFormat qtFormatToGst(QVideoFrameFormat::PixelFormat qtFmt);

// ─── Colorimetry conversion ─────────────────────────────────────────

/// Map GStreamer colorimetry fields to Qt equivalents and apply to @p format.
void applyColorimetry(QVideoFrameFormat& format, const GstVideoInfo& videoInfo);

/// Extract HDR mastering display info and content light level from @p caps
/// and apply to @p format (max luminance). No-op if caps lack HDR metadata.
void applyHdrMetadata(QVideoFrameFormat& format, const GstCaps* caps);

/// Returns true if the video info indicates HDR content (PQ or HLG transfer).
bool isHdrContent(const GstVideoInfo& videoInfo);

// ─── Caps string builders ───────────────────────────────────────────
// Each returns a semicolon-separated GStreamer caps string fragment.

/// CPU memory formats: "video/x-raw,format={NV12,I420,...}"
QByteArray cpuCapsFormats();

/// DMA-BUF memory formats (legacy caps form): "video/x-raw(memory:DMABuf),format={...}"
///
/// Many V4L2 / gst-omx decoders still emit this legacy caps form — keep it for
/// compat. Modern decoders (GStreamer ≥1.24 vah264dec, v4l2h264dec on
/// recent kernels) instead emit the DMA_DRM variant below.
QByteArray dmaBufCapsFormats();

/// DMA-BUF + DRM modifier caps form (GStreamer 1.24+):
///     "video/x-raw(memory:DMABuf),format=DMA_DRM,drm-format={NV12:0,...}"
///
/// Modifier restricted to 0x0 (DRM_FORMAT_MOD_LINEAR) because the CPU-mmap
/// path in GstDmaBufVideoBuffer can't untile GPU-tiled buffers. Decoders
/// that can only emit tiled DMA-BUF (e.g. Intel VA-API's Y-tiled output)
/// will refuse negotiation against these caps and the pipeline drops to
/// the legacy DMABuf form or CPU caps — same behaviour as before this
/// caps addition. True GPU zero-copy for tiled modifiers requires EGL
/// import (Level 2) which is out of scope here.
QByteArray dmaBufDrmCapsFormats();

/// D3D11 memory formats: "video/x-raw(memory:D3D11Memory),format={...}"
QByteArray d3d11CapsFormats();

/// Android HardwareBuffer formats: "video/x-raw(memory:AndroidHardwareBuffer),format={...}"
QByteArray androidHwBufCapsFormats();

/// GL memory formats: "video/x-raw(memory:GLMemory),format={...}"
QByteArray glMemoryCapsFormats();

/// VA-API surface formats: "video/x-raw(memory:VASurface),format={...}"
QByteArray vaSurfaceCapsFormats();

// ─── DRM fourcc ─────────────────────────────────────────────────────

/// Map a GStreamer video format + plane index to a DRM_FORMAT_* fourcc.
/// Returns -1 for unsupported formats.
int drmFourccForFormat(GstVideoFormat gstFmt, int plane);

/// Map a GStreamer video format to a whole-frame DRM_FORMAT_* fourcc
/// (e.g., DRM_FORMAT_NV12 for NV12). Used for single-EGLImage import
/// where all planes are packed into one EGL image.
/// Returns -1 for unsupported formats.
int drmFourccForWholeFrame(GstVideoFormat gstFmt);

}  // namespace GstFormatTable
