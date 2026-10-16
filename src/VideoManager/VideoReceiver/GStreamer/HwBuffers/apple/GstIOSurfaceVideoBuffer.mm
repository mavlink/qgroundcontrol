#include "GstIOSurfaceVideoBuffer.h"

#include "GstHwFrameTexturesBase.h"
#include "GstHwPathRegistry.h"
#include "GstHwPathTelemetry.h"
#include "GstHwVideoBuffer.h"

#if (defined(Q_OS_MACOS) || defined(Q_OS_IOS)) && defined(QGC_HAS_GST_IOSURFACE_GPU_PATH)

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <gst/video/video.h>
#include <private/qvideotexturehelper_p.h>
#include <rhi/qrhi.h>
#include <span>

#include <QtCore/QLoggingCategory>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QSize>

#include "QGCLoggingCategory.h"
#include <CoreVideo/CVMetalTexture.h>
#include <CoreVideo/CVMetalTextureCache.h>
#include <CoreVideo/CoreVideo.h>
#include <IOSurface/IOSurfaceRef.h>  // Public C API on macOS and iOS; the IOSurface/IOSurface.h umbrella is macOS-only.
#import <Metal/Metal.h>

QGC_LOGGING_CATEGORY(GstIOSurfaceLog, "Video.GStreamer.HwBuffers.GstIOSurface")

namespace {

using GstHw::kMaxPlanes;

// One-shot per failure cause; teardown emits the running counter. Shared causes come from
// GstHw::MapDiagnostics; the remainder are IOSurface/Metal-specific.
struct IOSurfaceDiagnostics : GstHw::MapDiagnostics
{
    std::atomic<bool> loggedNoMemory{false};
    std::atomic<bool> loggedMetaLayoutMismatch{false};
    std::atomic<bool> loggedPixelBufferExtractFail{false};
    std::atomic<bool> loggedNoIOSurface{false};
    std::atomic<bool> loggedNoMtlDevice{false};
    std::atomic<bool> loggedUnsupportedFormat{false};
    std::atomic<bool> loggedCacheCreateFail{false};
};
IOSurfaceDiagnostics s_diag;

// Process-wide CVMetalTextureCache, keyed by MTLDevice*. Apple's documented bridge between
// CVPixelBuffer/IOSurface and Metal — reuses MTLTexture objects when the decoder recycles
// IOSurfaces (typical with VideoToolbox), and avoids per-frame MTLTextureDescriptor allocation.
// Apple's docs guarantee thread-safety. Recreated on QRhi device change (window
// destroy/recreate); the live cache is released by reset() at receiver teardown.
struct MetalTextureCache {
    CVMetalTextureCacheRef cache = nullptr;
    void *deviceKey = nullptr; // raw MTLDevice* for compare; the cache holds its own ref to the device
};
QMutex s_cacheMutex;
MetalTextureCache s_cache;

CVMetalTextureCacheRef ensureCacheLocked(id<MTLDevice> device)
{
    void *key = (__bridge void*)device;
    if (s_cache.cache && s_cache.deviceKey == key) {
        return s_cache.cache;
    }
    if (s_cache.cache) {
        CFRelease(s_cache.cache);
        s_cache.cache = nullptr;
        s_cache.deviceKey = nullptr;
    }
    CVMetalTextureCacheRef cache = nullptr;
    const CVReturn r = CVMetalTextureCacheCreate(
        kCFAllocatorDefault, nullptr, device, nullptr, &cache);
    if (r != kCVReturnSuccess || !cache) {
        if (!s_diag.loggedCacheCreateFail.exchange(true, std::memory_order_relaxed)) {
            qCWarning(GstIOSurfaceLog) << "CVMetalTextureCacheCreate failed (CVReturn=" << int(r) << ")";
        }
        return nullptr;
    }
    s_cache.cache = cache;
    s_cache.deviceKey = key;
    return cache;
}



MTLPixelFormat metalPixelFormatForPlane(QVideoFrameFormat::PixelFormat fmt, int plane)
{
    switch (fmt) {
    case QVideoFrameFormat::Format_NV12:
    case QVideoFrameFormat::Format_NV21:
        return plane == 0 ? MTLPixelFormatR8Unorm : MTLPixelFormatRG8Unorm;
    case QVideoFrameFormat::Format_YUV420P:
    case QVideoFrameFormat::Format_YV12:
        return MTLPixelFormatR8Unorm;
    case QVideoFrameFormat::Format_BGRA8888:
    case QVideoFrameFormat::Format_BGRX8888:
        return MTLPixelFormatBGRA8Unorm;
    case QVideoFrameFormat::Format_ARGB8888:
        return MTLPixelFormatRGBA8Unorm;
    case QVideoFrameFormat::Format_P010:
        return plane == 0 ? MTLPixelFormatR16Unorm : MTLPixelFormatRG16Unorm;
    default:
        return MTLPixelFormatInvalid;
    }
}

class FrameTextures final : public GstHwFrameTexturesBase
{
public:
    /// @p handles are the planes' MTLTexture pointers; @p cvtex the CVMetalTextureRefs that keep them alive.
    FrameTextures(QRhi* rhi, QSize size, QVideoFrameFormat::PixelFormat pixelFormat, std::span<const quint64> handles,
                  std::array<CVMetalTextureRef, kMaxPlanes> cvtex)
        : _cvtex(cvtex)
    {
        wrapPlanes(rhi, size, pixelFormat, handles);
    }

    ~FrameTextures() override
    {
        // CVMetalTextureRefs hold the underlying MTLTexture and the CVPixelBuffer's IOSurface
        // refcount; releasing returns them to the cache for reuse. Base dtor clears _srcSample.
        for (int i = 0; i < _count; ++i) {
            if (_cvtex[i]) CFRelease(_cvtex[i]);
        }
    }

    // Replace the per-plane CVMetalTextureRef holders with fresh ones (transferring the
    // IOSurface ref). The underlying MTLTexture handles are unchanged so wrappers stay valid.
    void adoptCvRefs(std::array<CVMetalTextureRef, kMaxPlanes> &cvtex)
    {
        for (int i = 0; i < _count; ++i) {
            if (_cvtex[i]) CFRelease(_cvtex[i]);
            _cvtex[i] = cvtex[i];
            cvtex[i] = nullptr;
        }
    }

private:
    std::array<CVMetalTextureRef, kMaxPlanes> _cvtex;
};


} // namespace

GstIOSurfaceVideoBuffer::GstIOSurfaceVideoBuffer(GstSample *sample,
                                                 const GstVideoInfo &videoInfo,
                                                 const QVideoFrameFormat &format)
    : GstHwVideoBuffer(QVideoFrame::RhiTextureHandle, sample, videoInfo, format)
{
}

bool GstIOSurfaceVideoBuffer::validatePlaneHandles() const
{
    return validatePlanes([](GstMemory *mem) {
        if (!mem || !mem->allocator) return false;
        return g_strcmp0(mem->allocator->mem_type, "AppleCoreVideoMemory") == 0;
    });
}

void GstIOSurfaceVideoBuffer::resetTextureCache() noexcept
{
    QMutexLocker lock(&s_cacheMutex);
    if (s_cache.cache) {
        CFRelease(s_cache.cache);
    }
    s_cache = MetalTextureCache{};
}

QVideoFrameTexturesUPtr GstIOSurfaceVideoBuffer::mapTextures(QRhi &rhi, QVideoFrameTexturesUPtr &old)
{
    // *** UNTESTED on macOS hardware. CI compiles this path; runtime validation TBD. ***
    // Pulls a CVPixelBufferRef from the GstBuffer (gst-vt path), grabs its
    // IOSurfaceRef, imports each plane via [MTLDevice newTextureWithDescriptor:
    // iosurface:plane:], and wraps each MTLTexture in a QRhiTexture.
    // IOSurface is device-agnostic so no shared-MTLDevice bridge is required.
    GstBuffer *buffer = nullptr;
    if (!checkMapPreconditions(rhi, static_cast<int>(QRhi::Metal), GstIOSurfaceLog(), s_diag, buffer)) {
        return GstHwPathTelemetry::fail(HwVideoBufferPath::IOSurface);
    }

    // Mirror of GstCoreVideoMeta from gst-plugins-bad sys/applemedia/corevideobuffer.h (plugin-private, not installed;
    // identical in 1.20-1.28). The registered meta size is checked below before reading past the GstMeta header.
    struct CoreVideoMetaMirror
    {
        GstMeta meta;
        CVBufferRef cvbuf;
        CVPixelBufferRef pixbuf;
    };
    static std::atomic<GType> s_coreVideoMetaApiType{0};
    GType coreVideoMetaApiType = s_coreVideoMetaApiType.load(std::memory_order_acquire);
    if (G_UNLIKELY(coreVideoMetaApiType == 0)) {
        coreVideoMetaApiType = g_type_from_name("GstCoreVideoMetaAPI");
        s_coreVideoMetaApiType.store(coreVideoMetaApiType, std::memory_order_release);
    }
    auto* cvMeta = reinterpret_cast<CoreVideoMetaMirror*>(
        coreVideoMetaApiType ? gst_buffer_get_meta(buffer, coreVideoMetaApiType) : nullptr);
    if (!cvMeta) {
        QGC_HW_WARN_ONCE(GstIOSurfaceLog, s_diag.loggedNoMemory, "mapTextures: GstCoreVideoMeta not found on buffer");
        return GstHwPathTelemetry::fail(HwVideoBufferPath::IOSurface);
    }
    if (!cvMeta->meta.info || cvMeta->meta.info->size != sizeof(CoreVideoMetaMirror)) {
        QGC_HW_WARN_ONCE(GstIOSurfaceLog, s_diag.loggedMetaLayoutMismatch,
                         "mapTextures: GstCoreVideoMeta size"
                             << (cvMeta->meta.info ? cvMeta->meta.info->size : 0) << "!= mirrored layout size"
                             << sizeof(CoreVideoMetaMirror) << "— upstream layout changed");
        return GstHwPathTelemetry::fail(HwVideoBufferPath::IOSurface);
    }

    CVPixelBufferRef pixelBuffer = cvMeta->pixbuf;
    if (!pixelBuffer) {
        QGC_HW_WARN_ONCE(GstIOSurfaceLog, s_diag.loggedPixelBufferExtractFail,
                  "mapTextures: GstCoreVideoMeta.pixbuf is null");
        return GstHwPathTelemetry::fail(HwVideoBufferPath::IOSurface);
    }
    CFRetain(pixelBuffer);

    IOSurfaceRef ioSurface = CVPixelBufferGetIOSurface(pixelBuffer);
    if (!ioSurface) {
        CFRelease(pixelBuffer);
        QGC_HW_WARN_ONCE(GstIOSurfaceLog, s_diag.loggedNoIOSurface, "mapTextures: CVPixelBufferGetIOSurface returned null "
                                       "(non-IOSurface-backed CVPixelBuffer?)");
        return GstHwPathTelemetry::fail(HwVideoBufferPath::IOSurface);
    }

    const auto *nh = static_cast<const QRhiMetalNativeHandles*>(rhi.nativeHandles());
    if (!nh || !nh->dev) {
        CFRelease(pixelBuffer);
        QGC_HW_WARN_ONCE(GstIOSurfaceLog, s_diag.loggedNoMtlDevice, "mapTextures: QRhiMetalNativeHandles missing MTLDevice");
        return GstHwPathTelemetry::fail(HwVideoBufferPath::IOSurface);
    }
    id<MTLDevice> device = (__bridge id<MTLDevice>)nh->dev;

    const int gstPlaneCount = int(GST_VIDEO_INFO_N_PLANES(&_videoInfo));
    const int ioSurfacePlaneCount = [&]() -> int {
        size_t n = IOSurfaceGetPlaneCount(ioSurface);
        return n == 0 ? 1 : int(n); // non-planar IOSurfaces report 0 planes
    }();
    if (gstPlaneCount != ioSurfacePlaneCount) {
        static std::atomic<bool> s_warnedPlaneMismatch{false};
        QGC_HW_WARN_ONCE(GstIOSurfaceLog, s_warnedPlaneMismatch, "GstVideoInfo plane count" << gstPlaneCount
                                         << "differs from IOSurface plane count" << ioSurfacePlaneCount
                                         << "— using minimum");
    }
    const int planeCount = (std::min)({gstPlaneCount, ioSurfacePlaneCount, kMaxPlanes});
    CVMetalTextureCacheRef cache = nullptr;
    {
        QMutexLocker lock(&s_cacheMutex);
        cache = ensureCacheLocked(device);
        // Flush stale entries from prior frames so the cache can release MTLTextures whose
        // CVPixelBuffers have been recycled. Apple's docs recommend a flush-per-frame.
        if (cache) CVMetalTextureCacheFlush(cache, 0);
    }
    if (!cache) {
        CFRelease(pixelBuffer);
        return GstHwPathTelemetry::fail(HwVideoBufferPath::IOSurface);
    }

    std::array<quint64, kMaxPlanes> handles{};
    std::array<CVMetalTextureRef, kMaxPlanes> cvtex{};
    auto cleanupAndFail = [&](int upto) {
        for (int j = 0; j < upto; ++j) {
            if (cvtex[j]) { CFRelease(cvtex[j]); cvtex[j] = nullptr; }
        }
        CFRelease(pixelBuffer);
        return GstHwPathTelemetry::fail(HwVideoBufferPath::IOSurface);
    };
    for (int i = 0; i < planeCount; ++i) {
        const MTLPixelFormat fmt = metalPixelFormatForPlane(_format.pixelFormat(), i);
        if (fmt == MTLPixelFormatInvalid) {
            QGC_HW_WARN_ONCE(GstIOSurfaceLog, s_diag.loggedUnsupportedFormat,
                      "mapTextures: unsupported pixel format" << int(_format.pixelFormat())
                      << "for plane" << i);
            return cleanupAndFail(i);
        }
        // IOSurface plane getters work for both planar and non-planar (plane=0 returns full dims),
        // unlike CVPixelBufferGetWidthOfPlane which is undefined on non-planar buffers.
        const NSUInteger w = IOSurfaceGetWidthOfPlane(ioSurface, i);
        const NSUInteger h = IOSurfaceGetHeightOfPlane(ioSurface, i);
        CVMetalTextureRef ct = nullptr;
        const CVReturn r = CVMetalTextureCacheCreateTextureFromImage(
            kCFAllocatorDefault, cache, pixelBuffer, nullptr,
            fmt, w, h, NSUInteger(i), &ct);
        if (r != kCVReturnSuccess || !ct) {
            QGC_HW_WARN_ONCE(GstIOSurfaceLog, s_diag.loggedTextureCreateFail,
                      "mapTextures: CVMetalTextureCacheCreateTextureFromImage failed for plane"
                      << i << "(format=" << int(fmt) << "size=" << QSize(w, h)
                      << "CVReturn=" << int(r) << ")");
            return cleanupAndFail(i);
        }
        id<MTLTexture> t = CVMetalTextureGetTexture(ct);
        if (!t) {
            CFRelease(ct);
            QGC_HW_WARN_ONCE(GstIOSurfaceLog, s_diag.loggedTextureCreateFail,
                      "mapTextures: CVMetalTextureGetTexture returned nil for plane" << i);
            return cleanupAndFail(i);
        }
        cvtex[i] = ct;
        handles[i] = reinterpret_cast<quint64>(static_cast<void*>(t));
    }

    CFRelease(pixelBuffer);

    // No preflight: the CVMetalTextureRefs above are already created, and only the bundle releases them.
    const GstHwMapTail tail{.path = HwVideoBufferPath::IOSurface,
                            .frameSize = _format.frameSize(),
                            .pixelFormat = _format.pixelFormat(),
                            .log = &GstIOSurfaceLog(),
                            .diag = &s_diag,
                            .preflight = false,
                            .successTag = "IOSurface"};
    const std::span<const quint64> planes(handles.data(), std::size_t(planeCount));
    // VideoToolbox recycles CVPixelBuffer/IOSurface across frames and the CVMetalTextureCache returns the same
    // MTLTexture for a repeated IOSurface, so a bundle over the same textures samples the new frame unchanged.
    return GstHwFrameTexturesBase::finishMap<FrameTextures>(
        *this, rhi, old, tail,
        [&](FrameTextures& prev) {
            if (!prev.matchesImport(&rhi, tail.frameSize, tail.pixelFormat, planes)) {
                return false;
            }
            prev.adoptCvRefs(cvtex);
            return true;
        },
        [&] { return std::make_unique<FrameTextures>(&rhi, tail.frameSize, tail.pixelFormat, planes, cvtex); });
}

// gst-applemedia exports no public gst_is_apple_core_video_memory() predicate, so match the allocator's memory type.
constinit const HwPathDescriptor GstHwPaths::kIOSurface = {
    .path = HwVideoBufferPath::IOSurface,
    .matches = [](GstMemory* mem, const HwVideoBufferContext&) -> bool {
        return mem->allocator && (g_strcmp0(mem->allocator->mem_type, "AppleCoreVideoMemory") == 0);
    },
    .construct = &GstHwPaths::construct<GstIOSurfaceVideoBuffer>,
    .resetCaches = &GstIOSurfaceVideoBuffer::resetTextureCache,
};

#endif // (Q_OS_MACOS || Q_OS_IOS) && QGC_HAS_GST_IOSURFACE_GPU_PATH
