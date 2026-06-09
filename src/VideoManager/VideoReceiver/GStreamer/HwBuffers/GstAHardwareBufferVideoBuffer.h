#pragma once

#include <QtCore/qglobal.h>

#if defined(QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH)

#include "GstHwVideoBuffer.h"

#include <EGL/egl.h>

class QRhi;

/// \brief Zero-copy QVideoFrame backing for GStreamer AHardwareBuffer samples (Android).
///
/// Imports via eglGetNativeClientBufferANDROID + eglCreateImageKHR into GL textures.
/// Requires EGL_ANDROID_image_native_buffer.
///
class GstAHardwareBufferVideoBuffer final : public GstHwVideoBuffer
{
public:
    GstAHardwareBufferVideoBuffer(GstSample *sample,
                                  const GstVideoInfo &videoInfo,
                                  const QVideoFrameFormat &format,
                                  EGLDisplay eglDisplay);
    ~GstAHardwareBufferVideoBuffer() override;

    MapData map(QVideoFrame::MapMode mode) override;
    QVideoFrameTexturesUPtr mapTextures(QRhi &rhi, QVideoFrameTexturesUPtr &oldTextures) override;
    bool validatePlaneHandles() const override;

    static quint64 takeMapFailureCount();
    static quint64 peekMapFailureCount();

private:
    EGLDisplay _eglDisplay = EGL_NO_DISPLAY;
};

#endif // QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH
