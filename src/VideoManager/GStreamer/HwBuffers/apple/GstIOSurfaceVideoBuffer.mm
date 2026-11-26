#include "GstIOSurfaceVideoBuffer.h"

#include "GstHwPathRegistry.h"
#include "GstHwVideoBuffer.h"

#if (defined(Q_OS_MACOS) || defined(Q_OS_IOS)) && defined(QGC_HAS_GST_IOSURFACE_GPU_PATH)

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <gst/video/video.h>
#include <memory>
#include <rhi/qrhi.h>

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

// Process-wide CVMetalTextureCache, keyed by MTLDevice*. Apple's documented bridge between CVPixelBuffer/IOSurface and
// Metal — reuses MTLTexture objects when the decoder recycles IOSurfaces (typical with VideoToolbox), and avoids
// per-frame MTLTextureDescriptor allocation. Apple's docs guarantee thread-safety. Recreated on QRhi device change
// (window destroy/recreate); resetTextureCache() releases it when the GPU caches are reset.
struct MetalTextureCache
{
    CVMetalTextureCacheRef cache = nullptr;
    void* deviceKey = nullptr;  // raw MTLDevice* for compare; the cache holds its own ref to the device
};

QMutex s_cacheMutex;
MetalTextureCache s_cache;

CVMetalTextureCacheRef ensureCacheLocked(id<MTLDevice> device)
{
    void* key = (__bridge void*) device;
    if (s_cache.cache && s_cache.deviceKey == key) {
        return s_cache.cache;
    }
    if (s_cache.cache) {
        CFRelease(s_cache.cache);
        s_cache.cache = nullptr;
        s_cache.deviceKey = nullptr;
    }
    CVMetalTextureCacheRef cache = nullptr;
    const CVReturn r = CVMetalTextureCacheCreate(kCFAllocatorDefault, nullptr, device, nullptr, &cache);
    if (r != kCVReturnSuccess || !cache) {
        QGC_HW_WARN_ONCE(GstIOSurfaceLog, "CVMetalTextureCacheCreate failed (CVReturn=" << int(r) << ")");
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

/// The frame's MTLTextures, each held by the CVMetalTextureRef it came from, which goes back to the cache with the
/// handles. CVMetalTextureRefs hold the underlying MTLTexture and the CVPixelBuffer's IOSurface refcount.
class IOSurfaceTextureHandles final : public GstHwTextureHandles
{
public:
    using GstHwTextureHandles::GstHwTextureHandles;

    ~IOSurfaceTextureHandles() override
    {
        for (CVMetalTextureRef texture : _cvtex) {
            if (texture) {
                CFRelease(texture);
            }
        }
    }

    /// Plane @p plane's MTLTexture, of @p texture, whose reference the handles take over.
    void adoptPlane(int plane, CVMetalTextureRef texture, id<MTLTexture> metalTexture)
    {
        _cvtex[std::size_t(plane)] = texture;
        setHandle(plane, static_cast<void*>(metalTexture));
    }

private:
    std::array<CVMetalTextureRef, kMaxPlanes> _cvtex{};
};

}  // namespace

bool GstIOSurfaceVideoBuffer::validatePlaneHandles() const
{
    return validatePlanes([](GstMemory* mem) {
        if (!mem || !mem->allocator)
            return false;
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

QVideoFrameTexturesUPtr GstIOSurfaceVideoBuffer::importTextures(QRhi& rhi, QVideoFrameTexturesUPtr& /*old*/)
{
    // Untested on Apple hardware: CI only compiles this path. Takes the CVPixelBufferRef from the buffer's
    // GstCoreVideoMeta (vtdec) and each plane's MTLTexture from the CVMetalTextureCache, which Qt then wraps.
    // IOSurface is device-agnostic so no shared-MTLDevice bridge is required.
    GstBuffer* buffer = nullptr;
    if (!checkMapPreconditions(rhi, static_cast<int>(QRhi::Metal), GstIOSurfaceLog(), buffer)) {
        return {};
    }

    // Mirror of GstCoreVideoMeta from gst-plugins-bad sys/applemedia/corevideobuffer.h (plugin-private, not installed;
    // identical in 1.20-1.28). The registered meta size is checked below before reading past the GstMeta header.
    // GStreamer 1.30 gives the surface without it: gst_iosurface_memory_peek_surface() (gst/allocators/gstiosurface.h).
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
        QGC_HW_WARN_ONCE(GstIOSurfaceLog, "mapTextures: GstCoreVideoMeta not found on buffer");
        return {};
    }
    if (!cvMeta->meta.info || cvMeta->meta.info->size != sizeof(CoreVideoMetaMirror)) {
        QGC_HW_WARN_ONCE(GstIOSurfaceLog, "mapTextures: GstCoreVideoMeta size"
                                              << (cvMeta->meta.info ? cvMeta->meta.info->size : 0)
                                              << "!= mirrored layout size" << sizeof(CoreVideoMetaMirror)
                                              << "— upstream layout changed");
        return {};
    }

    CVPixelBufferRef pixelBuffer = cvMeta->pixbuf;
    if (!pixelBuffer) {
        QGC_HW_WARN_ONCE(GstIOSurfaceLog, "mapTextures: GstCoreVideoMeta.pixbuf is null");
        return {};
    }
    CFRetain(pixelBuffer);

    IOSurfaceRef ioSurface = CVPixelBufferGetIOSurface(pixelBuffer);
    if (!ioSurface) {
        CFRelease(pixelBuffer);
        QGC_HW_WARN_ONCE(GstIOSurfaceLog, "mapTextures: CVPixelBufferGetIOSurface returned null "
                                          "(non-IOSurface-backed CVPixelBuffer?)");
        return {};
    }

    const auto* nh = static_cast<const QRhiMetalNativeHandles*>(rhi.nativeHandles());
    if (!nh || !nh->dev) {
        CFRelease(pixelBuffer);
        QGC_HW_WARN_ONCE(GstIOSurfaceLog, "mapTextures: QRhiMetalNativeHandles missing MTLDevice");
        return {};
    }
    id<MTLDevice> device = (__bridge id<MTLDevice>) nh->dev;

    const int gstPlaneCount = int(GST_VIDEO_INFO_N_PLANES(&_videoInfo));
    const int ioSurfacePlaneCount = [&]() -> int {
        size_t n = IOSurfaceGetPlaneCount(ioSurface);
        return n == 0 ? 1 : int(n);  // non-planar IOSurfaces report 0 planes
    }();
    if (gstPlaneCount != ioSurfacePlaneCount) {
        QGC_HW_WARN_ONCE(GstIOSurfaceLog, "GstVideoInfo plane count" << gstPlaneCount
                                                                     << "differs from IOSurface plane count"
                                                                     << ioSurfacePlaneCount << "— using minimum");
    }
    const int planeCount = (std::min) ({gstPlaneCount, ioSurfacePlaneCount, kMaxPlanes});
    CVMetalTextureCacheRef cache = nullptr;
    {
        QMutexLocker lock(&s_cacheMutex);
        cache = ensureCacheLocked(device);
        // Flush stale entries from prior frames so the cache can release MTLTextures whose
        // CVPixelBuffers have been recycled. Apple's docs recommend a flush-per-frame.
        if (cache)
            CVMetalTextureCacheFlush(cache, 0);
    }
    if (!cache) {
        CFRelease(pixelBuffer);
        return {};
    }

    // Each plane's CVMetalTextureRef is the handles' as soon as it is created, so a failure releases what came before.
    auto handles = std::make_unique<IOSurfaceTextureHandles>(refSample());
    for (int i = 0; i < planeCount; ++i) {
        const MTLPixelFormat fmt = metalPixelFormatForPlane(_format.pixelFormat(), i);
        if (fmt == MTLPixelFormatInvalid) {
            QGC_HW_WARN_ONCE(GstIOSurfaceLog,
                             "mapTextures: unsupported pixel format" << int(_format.pixelFormat()) << "for plane" << i);
            CFRelease(pixelBuffer);
            return {};
        }
        // IOSurface plane getters work for both planar and non-planar (plane=0 returns full dims),
        // unlike CVPixelBufferGetWidthOfPlane which is undefined on non-planar buffers.
        const NSUInteger w = IOSurfaceGetWidthOfPlane(ioSurface, i);
        const NSUInteger h = IOSurfaceGetHeightOfPlane(ioSurface, i);
        CVMetalTextureRef ct = nullptr;
        const CVReturn r = CVMetalTextureCacheCreateTextureFromImage(kCFAllocatorDefault, cache, pixelBuffer, nullptr,
                                                                     fmt, w, h, NSUInteger(i), &ct);
        if (r != kCVReturnSuccess || !ct) {
            QGC_HW_WARN_ONCE(GstIOSurfaceLog, "mapTextures: CVMetalTextureCacheCreateTextureFromImage failed for plane"
                                                  << i << "(format=" << int(fmt) << "size=" << QSize(w, h)
                                                  << "CVReturn=" << int(r) << ")");
            CFRelease(pixelBuffer);
            return {};
        }
        id<MTLTexture> t = CVMetalTextureGetTexture(ct);
        if (!t) {
            CFRelease(ct);
            CFRelease(pixelBuffer);
            QGC_HW_WARN_ONCE(GstIOSurfaceLog, "mapTextures: CVMetalTextureGetTexture returned nil for plane" << i);
            return {};
        }
        handles->adoptPlane(i, ct, t);
    }

    CFRelease(pixelBuffer);

    return wrapTextures(rhi, std::move(handles), GstIOSurfaceLog(), "IOSurface");
}

// gst-applemedia exports no public gst_is_apple_core_video_memory() predicate, so match the allocator's memory type.
constinit const HwPathDescriptor GstHwPaths::kIOSurface = {
    .path = HwVideoBufferPath::IOSurface,
    .matches = [](GstMemory* mem) -> bool {
        return mem->allocator && (g_strcmp0(mem->allocator->mem_type, "AppleCoreVideoMemory") == 0);
    },
    .construct = &GstHwPaths::construct<GstIOSurfaceVideoBuffer>,
    .resetCaches = &GstIOSurfaceVideoBuffer::resetTextureCache,
};

#endif  // (Q_OS_MACOS || Q_OS_IOS) && QGC_HAS_GST_IOSURFACE_GPU_PATH
