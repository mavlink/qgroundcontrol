#pragma once

#include <QtCore/qglobal.h>

#if defined(QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH)

#include <EGL/egl.h>
#include <QtGui/QMatrix4x4>

#include "GstHwVideoBuffer.h"

class QRhi;

/// \brief Zero-copy QVideoFrame backing for GStreamer AHardwareBuffer samples (Android); imports via
/// eglGetNativeClientBufferANDROID + eglCreateImageKHR (needs EGL_ANDROID_image_native_buffer).
class GstAHardwareBufferVideoBuffer final : public GstHwVideoBuffer
{
public:
    GstAHardwareBufferVideoBuffer(GstSample* sample, const GstVideoInfo& videoInfo, const QVideoFrameFormat& format,
                                  EGLDisplay eglDisplay);

    QVideoFrameTexturesUPtr mapTextures(QRhi& rhi, QVideoFrameTexturesUPtr& oldTextures) override;
    bool validatePlaneHandles() const override;

    /// External-OES texcoord transform consumed by qvideotexturehelper for Format_SamplerExternalOES.
    QMatrix4x4 externalTextureMatrix() const override;

    const char* storageTag() const override { return "AHardwareBuffer"; }

    static void resetImageCache() noexcept;

private:
    EGLDisplay _eglDisplay = EGL_NO_DISPLAY;
};

#endif  // QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH
