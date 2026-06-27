#pragma once

#include <QtCore/qglobal.h>

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH) && defined(QGC_HAS_GST_VULKAN_GPU_PATH)

namespace GstDmaBufVulkan {

/// Reset the once-logged warning flags for the Vulkan DMABuf import (test/reinit hook). Render-thread or pre-init only.
void resetLoggedState() noexcept;

}  // namespace GstDmaBufVulkan

#endif  // QGC_HAS_GST_DMABUF_GPU_PATH && QGC_HAS_GST_VULKAN_GPU_PATH
