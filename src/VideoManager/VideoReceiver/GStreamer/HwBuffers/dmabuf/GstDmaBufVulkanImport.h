#pragma once

#include <QtCore/qglobal.h>

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH) && defined(QGC_HAS_GST_VULKAN_GPU_PATH)

#include <gst/video/video-info.h>

namespace GstDmaBufVulkan {

/// True when @p videoInfo's layout can be imported as a single QRhi Vulkan texture; safe from any thread.
bool canImport(const GstVideoInfo& videoInfo) noexcept;

/// Re-arm the once-logged warnings and force every render thread to re-resolve its Vulkan device functions.
void resetCachedState() noexcept;

}  // namespace GstDmaBufVulkan

#endif  // QGC_HAS_GST_DMABUF_GPU_PATH && QGC_HAS_GST_VULKAN_GPU_PATH
