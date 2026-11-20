#pragma once

#include <QtCore/qglobal.h>

#if defined(QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH)

#include <QtGui/QMatrix4x4>

#include "GstHwVideoBuffer.h"
#include <EGL/egl.h>

class QRhi;

/// \brief QVideoFrame backing for GStreamer AHardwareBuffer samples (Android); imports via
/// eglGetNativeClientBufferANDROID + eglCreateImageKHR (needs EGL_ANDROID_image_native_buffer). Never compiled or run:
/// it needs <gst/android/gstandroid.h> (gst_is_ahardware_buffer_memory), which no GStreamer release through 1.28 nor
/// its main branch has, so it builds only against an SDK that adds that API.
class GstAHardwareBufferVideoBuffer final : public GstHwVideoBuffer
{
public:
    GstAHardwareBufferVideoBuffer(GstSample* sample, const GstVideoInfo& videoInfo, const QVideoFrameFormat& format,
                                  EGLDisplay eglDisplay);

    QVideoFrameTexturesUPtr mapTextures(QRhi& rhi, QVideoFrameTexturesUPtr& oldTextures) override;
    bool validatePlaneHandles() const override;

    /// External-OES texcoord transform consumed by qvideotexturehelper for Format_SamplerExternalOES.
    QMatrix4x4 externalTextureMatrix() const override;

    static void resetImageCache();

private:
    EGLDisplay _eglDisplay = EGL_NO_DISPLAY;
};

#endif  // QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH
