#pragma once

#include <QtCore/qglobal.h>

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)

#include "GstHwVideoBuffer.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>

class QRhi;

/// \brief Zero-copy QVideoFrame backing for GStreamer DMABuf samples. mapTextures() (render thread) waits for the
/// producer's writes, then hands the planes to Qt's DMABuf importer under a GL QRhi, or imports a VkImage under a
/// Vulkan QRhi.
class GstDmaBufVideoBuffer final : public GstHwVideoBuffer
{
public:
    GstDmaBufVideoBuffer(GstSample* sample, const GstVideoInfo& videoInfo, const QVideoFrameFormat& format,
                         EGLDisplay eglDisplay);

    bool isDmaBuf() const override { return true; }

    MapData map(QVideoFrame::MapMode mode) override;
    QVideoFrameTexturesUPtr mapTextures(QRhi& rhi, QVideoFrameTexturesUPtr& oldTextures) override;
    bool validatePlaneHandles() const override;

    /// Failed imports in a row that disable the path: Qt's importer warns on every one.
    static constexpr int kMaxConsecutiveImportFailures = 30;

    /// True once an import failed in a way later frames can't avoid; cleared by resetCachedState().
    static bool importDisabled() noexcept;
    static void resetCachedState() noexcept;

#ifdef QGC_UNITTEST_BUILD
    /// Feeds one failed import into the disable latch, as mapTextures() does.
    static void recordImportFailureForTest(bool unrecoverable) noexcept;
#endif

private:
#if defined(QGC_HAS_GST_VULKAN_GPU_PATH)
    QVideoFrameTexturesUPtr importVulkan(QRhi& rhi, QVideoFrameTexturesUPtr& old);
#endif
    QVideoFrameTexturesUPtr importGl(QRhi& rhi, GstBuffer* buffer);
    /// A vendor tiling from DMA_DRM caps: the memory has no CPU-readable layout.
    bool isTiled() const noexcept;

    EGLDisplay _eglDisplay = EGL_NO_DISPLAY;
    /// From DMA_DRM caps; DRM_FORMAT_MOD_INVALID (implicit, driver-resolved) for legacy DMABuf caps.
    guint64 _drmModifier;
    /// eglfs quirk: DRM YUYV/UYVY fourcc imported as one image the driver converts to RGBA; 0 otherwise.
    quint32 _packedYuvFourcc = 0;
};

#endif  // QGC_HAS_GST_DMABUF_GPU_PATH
