#include "GstIOSurfaceVideoBuffer.h"
#include "GstHwVideoBuffer.h"

#if (defined(Q_OS_MACOS) || defined(Q_OS_IOS)) && defined(QGC_HAS_GST_IOSURFACE_GPU_PATH)

#include "QGCLoggingCategory.h"

#include <QtCore/QLoggingCategory>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QSize>
#include <private/qvideotexturehelper_p.h>
#include <rhi/qrhi.h>

#include <gst/video/video.h>

#include <CoreVideo/CoreVideo.h>
#include <CoreVideo/CVMetalTexture.h>
#include <CoreVideo/CVMetalTextureCache.h>
// IOSurfaceRef.h is the public C API on both macOS and iOS; the umbrella IOSurface/IOSurface.h is macOS-only.
#include <IOSurface/IOSurfaceRef.h>
#import <Metal/Metal.h>

#include <algorithm>
#include <array>
#include <atomic>

QGC_LOGGING_CATEGORY(GstIOSurfaceLog, "Video.GStreamer.HwBuffers.GstIOSurface")

namespace {

constexpr int kMaxPlanes = 4;

std::atomic<quint64> s_mapFailureCount{0};
std::atomic<bool> s_loggedFirstSuccess{false};
// One-shot per failure cause; teardown emits the running counter.
std::atomic<bool> s_loggedNullSample{false};
std::atomic<bool> s_loggedBadBackend{false};
std::atomic<bool> s_loggedNullBuffer{false};
std::atomic<bool> s_loggedNoMemory{false};
std::atomic<bool> s_loggedPixelBufferExtractFail{false};
std::atomic<bool> s_loggedNoIOSurface{false};
std::atomic<bool> s_loggedNoMtlDevice{false};
std::atomic<bool> s_loggedUnsupportedFormat{false};
std::atomic<bool> s_loggedTextureCreateFail{false};
std::atomic<bool> s_loggedRhiCreateFromFail{false};
std::atomic<bool> s_loggedCacheCreateFail{false};

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
        if (!s_loggedCacheCreateFail.exchange(true, std::memory_order_relaxed)) {
            qCWarning(GstIOSurfaceLog) << "CVMetalTextureCacheCreate failed (CVReturn=" << int(r) << ")";
        }
        return nullptr;
    }
    s_cache.cache = cache;
    s_cache.deviceKey = key;
    return cache;
}

QVideoFrameTexturesUPtr fail()
{
    s_mapFailureCount.fetch_add(1, std::memory_order_relaxed);
    return {};
}

#define WARN_ONCE(flag, ...) \
    do { if (!(flag).exchange(true, std::memory_order_relaxed)) { \
        qCWarning(GstIOSurfaceLog) << __VA_ARGS__; \
    } } while (0)

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

class FrameTextures final : public QVideoFrameTextures
{
public:
    FrameTextures(QRhi *rhi, QSize size, QVideoFrameFormat::PixelFormat pixelFormat,
                  std::array<id<MTLTexture>, kMaxPlanes> mtex,
                  std::array<CVMetalTextureRef, kMaxPlanes> cvtex, int count)
        : _count(count)
        , _mtex(mtex)
        , _cvtex(cvtex)
    {
        const auto *desc = QVideoTextureHelper::textureDescription(pixelFormat);
        if (!desc) {
            WARN_ONCE(s_loggedTextureCreateFail,
                      "FrameTextures: QVideoTextureHelper has no description for format" << int(pixelFormat));
            return;
        }
        for (int i = 0; i < _count; ++i) {
            const QSize planeSize = desc->rhiPlaneSize(size, i, rhi);
            _textures[i].reset(rhi->newTexture(desc->rhiTextureFormat(i, rhi), planeSize, 1, {}));
            const quint64 handle = reinterpret_cast<quint64>(static_cast<void*>(mtex[i]));
            if (_textures[i] && !_textures[i]->createFrom({handle, 0})) {
                WARN_ONCE(s_loggedRhiCreateFromFail,
                          "FrameTextures: QRhiTexture::createFrom failed for plane" << i
                          << "(planeSize=" << planeSize << ")");
                _textures[i].reset();
            }
        }
    }

    ~FrameTextures() override
    {
        // Release CVMetalTextureRefs first — each holds the underlying MTLTexture and the
        // CVPixelBuffer's IOSurface refcount; releasing returns them to the cache for reuse.
        for (int i = 0; i < _count; ++i) {
            if (_cvtex[i]) CFRelease(_cvtex[i]);
            _mtex[i] = nil;
        }
    }

