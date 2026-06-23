#pragma once

#include <QtCore/qglobal.h>

#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)

#include "GstHwVideoBuffer.h"

class QRhi;

/// \brief Zero-copy QVideoFrame backing for GStreamer GLMemory samples.
///
/// Wraps GstGLMemory texture ids in QRhiTextures. Requires the QRhi GL context
/// to share with the GstGLContext — see GstGlContextBridge for the wiring.
///
class GstGlVideoBuffer final : public GstHwVideoBuffer
{
public:
    /// @p sample is ref'd; the buffer keeps it alive until destruction.
    GstGlVideoBuffer(GstSample *sample,
                     const GstVideoInfo &videoInfo,
                     const QVideoFrameFormat &format);
    ~GstGlVideoBuffer() override;

    MapData map(QVideoFrame::MapMode mode) override;
    QVideoFrameTexturesUPtr mapTextures(QRhi &rhi, QVideoFrameTexturesUPtr &oldTextures) override;
    bool validatePlaneHandles() const override;

    /// Process-wide failure tally (read+reset / read-only). Mirrors GstDmaBufVideoBuffer
    /// so the adapter can log both paths uniformly on teardown.
    static quint64 takeMapFailureCount();
    static quint64 peekMapFailureCount();

    /// Per-process count of mapTextures calls that returned the previous QRhiTexture wrapper
    /// instead of allocating a new one (gst-gl pool returned the same plane id).
    static quint64 takeTextureReuseHits();
    static quint64 peekTextureReuseHits();

    /// Read-and-reset the per-process sync-wait split. Returns CPU-blocking-wait count;
    /// writes the GPU-side-wait count into @p gpuWaits. Lets the adapter expose how often
    /// the cheap GPU fence path was taken vs the conservative CPU block.
    static quint64 takeSyncWaitCounts(quint64 &gpuWaits);
};

#endif // QGC_HAS_GST_GLMEMORY_GPU_PATH
