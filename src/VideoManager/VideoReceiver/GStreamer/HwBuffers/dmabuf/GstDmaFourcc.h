#pragma once

#include <QtCore/qglobal.h>

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)

#include <gst/video/gstvideometa.h>
#include <gst/video/video-info.h>

namespace GstHw {

/// Byte offset and row stride of one plane within its dma-buf.
struct PlaneLayout
{
    gsize offset = 0;
    gint stride = 0;
};

/// Layout of plane @p plane: from @p vmeta when present (decoders pad rows/planes beyond the caps layout), else @p
/// info.
PlaneLayout planeLayout(const GstVideoInfo& info, const GstVideoMeta* vmeta, int plane) noexcept;

/// DRM fourcc for plane @p plane of @p info (per-plane EGLImage import). Returns -1 for formats with no DRM mapping.
/// Modeled on Qt's fourccFromVideoInfo() (qgstvideobuffer.cpp, LGPL-3).
int drmFourccForPlane(const GstVideoInfo& info, int plane);

/// DRM fourcc for formats importable as a single EGLImage/VkImage (all planes, one fd). Returns -1 otherwise.
int drmFourccForSingleFd(const GstVideoInfo& info);

}  // namespace GstHw

#endif  // QGC_HAS_GST_DMABUF_GPU_PATH