    QRhiTexture *texture(uint plane) const override
    {
        return (int(plane) < _count) ? _textures[plane].get() : nullptr;
    }

private:
    int _count = 0;
    std::array<id<MTLTexture>, kMaxPlanes> _mtex;
    std::array<CVMetalTextureRef, kMaxPlanes> _cvtex;
    std::unique_ptr<QRhiTexture> _textures[kMaxPlanes];
};

} // namespace

GstIOSurfaceVideoBuffer::GstIOSurfaceVideoBuffer(GstSample *sample,
                                                 const GstVideoInfo &videoInfo,
                                                 const QVideoFrameFormat &format)
    : GstHwVideoBuffer(QVideoFrame::RhiTextureHandle, sample, videoInfo, format)
{
}

GstIOSurfaceVideoBuffer::~GstIOSurfaceVideoBuffer() = default;

QAbstractVideoBuffer::MapData GstIOSurfaceVideoBuffer::map(QVideoFrame::MapMode /*mode*/)
{
    return {};
}

bool GstIOSurfaceVideoBuffer::validatePlaneHandles() const
{
    if (!_sample) return false;
    GstBuffer *buffer = gst_sample_get_buffer(_sample);
    if (!buffer) return false;
    const int memCount = qMin(int(gst_buffer_n_memory(buffer)), kMaxPlanes);
    if (memCount <= 0) return false;
    for (int i = 0; i < memCount; ++i) {
        GstMemory *mem = gst_buffer_peek_memory(buffer, i);
        if (!mem || !mem->allocator) return false;
        if (g_strcmp0(mem->allocator->mem_type, "AppleCoreVideoMemory") != 0) return false;
    }
    return true;
}

