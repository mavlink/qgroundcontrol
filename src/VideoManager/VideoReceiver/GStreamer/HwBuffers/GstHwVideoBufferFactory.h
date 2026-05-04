#pragma once

#include <QtCore/qglobal.h>
#include <QtMultimedia/QVideoFrameFormat>

#include <gst/gst.h>
#include <gst/video/video-info.h>

#include <memory>

class QHwVideoBuffer;

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH) || defined(QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH)
#  include <EGL/egl.h>
#endif

/// Identifies which GPU path was chosen; used by the adapter to increment the right counter.
enum class HwVideoBufferPath {
    None,
    DmaBuf,
    GlMemory,
    D3D11,
    D3D12,
    IOSurface,
    AHardwareBuffer,
};

/// Factory that selects the appropriate zero-copy QHwVideoBuffer for a GstSample.
///
/// Returns nullptr if no compiled GPU path matches the buffer's memory type,
/// in which case the caller should fall back to the CPU memcpy path.
/// @p gpuEnabled must be true for any GPU path to be attempted.
std::unique_ptr<QHwVideoBuffer> makeHwVideoBuffer(
    GstSample *sample,
    const GstVideoInfo &info,
    QVideoFrameFormat format,
    bool gpuEnabled,
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    EGLDisplay eglDisplay,
#else
    void *eglDisplay,
#endif
#if defined(QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH)
    EGLDisplay ahbEglDisplay,
#else
    void *ahbEglDisplay,
#endif
    HwVideoBufferPath &matchedPath
);
