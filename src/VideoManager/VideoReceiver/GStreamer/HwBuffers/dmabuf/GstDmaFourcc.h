#pragma once

#include <QtCore/qglobal.h>

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)

#include <gst/video/video-info.h>

#include <cstdint>

namespace GstHw {

/// DRM fourcc for plane @p plane of @p info (per-plane EGLImage import). Returns -1 for formats with no DRM mapping.
/// Modeled on Qt's fourccFromVideoInfo() (qgstvideobuffer.cpp, LGPL-3).
int drmFourccForPlane(const GstVideoInfo& info, int plane);

/// DRM fourcc for formats importable as a single EGLImage/VkImage (all planes, one fd). Returns -1 otherwise.
int drmFourccForSingleFd(const GstVideoInfo& info);

/// GStreamer format token (e.g. "NV12") for a DRM fourcc our import path can consume; nullptr otherwise. Reverse of
/// the fourccs drmFourccForSingleFd / drmFourccForPlane emit, restricted to renderable single-image formats so caps
/// advertising it stays consistent with what the EGLImage importer actually accepts.
const char* gstFormatNameForImportableFourcc(uint32_t fourcc) noexcept;

}  // namespace GstHw

#endif  // QGC_HAS_GST_DMABUF_GPU_PATH