QVideoFrameTexturesUPtr GstIOSurfaceVideoBuffer::mapTextures(QRhi &rhi, QVideoFrameTexturesUPtr & /*old*/)
{
    Q_ASSERT(rhi.thread()->isCurrentThread()); // Qt's contract: mapTextures runs on the QRhi (render) thread.
    // *** UNTESTED on macOS hardware. CI compiles this path; runtime validation TBD. ***
    // Pulls a CVPixelBufferRef from the GstBuffer (gst-vt path), grabs its
    // IOSurfaceRef, imports each plane via [MTLDevice newTextureWithDescriptor:
    // iosurface:plane:], and wraps each MTLTexture in a QRhiTexture.
    // IOSurface is device-agnostic so no shared-MTLDevice bridge is required.
    if (!_sample) {
        WARN_ONCE(s_loggedNullSample, "mapTextures: GstSample is null");
        return fail();
    }
    if (rhi.backend() != QRhi::Metal) {
        WARN_ONCE(s_loggedBadBackend,
                  "mapTextures: QRhi backend is" << rhi.backendName() << "(Metal required)");
        return fail();
    }

    GstBuffer *buffer = gst_sample_get_buffer(_sample);
    if (!buffer) {
        WARN_ONCE(s_loggedNullBuffer, "mapTextures: GstSample has no buffer");
        return fail();
    }

    // gst/applemedia/applemedia.h is not installed in gst-plugins-bad 1.28; access the
    // CVPixelBufferRef through GstCoreVideoMeta which vtdec attaches to every output buffer.
    struct _QgcCoreVideoMeta { GstMeta meta; CVBufferRef cvbuf; CVPixelBufferRef pixbuf; };
    static GType s_coreVideoMetaApiType = 0;
    if (G_UNLIKELY(s_coreVideoMetaApiType == 0))
        s_coreVideoMetaApiType = g_type_from_name("GstCoreVideoMetaAPI");
    auto *cvMeta = reinterpret_cast<_QgcCoreVideoMeta *>(
        s_coreVideoMetaApiType ? gst_buffer_get_meta(buffer, s_coreVideoMetaApiType) : nullptr);
    if (!cvMeta) {
        WARN_ONCE(s_loggedNoMemory, "mapTextures: GstCoreVideoMeta not found on buffer");
        return fail();
    }

    CVPixelBufferRef pixelBuffer = cvMeta->pixbuf;
    if (!pixelBuffer) {
        WARN_ONCE(s_loggedPixelBufferExtractFail,
                  "mapTextures: GstCoreVideoMeta.pixbuf is null");
        return fail();
    }
    CFRetain(pixelBuffer);

    IOSurfaceRef ioSurface = CVPixelBufferGetIOSurface(pixelBuffer);
    if (!ioSurface) {
        CFRelease(pixelBuffer);
        WARN_ONCE(s_loggedNoIOSurface, "mapTextures: CVPixelBufferGetIOSurface returned null "
                                       "(non-IOSurface-backed CVPixelBuffer?)");
        return fail();
    }

    const auto *nh = static_cast<const QRhiMetalNativeHandles*>(rhi.nativeHandles());
    if (!nh || !nh->dev) {
        CFRelease(pixelBuffer);
        WARN_ONCE(s_loggedNoMtlDevice, "mapTextures: QRhiMetalNativeHandles missing MTLDevice");
        return fail();
    }
    id<MTLDevice> device = (__bridge id<MTLDevice>)nh->dev;

    const int gstPlaneCount = int(GST_VIDEO_INFO_N_PLANES(&_videoInfo));
    const int ioSurfacePlaneCount = [&]() -> int {
        size_t n = IOSurfaceGetPlaneCount(ioSurface);
        return n == 0 ? 1 : int(n); // non-planar IOSurfaces report 0 planes
    }();
    if (gstPlaneCount != ioSurfacePlaneCount) {
        static bool s_warnedPlaneMismatch = false;
        if (!s_warnedPlaneMismatch) {
            s_warnedPlaneMismatch = true;
            qCWarning(GstIOSurfaceLog) << "GstVideoInfo plane count" << gstPlaneCount
                                       << "differs from IOSurface plane count" << ioSurfacePlaneCount
                                       << "— using minimum";
        }
    }
    const int planeCount = std::min({gstPlaneCount, ioSurfacePlaneCount, kMaxPlanes});
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
        return fail();
    }

    std::array<id<MTLTexture>, kMaxPlanes> mtex{};
    std::array<CVMetalTextureRef, kMaxPlanes> cvtex{};
    auto cleanupAndFail = [&](int upto) {
        for (int j = 0; j < upto; ++j) {
            if (cvtex[j]) { CFRelease(cvtex[j]); cvtex[j] = nullptr; }
            mtex[j] = nil;
        }
        CFRelease(pixelBuffer);
        return fail();
    };
    for (int i = 0; i < planeCount; ++i) {
        const MTLPixelFormat fmt = metalPixelFormatForPlane(_format.pixelFormat(), i);
        if (fmt == MTLPixelFormatInvalid) {
            WARN_ONCE(s_loggedUnsupportedFormat,
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
            WARN_ONCE(s_loggedTextureCreateFail,
                      "mapTextures: CVMetalTextureCacheCreateTextureFromImage failed for plane"
                      << i << "(format=" << int(fmt) << "size=" << QSize(w, h)
                      << "CVReturn=" << int(r) << ")");
            return cleanupAndFail(i);
        }
        id<MTLTexture> t = CVMetalTextureGetTexture(ct);
        if (!t) {
            CFRelease(ct);
            WARN_ONCE(s_loggedTextureCreateFail,
                      "mapTextures: CVMetalTextureGetTexture returned nil for plane" << i);
            return cleanupAndFail(i);
        }
        cvtex[i] = ct;
        mtex[i] = t;
    }

    CFRelease(pixelBuffer);

    auto textures = std::make_unique<FrameTextures>(&rhi, _format.frameSize(),
                                                    _format.pixelFormat(), mtex, cvtex, planeCount);
    // Per-plane: NV12 chroma can fail while luma succeeds. Returning a partial bundle
    // would render with missing planes and no failure-counter increment.
    for (int i = 0; i < planeCount; ++i) {
        if (!textures->texture(static_cast<uint>(i))) {
            qCWarning(GstIOSurfaceLog) << "createFrom failed for plane" << i
                                       << "format=" << int(_format.pixelFormat());
            return fail();
        }
    }

    if (!s_loggedFirstSuccess.exchange(true, std::memory_order_relaxed)) {
        qCInfo(GstIOSurfaceLog) << "First IOSurface zero-copy mapTextures success: size="
                                << _format.frameSize() << "format=" << int(_format.pixelFormat())
                                << "planes=" << planeCount;
    }
    return textures;
}

quint64 GstIOSurfaceVideoBuffer::takeMapFailureCount()
{
    return s_mapFailureCount.exchange(0, std::memory_order_relaxed);
}

quint64 GstIOSurfaceVideoBuffer::peekMapFailureCount()
{
    return s_mapFailureCount.load(std::memory_order_relaxed);
}

#endif // (Q_OS_MACOS || Q_OS_IOS) && QGC_HAS_GST_IOSURFACE_GPU_PATH
