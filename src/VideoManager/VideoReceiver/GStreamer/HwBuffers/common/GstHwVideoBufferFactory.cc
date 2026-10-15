#include "GstHwVideoBufferFactory.h"

#include <atomic>
#include <private/qhwvideobuffer_p.h>
#include <utility>

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
#include <rhi/qrhi.h>

#include "GstGlVideoBuffer.h"
#include "QGCRhiCapture.h"
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

#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
// GstGlVideoBuffer::mapTextures binds GLMemory only under Qt's OpenGL(ES2) QRhi; the glupload bin still emits GLMemory
// under a non-GL RHI (forced Vulkan), so gate selection here and let the frame take the CPU path instead. Runs on the
// streaming thread, so read the published backend rather than the render-thread-owned QRhi.
bool glMemoryConsumable() noexcept
{
    const int backend = QGCRhiCapture::deviceSnapshot().backend.load(std::memory_order_acquire);
    return (backend < 0) || (backend == static_cast<int>(QRhi::OpenGLES2));
}
#endif

#if defined(QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH)
// GL_TEXTURE_EXTERNAL_OES needs the SamplerExternalOES pixel format for Qt's shader path. QVideoFrameFormat has no
// pixel-format setter, so rebuild the format around it.
QVideoFrameFormat externalOesFormat(const QVideoFrameFormat& format)
{
    QVideoFrameFormat external(format.frameSize(), QVideoFrameFormat::Format_SamplerExternalOES);
    external.setViewport(format.viewport());
    external.setScanLineDirection(format.scanLineDirection());
    external.setStreamFrameRate(format.streamFrameRate());
    external.setColorSpace(format.colorSpace());
    external.setColorTransfer(format.colorTransfer());
    external.setColorRange(format.colorRange());
    external.setMirrored(format.isMirrored());
    external.setRotation(format.rotation());
    external.setMaxLuminance(format.maxLuminance());
    return external;
}
#endif

// Construct the QHwVideoBuffer for an already-resolved path (fast path: skips the gst_is_*_memory ladder). Returns
// nullptr for paths that need extra context the cache can't carry, forcing a full re-resolve.
std::unique_ptr<GstHwVideoBuffer> constructForPath(HwVideoBufferPath path, GstSample* sample,
                                                 [[maybe_unused]] const GstVideoInfo& info,
                                                 [[maybe_unused]] QVideoFrameFormat format,
                                                 [[maybe_unused]] const HwVideoBufferContext& context)
{
    switch (path) {
        case HwVideoBufferPath::DmaBuf:
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
            if (context.dmaBufEglDisplay == EGL_NO_DISPLAY) {
                return nullptr;
            }
            return std::make_unique<GstDmaBufVideoBuffer>(sample, info, format, context.dmaBufEglDisplay);
#else
            break;
#endif
        case HwVideoBufferPath::GlMemory:
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
            return std::make_unique<GstGlVideoBuffer>(sample, info, format);
#else
            break;
#endif
        case HwVideoBufferPath::D3D11:
#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
            return std::make_unique<GstD3D11VideoBuffer>(sample, info, format);
#else
            break;
#endif
        case HwVideoBufferPath::D3D12:
#if defined(QGC_HAS_GST_D3D12_GPU_PATH)
            return std::make_unique<GstD3D12VideoBuffer>(sample, info, format);
#else
            break;
#endif
        case HwVideoBufferPath::Vulkan:
#if defined(QGC_HAS_GST_VULKAN_GPU_PATH)
            return std::make_unique<GstVulkanVideoBuffer>(sample, info, format);
#else
            break;
#endif
        case HwVideoBufferPath::IOSurface:
#if defined(QGC_HAS_GST_IOSURFACE_GPU_PATH)
            return std::make_unique<GstIOSurfaceVideoBuffer>(sample, info, format);
#else
            break;
#endif
        case HwVideoBufferPath::AHardwareBuffer:
#if defined(QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH)
            if (context.ahbEglDisplay == EGL_NO_DISPLAY) {
                return nullptr;
            }
            return std::make_unique<GstAHardwareBufferVideoBuffer>(sample, info, externalOesFormat(format),
                                                                   context.ahbEglDisplay);
#else
            break;
#endif
        case HwVideoBufferPath::None:
        case HwVideoBufferPath::_Count:
            break;
    }

    return nullptr;
}

