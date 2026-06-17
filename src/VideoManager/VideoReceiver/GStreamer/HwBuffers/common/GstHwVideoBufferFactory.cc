#include "GstHwVideoBufferFactory.h"

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QSet>
#include <QtCore/QString>
#include <private/qhwvideobuffer_p.h>

#include "GstHwPathTelemetry.h"
#include "QGCLoggingCategory.h"

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
#include <gst/allocators/gstdmabuf.h>

#include "GstDmaBufVideoBuffer.h"
#endif
#if defined(QGC_HAS_GST_CUDA_GPU_PATH)
#include <gst/cuda/gstcuda.h>

#include "GstCudaVideoBuffer.h"
#endif
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
#include <gst/gl/gstglmemory.h>

#include "GstGlVideoBuffer.h"
#endif
#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
#include <gst/d3d11/gstd3d11.h>

#include "GstD3D11VideoBuffer.h"
#endif
#if defined(QGC_HAS_GST_D3D12_GPU_PATH)
#include <gst/d3d12/gstd3d12.h>

#include "GstD3D12VideoBuffer.h"
#endif
#if defined(QGC_HAS_GST_IOSURFACE_GPU_PATH)
#include "GstIOSurfaceVideoBuffer.h"
#endif
#if defined(QGC_HAS_GST_VULKAN_GPU_PATH)
#include <vulkan/vulkan.h>
// `slots` member in gst-vulkan video headers vs Qt's `slots` keyword macro — see GstVulkanVideoBuffer.cc.
#pragma push_macro("slots")
#undef slots
#include <gst/vulkan/gstvkimagememory.h>
#pragma pop_macro("slots")

#include "GstVulkanVideoBuffer.h"
#endif
#if defined(QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH)
#include <gst/android/gstandroid.h>

#include "GstAHardwareBufferVideoBuffer.h"
#endif

QGC_LOGGING_CATEGORY(GstHwBufFactoryLog, "Video.GStreamer.HwBuffers.Factory")


namespace {

const char* pathName(HwVideoBufferPath path) noexcept
{
    switch (path) {
        case HwVideoBufferPath::DmaBuf:          return "DMABuf";
        case HwVideoBufferPath::GlMemory:        return "GL";
        case HwVideoBufferPath::D3D11:           return "D3D11";
        case HwVideoBufferPath::D3D12:           return "D3D12";
        case HwVideoBufferPath::IOSurface:       return "IOSurface";
        case HwVideoBufferPath::AHardwareBuffer: return "AHWBuf";
        case HwVideoBufferPath::Vulkan:          return "Vulkan";
        case HwVideoBufferPath::None:            break;
    }
    return "None";
}

// Construct the QHwVideoBuffer for an already-resolved path (fast path: skips the gst_is_*_memory ladder). Returns
// nullptr for paths that need extra context the cache can't carry, forcing a full re-resolve.
std::unique_ptr<QHwVideoBuffer> constructForPath(HwVideoBufferPath path, GstSample* sample,
                                                 [[maybe_unused]] const GstVideoInfo& info,
                                                 [[maybe_unused]] QVideoFrameFormat format,
                                                 [[maybe_unused]] const HwVideoBufferContext& context)
{
    switch (path) {
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
        case HwVideoBufferPath::DmaBuf:
            if (context.dmaBufEglDisplay == EGL_NO_DISPLAY) {
                return nullptr;
            }
            return std::make_unique<GstDmaBufVideoBuffer>(sample, info, format, context.dmaBufEglDisplay);
#endif
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
        case HwVideoBufferPath::GlMemory:
            return std::make_unique<GstGlVideoBuffer>(sample, info, format);
#endif
#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
        case HwVideoBufferPath::D3D11:
            return std::make_unique<GstD3D11VideoBuffer>(sample, info, format);
#endif
#if defined(QGC_HAS_GST_D3D12_GPU_PATH)
        case HwVideoBufferPath::D3D12:
            return std::make_unique<GstD3D12VideoBuffer>(sample, info, format);
#endif
#if defined(QGC_HAS_GST_VULKAN_GPU_PATH)
        case HwVideoBufferPath::Vulkan:
            return std::make_unique<GstVulkanVideoBuffer>(sample, info, format);
#endif
#if defined(QGC_HAS_GST_IOSURFACE_GPU_PATH)
        case HwVideoBufferPath::IOSurface:
            return std::make_unique<GstIOSurfaceVideoBuffer>(sample, info, format);
#endif
#if defined(QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH)
        case HwVideoBufferPath::AHardwareBuffer:
            if (context.ahbEglDisplay == EGL_NO_DISPLAY) {
                return nullptr;
            }
            format.setPixelFormat(QVideoFrameFormat::Format_SamplerExternalOES);
            return std::make_unique<GstAHardwareBufferVideoBuffer>(sample, info, format, context.ahbEglDisplay);
#endif
        default:
            return nullptr;
    }
}

bool memoryMatchesPath(HwVideoBufferPath path, GstMemory* mem, [[maybe_unused]] const HwVideoBufferContext& context)
{
    if (!mem) {
        return false;
    }

    switch (path) {
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
        case HwVideoBufferPath::DmaBuf:
            return context.dmaBufEglDisplay != EGL_NO_DISPLAY && gst_is_dmabuf_memory(mem);
#endif
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
        case HwVideoBufferPath::GlMemory:
            return gst_is_gl_memory(mem);
#endif
#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
        case HwVideoBufferPath::D3D11:
            return gst_is_d3d11_memory(mem);
#endif
#if defined(QGC_HAS_GST_D3D12_GPU_PATH)
        case HwVideoBufferPath::D3D12:
            return gst_is_d3d12_memory(mem);
#endif
#if defined(QGC_HAS_GST_VULKAN_GPU_PATH)
        case HwVideoBufferPath::Vulkan:
            return gst_is_vulkan_image_memory(mem);
#endif
#if defined(QGC_HAS_GST_IOSURFACE_GPU_PATH)
        case HwVideoBufferPath::IOSurface:
            return mem->allocator && g_strcmp0(mem->allocator->mem_type, "AppleCoreVideoMemory") == 0;
#endif
#if defined(QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH)
        case HwVideoBufferPath::AHardwareBuffer:
            return context.ahbEglDisplay != EGL_NO_DISPLAY && gst_is_ahardware_buffer_memory(mem);
#endif
        case HwVideoBufferPath::None:
            break;
        default:
            break;
    }

    return false;
}

}  // namespace

