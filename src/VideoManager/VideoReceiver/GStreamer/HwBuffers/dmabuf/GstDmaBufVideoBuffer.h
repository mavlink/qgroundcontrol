#pragma once

#include <QtCore/qglobal.h>

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "GstHwVideoBuffer.h"

class QRhi;

/// \brief Zero-copy QVideoFrame backing for GStreamer DMABuf samples; mapTextures() (render thread) imports the dma-buf
/// fds as EGLImages under a GL QRhi or as a VkImage under a Vulkan QRhi.
class GstDmaBufVideoBuffer final : public GstHwVideoBuffer
{
public:
    GstDmaBufVideoBuffer(GstSample* sample, const GstVideoInfo& videoInfo, const QVideoFrameFormat& format,
                         EGLDisplay eglDisplay);

    bool isDmaBuf() const override { return true; }

    QVideoFrameTexturesUPtr mapTextures(QRhi& rhi, QVideoFrameTexturesUPtr& oldTextures) override;
    bool validatePlaneHandles() const override;

    static void resetCachedState() noexcept;

#ifdef QGC_UNITTEST_BUILD
    static bool directGlImportAllowedForTest(guint64 drmModifier) noexcept;
    static bool texStorageImportAllowedForTest(bool contextIsOpenGles, bool hasTexStorageExt) noexcept;
    /// Feeds an eglCreateImage EGL_BAD_MATCH into the import latches, as the single-fd or per-plane import would.
    static void recordEglBadMatchForTest(bool singleFd) noexcept;
#endif

private:
#if defined(QGC_HAS_GST_VULKAN_GPU_PATH)
    QVideoFrameTexturesUPtr importVulkan(QRhi& rhi, QVideoFrameTexturesUPtr& old);
#endif

    EGLDisplay _eglDisplay = EGL_NO_DISPLAY;
    guint64 _drmModifier = 0;  // 0 = LINEAR / non-DRM; parsed once from caps in the ctor
};

#endif  // QGC_HAS_GST_DMABUF_GPU_PATH