bool memoryMatchesPath(HwVideoBufferPath path, GstMemory* mem, [[maybe_unused]] const HwVideoBufferContext& context)
{
    if (!mem) {
        return false;
    }

    switch (path) {
        case HwVideoBufferPath::DmaBuf:
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
            return context.dmaBufEglDisplay != EGL_NO_DISPLAY && gst_is_dmabuf_memory(mem);
#else
            break;
#endif
        case HwVideoBufferPath::GlMemory:
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
            return gst_is_gl_memory(mem) && glMemoryConsumable();
#else
            break;
#endif
        case HwVideoBufferPath::D3D11:
#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
            return gst_is_d3d11_memory(mem);
#else
            break;
#endif
        case HwVideoBufferPath::D3D12:
#if defined(QGC_HAS_GST_D3D12_GPU_PATH)
            return gst_is_d3d12_memory(mem);
#else
            break;
#endif
        case HwVideoBufferPath::Vulkan:
#if defined(QGC_HAS_GST_VULKAN_GPU_PATH)
            return gst_is_vulkan_image_memory(mem);
#else
            break;
#endif
        case HwVideoBufferPath::IOSurface:
#if defined(QGC_HAS_GST_IOSURFACE_GPU_PATH)
            return mem->allocator && g_strcmp0(mem->allocator->mem_type, "AppleCoreVideoMemory") == 0;
#else
            break;
#endif
        case HwVideoBufferPath::AHardwareBuffer:
#if defined(QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH)
            return context.ahbEglDisplay != EGL_NO_DISPLAY && gst_is_ahardware_buffer_memory(mem);
#else
            break;
#endif
        case HwVideoBufferPath::None:
        case HwVideoBufferPath::_Count:
            break;
    }

    return false;
}

/// Creates the GstSample a backend buffer holds only once a path has matched the memory, so a frame that ends up on
/// the CPU path never allocates one.
class LazySample
{
public:
    LazySample(GstBuffer* buffer, GstCaps* caps)
        : _buffer(buffer)
        , _caps(caps)
    {}

    explicit LazySample(GstSample* sample)
        : _sample(gst_sample_ref(sample))
    {}

    ~LazySample()
    {
        if (_sample) {
            gst_sample_unref(_sample);
        }
    }

    LazySample(const LazySample&) = delete;
    LazySample& operator=(const LazySample&) = delete;

    GstSample* get()
    {
        if (!_sample) {
            _sample = gst_sample_new(_buffer, _caps, nullptr, nullptr);
        }
        return _sample;
    }

private:
    GstBuffer* _buffer = nullptr;
    GstCaps* _caps = nullptr;
    GstSample* _sample = nullptr;
};