std::unique_ptr<QHwVideoBuffer> makeHwVideoBuffer(GstSample* sample, [[maybe_unused]] const GstVideoInfo& info,
                                                  [[maybe_unused]] QVideoFrameFormat format,
                                                  const HwVideoBufferContext& context, HwVideoBufferPath& matchedPath,
                                                  HwResolvedPathCache* cache)
{
    matchedPath = HwVideoBufferPath::None;
    if (!context.gpuEnabled || !sample) {
        return nullptr;
    }

    GstBuffer* buffer = gst_sample_get_buffer(sample);
    if (!buffer) {
        return nullptr;
    }

    // Plane 0's allocator is the dispatch key; gst-video buffers from a single decoder use one allocator type across
    // all planes.
    GstMemory* mem0 = gst_buffer_peek_memory(buffer, 0);
    if (!mem0) {
        return nullptr;
    }

    // Fast path: a path was resolved+validated earlier this epoch — construct directly, skip the predicate ladder and
    // the per-sample validatePlaneHandles(). The allocator check stays per-buffer because decoders can change memory
    // type under stable caps; stale cache entries fall through to a full re-resolve.
    if (cache && cache->validated && cache->path != HwVideoBufferPath::None) {
        if (memoryMatchesPath(cache->path, mem0, context)) {
            if (auto buf = constructForPath(cache->path, sample, info, format, context)) {
                matchedPath = cache->path;
                return buf;
            }
        }
        cache->validated = false;
    }

    // Validate before commit — failure resets matchedPath so per-path counters don't double-count.
    auto buildOrFallback = [&matchedPath, &info, cache](auto&& buf) -> std::unique_ptr<QHwVideoBuffer> {
        if (!buf || !buf->validatePlaneHandles()) {
            GstHwPathTelemetry::recordFallbackReason(matchedPath, GstHwPathTelemetry::HwFallbackReason::ValidateFailed);
            static std::atomic<quint64> s_validateFails{0};
            const quint64 c = s_validateFails.fetch_add(1, std::memory_order_relaxed) + 1;
            if ((c & 0x3F) == 1) {
                qCWarning(GstHwBufFactoryLog)
                    << "validatePlaneHandles failed — CPU memcpy fallback (total:" << c << ")";
            }
            matchedPath = HwVideoBufferPath::None;
            return nullptr;
        }
        if (cache && !cache->validated) {
            qCDebug(GstHwBufFactoryLog).noquote()
                << "zero-copy path selected:" << pathName(matchedPath)
                << "format=" << gst_video_format_to_string(GST_VIDEO_INFO_FORMAT(&info))
                << GST_VIDEO_INFO_WIDTH(&info) << "x" << GST_VIDEO_INFO_HEIGHT(&info);
        }
        if (cache) {
            cache->path = matchedPath;
            cache->validated = true;
        }
        return buf;
    };

#if defined(QGC_HAS_GST_CUDA_GPU_PATH)
    // NVMM/CUDA memory exports to a DMABuf fd (gst_cuda_memory_export / Jetson NvBufSurfaceMapEglImage) and then reuses
    // the DMABuf EGLImage path, avoiding a separate CUDA-GL interop. Scaffold: requires NVIDIA hardware to validate.
    if (gst_is_cuda_memory(mem0)) {
        // One-shot capability latch: desktop dGPU drivers can reject the export every frame, so probe once and skip
        // the per-frame retry thereafter (CPU fallback).
        static std::atomic<bool> s_cudaExportUnsupported{false};
        if (s_cudaExportUnsupported.load(std::memory_order_relaxed)) {
            return nullptr;
        }
        matchedPath = HwVideoBufferPath::DmaBuf;
        if (auto buf = buildOrFallback(GstCudaVideoBuffer::exportToDmaBuf(sample, info, format, context))) {
            return buf;
        }
        if (!s_cudaExportUnsupported.exchange(true, std::memory_order_relaxed)) {
            GstHwPathTelemetry::recordFallbackReason(HwVideoBufferPath::DmaBuf,
                                                     GstHwPathTelemetry::HwFallbackReason::NoExt);
            qCWarning(GstHwBufFactoryLog) << "CUDA->DMABuf export unsupported on this driver — CPU fallback for the"
                                          << "remainder of the process";
        }
        matchedPath = HwVideoBufferPath::None;
        return nullptr;
    }
#endif

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    if (context.dmaBufEglDisplay != EGL_NO_DISPLAY && gst_is_dmabuf_memory(mem0)) {
        matchedPath = HwVideoBufferPath::DmaBuf;
        return buildOrFallback(std::make_unique<GstDmaBufVideoBuffer>(sample, info, format, context.dmaBufEglDisplay));
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

#if defined(QGC_HAS_GST_VULKAN_GPU_PATH)
    if (gst_is_vulkan_image_memory(mem0)) {
        matchedPath = HwVideoBufferPath::Vulkan;
        return buildOrFallback(std::make_unique<GstVulkanVideoBuffer>(sample, info, format));
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
    if (context.ahbEglDisplay != EGL_NO_DISPLAY && gst_is_ahardware_buffer_memory(mem0)) {
        matchedPath = HwVideoBufferPath::AHardwareBuffer;
        // GL_TEXTURE_EXTERNAL_OES requires the SamplerExternalOES pixel format for Qt's shader path.
        format.setPixelFormat(QVideoFrameFormat::Format_SamplerExternalOES);
        return buildOrFallback(
            std::make_unique<GstAHardwareBufferVideoBuffer>(sample, info, format, context.ahbEglDisplay));
    }
#endif

    {
        static QSet<QString> s_seen;
        static QMutex s_mtx;
        const QString memType =
            mem0->allocator ? QString::fromUtf8(mem0->allocator->mem_type) : QStringLiteral("<no-allocator>");
        QMutexLocker lock(&s_mtx);
        if (!s_seen.contains(memType)) {
            s_seen.insert(memType);
            qCDebug(GstHwBufFactoryLog) << "no zero-copy path for memory type" << memType
                                        << "— falling back to CPU memcpy";
        }
    }
    GstHwPathTelemetry::recordFallbackReason(HwVideoBufferPath::None,
                                             GstHwPathTelemetry::HwFallbackReason::UnknownMemType);
    if (cache) {
        cache->resolved = true;
    }
    return nullptr;
}
