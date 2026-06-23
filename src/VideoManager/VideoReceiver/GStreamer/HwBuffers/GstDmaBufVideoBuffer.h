#pragma once

#include <QtCore/qglobal.h>

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)

#include "GstHwVideoBuffer.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>

class QRhi;

/// \brief Zero-copy QVideoFrame backing for GStreamer DMABuf samples.
///
/// Imports per-plane DMABuf fds into EGLImages on demand in mapTextures(),
/// which Qt invokes on the render thread with a current GL context.
///
class GstDmaBufVideoBuffer final : public GstHwVideoBuffer
{
public:
    GstDmaBufVideoBuffer(GstSample *sample,
                         const GstVideoInfo &videoInfo,
                         const QVideoFrameFormat &format,
                         EGLDisplay eglDisplay);

    MapData map(QVideoFrame::MapMode mode) override;
    bool isDmaBuf() const override { return true; }

    ~GstDmaBufVideoBuffer() override;

    QVideoFrameTexturesUPtr mapTextures(QRhi &rhi, QVideoFrameTexturesUPtr &oldTextures) override;
    bool validatePlaneHandles() const override;

    static quint64 takeMapFailureCount();
    static quint64 peekMapFailureCount();

private:
    EGLDisplay _eglDisplay = EGL_NO_DISPLAY;
};

#endif // QGC_HAS_GST_DMABUF_GPU_PATH
