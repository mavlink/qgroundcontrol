#include "GstHwPathRegistry.h"

#include <algorithm>
#include <atomic>
#include <private/qhwvideobuffer_p.h>
#include <utility>

#include "GstHwPathTelemetry.h"
#include "GstHwVideoBuffer.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GstHwBufFactoryLog, "Video.GStreamer.HwBuffers.Factory")

namespace GstHwPathRegistry {
namespace {

constexpr const HwPathDescriptor* kCompiledPaths[] = {
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    &GstHwPaths::kDmaBuf,
#endif
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
    &GstHwPaths::kGlMemory,
#endif
#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
    &GstHwPaths::kD3D11,
#endif
#if defined(QGC_HAS_GST_D3D12_GPU_PATH)
    &GstHwPaths::kD3D12,
#endif
#if defined(QGC_HAS_GST_IOSURFACE_GPU_PATH)
    &GstHwPaths::kIOSurface,
#endif
#if defined(QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH)
    &GstHwPaths::kAHardwareBuffer,
#endif
};

#ifdef QGC_UNITTEST_BUILD
constinit std::atomic<const std::vector<const HwPathDescriptor*>*> s_testPaths{nullptr};
#endif

}  // namespace

std::span<const HwPathDescriptor* const> paths() noexcept
{
#ifdef QGC_UNITTEST_BUILD
    if (const auto* testPaths = s_testPaths.load(std::memory_order_acquire)) {
        return *testPaths;
    }
#endif
    return kCompiledPaths;
}

bool has(HwVideoBufferPath path) noexcept
{
    const auto compiled = paths();
    return std::any_of(compiled.begin(), compiled.end(),
                       [path](const HwPathDescriptor* descriptor) { return descriptor->path == path; });
}

#ifdef QGC_UNITTEST_BUILD
ScopedPathsForTest::ScopedPathsForTest(std::initializer_list<const HwPathDescriptor*> paths)
    : _paths(paths)
    , _previous(s_testPaths.exchange(&_paths, std::memory_order_acq_rel))
{}

ScopedPathsForTest::~ScopedPathsForTest()
{
    s_testPaths.store(_previous, std::memory_order_release);
}
#endif

}  // namespace GstHwPathRegistry

std::unique_ptr<QHwVideoBuffer> makeHwVideoBuffer(GstBuffer* buffer, GstCaps* caps, const GstVideoInfo& info,
                                                  QVideoFrameFormat format, const HwVideoBufferContext& context,
                                                  HwVideoBufferPath& matchedPath, HwResolvedPathCache* cache)
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

    // A matched path owns the frame: when it declines or fails, the frame goes to the CPU without trying later paths.
    auto importThrough = [&](const HwPathDescriptor& path) -> std::unique_ptr<QHwVideoBuffer> {
        if (path.admits && !path.admits(mem0, context)) {
            if (cache) {
                cache->validated = false;
            }
            return nullptr;
        }
        matchedPath = path.path;
        // The sample a backend buffer holds, created only now that a path has matched the memory.
        GstSample* const sample = gst_sample_new(buffer, caps, nullptr, nullptr);
        auto imported = buildOrFallback(path.construct(sample, info, format, context));
        gst_sample_unref(sample);
        return imported;
    };

    const auto paths = GstHwPathRegistry::paths();

    // Fast path: only the paths behind a sample validated earlier this epoch are tried. Allocator and handle checks
    // stay per-buffer: decoders can swap memory or recycle bad handles under stable caps.
    if (cache && cache->validated && cache->path != HwVideoBufferPath::None) {
        for (const HwPathDescriptor* path : paths) {
            if ((path->path == cache->path) && path->matches(mem0)) {
                return importThrough(*path);
            }
        }
        cache->validated = false;
    }

    for (const HwPathDescriptor* path : paths) {
        if (path->matches(mem0)) {
            return importThrough(*path);
        }
    }

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
