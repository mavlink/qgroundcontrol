#pragma once

#include <QtCore/qglobal.h>

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "GstHwVideoBuffer.h"

class QRhi;

/// \brief Zero-copy QVideoFrame backing for GStreamer DMABuf samples; imports per-plane fds into EGLImages on demand in
/// mapTextures() (render thread, current GL context).
class GstDmaBufVideoBuffer final : public GstHwVideoBuffer
{
public:
    GstDmaBufVideoBuffer(GstSample* sample, const GstVideoInfo& videoInfo, const QVideoFrameFormat& format,
                         EGLDisplay eglDisplay);

    bool isDmaBuf() const override { return true; }

    QVideoFrameTexturesUPtr mapTextures(QRhi& rhi, QVideoFrameTexturesUPtr& oldTextures) override;
    bool validatePlaneHandles() const override;

    const char* storageTag() const override { return "DMABuf"; }

    static void resetCachedState() noexcept;

#ifdef QGC_GST_BUILD_TESTING
    static bool singleFdImportEnabledForTest() noexcept;
    static bool directGlImportAllowedForTest(bool hasModifiersExt, guint64 drmModifier) noexcept;
    static bool texStorageImportAllowedForTest(bool contextIsOpenGles, bool hasTexStorageExt,
                                               guint64 drmModifier) noexcept;
#endif

private:
#if defined(QGC_HAS_GST_VULKAN_GPU_PATH)
    QVideoFrameTexturesUPtr importVulkan(QRhi& rhi);
#endif

    EGLDisplay _eglDisplay = EGL_NO_DISPLAY;
    guint64 _drmModifier = 0;  // 0 = LINEAR / non-DRM; parsed once from caps in the ctor
};

#endif  // QGC_HAS_GST_DMABUF_GPU_PATH
