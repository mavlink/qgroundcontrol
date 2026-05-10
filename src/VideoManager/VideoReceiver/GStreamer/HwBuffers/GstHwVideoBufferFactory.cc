#include "GstHwVideoBufferFactory.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QSet>
#include <QtCore/QString>
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
#  include <QtGui/QGuiApplication>
#endif

#include <private/qhwvideobuffer_p.h>

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
#  include "GstDmaBufVideoBuffer.h"
#  include <gst/allocators/gstdmabuf.h>
#endif
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
#  include "GstGlVideoBuffer.h"
#  include <gst/gl/gstglmemory.h>
#endif
#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
#  include "GstD3D11VideoBuffer.h"
#  include <gst/d3d11/gstd3d11.h>
#endif
#if defined(QGC_HAS_GST_D3D12_GPU_PATH)
#  include "GstD3D12VideoBuffer.h"
#  include <gst/d3d12/gstd3d12.h>
#endif
#if defined(QGC_HAS_GST_IOSURFACE_GPU_PATH)
#  include "GstIOSurfaceVideoBuffer.h"
#endif
#if defined(QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH)
#  include "GstAHardwareBufferVideoBuffer.h"
#  include <gst/android/gstandroid.h>
#endif

QGC_LOGGING_CATEGORY(GstHwBufFactoryLog, "Video.GStreamer.HwBuffers.Factory")

std::unique_ptr<QHwVideoBuffer> makeHwVideoBuffer(
    GstSample *sample,
    [[maybe_unused]] const GstVideoInfo &info,
    [[maybe_unused]] QVideoFrameFormat format,
    bool gpuEnabled,
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    EGLDisplay eglDisplay,
#else
    void * /*eglDisplay*/,
#endif
#if defined(QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH)
    EGLDisplay ahbEglDisplay,
#else
    void * /*ahbEglDisplay*/,
#endif
    HwVideoBufferPath &matchedPath)
{
    matchedPath = HwVideoBufferPath::None;
    if (!gpuEnabled || !sample) {
        return nullptr;
    }

    GstBuffer *buffer = gst_sample_get_buffer(sample);
    if (!buffer) {
        return nullptr;
    }

    // Plane 0's allocator type is the dispatch key; gst-video buffers from a single decoder use one allocator across all planes.
    GstMemory *mem0 = gst_buffer_peek_memory(buffer, 0);
    if (!mem0) {
        return nullptr;
    }

    // Validate before commit — failure resets matchedPath so per-path counters don't double-count.
    auto buildOrFallback = [&matchedPath](auto &&buf) -> std::unique_ptr<QHwVideoBuffer> {
        if (!buf || !buf->validatePlaneHandles()) {
            static std::atomic<quint64> s_validateFails{0};
            const quint64 c = s_validateFails.fetch_add(1, std::memory_order_relaxed) + 1;
            if ((c & 0x3F) == 1) {
                qCWarning(GstHwBufFactoryLog)
                    << "validatePlaneHandles failed — CPU memcpy fallback (total:" << c << ")";
            }
            matchedPath = HwVideoBufferPath::None;
            return nullptr;
        }
        return std::move(buf);
    };

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    if (eglDisplay != EGL_NO_DISPLAY && gst_is_dmabuf_memory(mem0)) {
        matchedPath = HwVideoBufferPath::DmaBuf;
        // Pi/eglfs quirk: the EGL impl on RPi (Broadcom/Mesa V3D) does implicit YUV→RGB conversion
        // when sampling YUYV/UYVY DMABuf via EGLImage, so the pixel data is already RGBA on the GPU
        // texture. Declaring the format as RGBA8888 picks Qt's RGB sampler shader (otherwise Qt
        // applies a YUV→RGB matrix in software, double-converting). Mirror's Qt's own backend
        // (qtmultimedia/.../qgstvideorenderersink.cpp:render — eglfs UYVY/YUYV branch).
        static const bool isEglfsQPA =
            QGuiApplication::platformName() == QLatin1String("eglfs");
        if (isEglfsQPA
                && (format.pixelFormat() == QVideoFrameFormat::Format_UYVY
                    || format.pixelFormat() == QVideoFrameFormat::Format_YUYV)) {
            QVideoFrameFormat spoofed(format.frameSize(), QVideoFrameFormat::Format_RGBA8888);
            spoofed.setStreamFrameRate(format.streamFrameRate());
            spoofed.setColorRange(format.colorRange());
            spoofed.setColorSpace(format.colorSpace());
            spoofed.setColorTransfer(format.colorTransfer());
            spoofed.setViewport(format.viewport());
            format = std::move(spoofed);
        }
        return buildOrFallback(std::make_unique<GstDmaBufVideoBuffer>(sample, info, format, eglDisplay));
    }
#endif

#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
    if (gst_is_gl_memory(mem0)) {
        matchedPath = HwVideoBufferPath::GlMemory;
        return buildOrFallback(std::make_unique<GstGlVideoBuffer>(sample, info, format));
    }
#endif

#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
    if (gst_is_d3d11_memory(mem0)) {
        matchedPath = HwVideoBufferPath::D3D11;
        return buildOrFallback(std::make_unique<GstD3D11VideoBuffer>(sample, info, format));
    }
#endif

#if defined(QGC_HAS_GST_D3D12_GPU_PATH)
    if (gst_is_d3d12_memory(mem0)) {
        matchedPath = HwVideoBufferPath::D3D12;
        return buildOrFallback(std::make_unique<GstD3D12VideoBuffer>(sample, info, format));
    }
#endif

#if defined(QGC_HAS_GST_IOSURFACE_GPU_PATH)
    // String-compare the allocator name; gst-applemedia exports no public gst_is_apple_core_video_memory() predicate.
    if (mem0->allocator && g_strcmp0(mem0->allocator->mem_type, "AppleCoreVideoMemory") == 0) {
        matchedPath = HwVideoBufferPath::IOSurface;
        return buildOrFallback(std::make_unique<GstIOSurfaceVideoBuffer>(sample, info, format));
    }
#endif

#if defined(QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH)
    if (ahbEglDisplay != EGL_NO_DISPLAY && gst_is_ahardware_buffer_memory(mem0)) {
        matchedPath = HwVideoBufferPath::AHardwareBuffer;
        // GL_TEXTURE_EXTERNAL_OES requires the SamplerExternalOES pixel format for Qt's shader path.
        format.setPixelFormat(QVideoFrameFormat::Format_SamplerExternalOES);
        return buildOrFallback(std::make_unique<GstAHardwareBufferVideoBuffer>(sample, info, format, ahbEglDisplay));
    }
#endif

    {
        static QSet<QString> s_seen;
        static QMutex s_mtx;
        const QString memType = mem0->allocator
            ? QString::fromUtf8(mem0->allocator->mem_type)
            : QStringLiteral("<no-allocator>");
        QMutexLocker lock(&s_mtx);
        if (!s_seen.contains(memType)) {
            s_seen.insert(memType);
            qCDebug(GstHwBufFactoryLog) << "no zero-copy path for memory type"
                                        << memType << "— falling back to CPU memcpy";
        }
    }
    return nullptr;
}