std::unique_ptr<QHwVideoBuffer> selectHwVideoBuffer(LazySample& sample, GstBuffer* buffer,
                                                    [[maybe_unused]] const GstVideoInfo& info,
                                                    [[maybe_unused]] QVideoFrameFormat format,
                                                    const HwVideoBufferContext& context, HwVideoBufferPath& matchedPath,
                                                    HwResolvedPathCache* cache)
{
    matchedPath = HwVideoBufferPath::None;
    if (!context.gpuEnabled || !buffer) {
        return nullptr;
    }

    // Plane 0's allocator is the dispatch key; gst-video buffers from a single decoder use one allocator type across
    // all planes.
    GstMemory* mem0 = gst_buffer_peek_memory(buffer, 0);
    if (!mem0) {
        return nullptr;
    }
    const GType allocatorType = mem0->allocator ? G_OBJECT_TYPE(mem0->allocator) : G_TYPE_INVALID;
    if (cache && (allocatorType != G_TYPE_INVALID) && (allocatorType == cache->unmatchedAllocator)) {
        return nullptr;
    }

    // Validate before commit — failure resets matchedPath so per-path counters don't double-count.
    auto buildOrFallback = [&matchedPath, &info, cache](auto&& buf) -> std::unique_ptr<QHwVideoBuffer> {
        if (!buf || !buf->validatePlaneHandles()) {
            GstHwPathTelemetry::recordFallbackReason(matchedPath, GstHwPathTelemetry::HwFallbackReason::ValidateFailed);
            // Some imports fail on every frame of a stream (e.g. multi-planar DMABuf under a Vulkan QRhi): warn once
            // per caps epoch, not per frame.
            if (!cache || !std::exchange(cache->validationFailureLogged, true)) {
                qCWarning(GstHwBufFactoryLog)
                    << GstHw::toString(matchedPath) << "buffer failed plane-handle validation — CPU fallback";
            }
            if (cache) {
                cache->validated = false;
            }
            matchedPath = HwVideoBufferPath::None;
            return nullptr;
        }
        if (cache && !cache->validated) {
            qCDebug(GstHwBufFactoryLog).noquote()
                << "zero-copy path selected:" << GstHw::toString(matchedPath)
                << "format=" << gst_video_format_to_string(GST_VIDEO_INFO_FORMAT(&info)) << GST_VIDEO_INFO_WIDTH(&info)
                << "x" << GST_VIDEO_INFO_HEIGHT(&info);
        }
        if (cache) {
            cache->path = matchedPath;
            cache->validated = true;
        }
        return buf;
    };

    // Fast path: a path resolved by a validated sample earlier this epoch skips the predicate ladder.
    // Allocator and handle checks stay per-buffer: decoders can swap memory or recycle bad handles under stable caps.
    if (cache && cache->validated && cache->path != HwVideoBufferPath::None) {
        if (memoryMatchesPath(cache->path, mem0, context)) {
            matchedPath = cache->path;
            if (auto buf = constructForPath(cache->path, sample.get(), info, format, context)) {
                return buildOrFallback(std::move(buf));
            }
            matchedPath = HwVideoBufferPath::None;
        }
        cache->validated = false;
    }

#if defined(QGC_HAS_GST_CUDA_GPU_PATH)
    // CUDA memory exported as a DMABuf fd (gst_cuda_memory_export) reuses the DMABuf EGLImage path instead of a
    // CUDA-GL interop. Scaffold: requires NVIDIA hardware to validate.
    if (gst_is_cuda_memory(mem0)) {
        // One-shot capability latch: desktop dGPU drivers can reject the export every frame, so probe once and skip
        // the per-frame retry thereafter (CPU fallback).
        static std::atomic<bool> s_cudaExportUnsupported{false};
        if (s_cudaExportUnsupported.load(std::memory_order_relaxed)) {
            return nullptr;
        }
        matchedPath = HwVideoBufferPath::DmaBuf;
        if (auto buf = buildOrFallback(GstCudaVideoBuffer::exportToDmaBuf(sample.get(), info, format, context))) {
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
        return buildOrFallback(
            std::make_unique<GstDmaBufVideoBuffer>(sample.get(), info, format, context.dmaBufEglDisplay));
    }
#endif

#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
    if (gst_is_gl_memory(mem0)) {
        if (!glMemoryConsumable()) {
            static std::atomic<bool> s_warned{false};
            if (!s_warned.exchange(true, std::memory_order_relaxed)) {
                qCWarning(GstHwBufFactoryLog) << "GLMemory frame but QRhi backend is not OpenGL — routing to CPU pool";
            }
            GstHwPathTelemetry::recordFallbackReason(HwVideoBufferPath::GlMemory,
                                                     GstHwPathTelemetry::HwFallbackReason::ImportUnsupported);
            return nullptr;
        }
        matchedPath = HwVideoBufferPath::GlMemory;
        return buildOrFallback(std::make_unique<GstGlVideoBuffer>(sample.get(), info, format));
    }
#endif

#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
    if (gst_is_d3d11_memory(mem0)) {
        matchedPath = HwVideoBufferPath::D3D11;
        return buildOrFallback(std::make_unique<GstD3D11VideoBuffer>(sample.get(), info, format));
    }
#endif

#if defined(QGC_HAS_GST_D3D12_GPU_PATH)
    if (gst_is_d3d12_memory(mem0)) {
        matchedPath = HwVideoBufferPath::D3D12;
        return buildOrFallback(std::make_unique<GstD3D12VideoBuffer>(sample.get(), info, format));
    }
#endif

#if defined(QGC_HAS_GST_VULKAN_GPU_PATH)
    if (gst_is_vulkan_image_memory(mem0)) {
        matchedPath = HwVideoBufferPath::Vulkan;
        return buildOrFallback(std::make_unique<GstVulkanVideoBuffer>(sample.get(), info, format));
    }
#endif

#if defined(QGC_HAS_GST_IOSURFACE_GPU_PATH)
    // String-compare the allocator name; gst-applemedia exports no public gst_is_apple_core_video_memory() predicate.
    if (mem0->allocator && g_strcmp0(mem0->allocator->mem_type, "AppleCoreVideoMemory") == 0) {
        matchedPath = HwVideoBufferPath::IOSurface;
        return buildOrFallback(std::make_unique<GstIOSurfaceVideoBuffer>(sample.get(), info, format));
    }
#endif

#if defined(QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH)
    if (context.ahbEglDisplay != EGL_NO_DISPLAY && gst_is_ahardware_buffer_memory(mem0)) {
        matchedPath = HwVideoBufferPath::AHardwareBuffer;
        return buildOrFallback(std::make_unique<GstAHardwareBufferVideoBuffer>(
            sample.get(), info, externalOesFormat(format), context.ahbEglDisplay));
    }
#endif

    // No compiled path imports this allocator's memory; that holds for the whole caps epoch.
    if (cache) {
        cache->unmatchedAllocator = allocatorType;
    }
    qCDebug(GstHwBufFactoryLog) << "no zero-copy path for memory type"
                                << (mem0->allocator ? mem0->allocator->mem_type : "<no-allocator>")
                                << "— falling back to CPU";
    GstHwPathTelemetry::recordFallbackReason(HwVideoBufferPath::None,
                                             GstHwPathTelemetry::HwFallbackReason::UnknownMemType);
    return nullptr;
}
}  // namespace

std::unique_ptr<QHwVideoBuffer> makeHwVideoBuffer(GstBuffer* buffer, GstCaps* caps, const GstVideoInfo& info,
                                                  QVideoFrameFormat format, const HwVideoBufferContext& context,
                                                  HwVideoBufferPath& matchedPath, HwResolvedPathCache* cache)
{
    LazySample sample(buffer, caps);
    return selectHwVideoBuffer(sample, buffer, info, std::move(format), context, matchedPath, cache);
}

std::unique_ptr<QHwVideoBuffer> makeHwVideoBuffer(GstSample* sample, const GstVideoInfo& info, QVideoFrameFormat format,
                                                  const HwVideoBufferContext& context, HwVideoBufferPath& matchedPath,
                                                  HwResolvedPathCache* cache)
{
    matchedPath = HwVideoBufferPath::None;
    if (!sample) {
        return nullptr;
    }
    LazySample lazy(sample);
    return selectHwVideoBuffer(lazy, gst_sample_get_buffer(sample), info, std::move(format), context, matchedPath,
                               cache);
}
