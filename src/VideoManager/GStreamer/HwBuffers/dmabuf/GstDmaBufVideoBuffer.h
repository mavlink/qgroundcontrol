#pragma once

#include <QtCore/qglobal.h>

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)

#include "GstHwVideoBuffer.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>

class QRhi;

/// \brief Zero-copy QVideoFrame backing for GStreamer DMABuf samples. mapTextures() (render thread) waits for the
/// producer's writes, then hands the planes to Qt's DMABuf importer under a GL QRhi, or imports them as VkImages under
/// a Vulkan QRhi (GstDmaBufVulkanImport). The buffer stays referenced until the textures are gone and the GPU has
/// drawn with them.
class GstDmaBufVideoBuffer final : public GstHwVideoBuffer
{
public:
    /// @p eglDisplay is Qt's EGLDisplay for an import under OpenGL (eglDisplayFor()); EGL_NO_DISPLAY otherwise.
    GstDmaBufVideoBuffer(GstSample* sample, const GstVideoInfo& videoInfo, const QVideoFrameFormat& format,
                         EGLDisplay eglDisplay);

    /// @p context's importer when it is Qt's EGLDisplay, as under OpenGL. EGL_NO_DISPLAY when it is the Vulkan device,
    /// which EGL must never be asked about.
    static EGLDisplay eglDisplayFor(const HwVideoBufferContext& context);

    /// True when @p fd is a dma-buf to the kernel. GStreamer's dmabuf allocator wraps any fd, so the memory type
    /// doesn't tell.
    static bool isDmaBufFd(int fd) noexcept;

    bool isDmaBuf() const override { return true; }

    MapData map(QVideoFrame::MapMode mode) override;
    QVideoFrameTexturesUPtr mapTextures(QRhi& rhi, QVideoFrameTexturesUPtr& oldTextures) override;
    bool validatePlaneHandles() const override;

    /// Failed imports of one layout in a row that disable the path: Qt's importer warns on every one.
    static constexpr int kMaxConsecutiveImportFailures = 30;

    /// How mapTextures() ended, for the disable latch.
    enum class ImportOutcome
    {
        Imported,
        NotImportable,  ///< Stopped before the driver was asked: by the QRhi mapping it, or by the frame's layout.
        Rejected,       ///< The driver refused the frame.
        Unrecoverable,  ///< The driver refused it in a way no later frame avoids (EGL_BAD_MATCH).
        FdDupFailed,    ///< A plane's dma-buf fd could not be duplicated (EMFILE, EBADF): no fault of the driver's.
    };

    /// True once an import failed in a way later frames can't avoid; cleared by resetCachedState().
    static bool importDisabled() noexcept;
    static void resetCachedState() noexcept;

#ifdef QGC_UNITTEST_BUILD
    /// Feeds one import the driver refused, of no particular frame, into the disable latch.
    static void recordImportFailureForTest(bool unrecoverable);
    /// Feeds the outcome of one import of this frame into the disable latch, as mapTextures() does.
    void noteImportOutcomeForTest(ImportOutcome outcome) const;
    /// The wait on the producer's writes mapTextures() runs before importing @p buffer; true once they completed.
    static bool waitForProducerWritesForTest(GstBuffer* buffer);
    /// Stands in for the fence of the GPU's work when a GL import's textures are deleted: @p fd turns readable once
    /// the test lets the GPU be done. -1 restores EGL's fence.
    static void setGpuFenceForTest(int fd);
#endif

private:
    /// The import itself; without textures, @p failure tells why when the driver refused or a dma-buf fd could not be
    /// duplicated.
    QVideoFrameTexturesUPtr importFrame(QRhi& rhi, QVideoFrameTexturesUPtr& old, ImportOutcome& failure);
#if defined(QGC_HAS_GST_DMABUF_VULKAN_IMPORT)
    QVideoFrameTexturesUPtr importVulkan(QRhi& rhi, GstBuffer* buffer, QVideoFrameTexturesUPtr& old,
                                         ImportOutcome& failure);
#endif
    QVideoFrameTexturesUPtr importGl(QRhi& rhi, GstBuffer* buffer, ImportOutcome& failure);
    /// Counts @p outcome in the run of failed imports of this frame's layout, which disables the path at
    /// kMaxConsecutiveImportFailures: a refusal by the driver, and a tiled frame that could not be imported. An fd
    /// that could not be duplicated counts for a tiled frame alone, which has no other way to the screen.
    void noteImportOutcome(ImportOutcome outcome) const;
    /// A vendor tiling from DMA_DRM caps: the memory has no CPU-readable layout.
    bool isTiled() const noexcept;

    EGLDisplay _eglDisplay = EGL_NO_DISPLAY;
    /// From DMA_DRM caps; DRM_FORMAT_MOD_INVALID (implicit, driver-resolved) for legacy DMABuf caps.
    guint64 _drmModifier;
    /// eglfs quirk: DRM YUYV/UYVY fourcc imported as one image the driver converts to RGBA; 0 otherwise.
    quint32 _packedYuvFourcc = 0;
};

#endif  // QGC_HAS_GST_DMABUF_GPU_PATH
