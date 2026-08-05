#include "GstIOSurfaceVideoBuffer.h"
#include "GstContextBridgeRegistry.h"
#include "GstHwVideoBuffer.h"
#include "GstHwFrameTexturesBase.h"
#include "GstHwPathTelemetry.h"

#if (defined(Q_OS_MACOS) || defined(Q_OS_IOS)) && defined(QGC_HAS_GST_IOSURFACE_GPU_PATH)

#include "QGCLoggingCategory.h"

#include <QtCore/QLoggingCategory>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QSize>
#include <private/qvideotexturehelper_p.h>
#include <rhi/qrhi.h>

#include <gst/video/video.h>
#if defined(__has_include) && __has_include(<gst/applemedia/coremediabuffer.h>)
#define QGC_HAS_PUBLIC_CORE_VIDEO_META 1
#include <gst/applemedia/coremediabuffer.h>
#endif

#include <CoreVideo/CoreVideo.h>
#include <CoreVideo/CVMetalTexture.h>
#include <CoreVideo/CVMetalTextureCache.h>
// IOSurfaceRef.h is the public C API on both macOS and iOS; the umbrella IOSurface/IOSurface.h is macOS-only.
#include <IOSurface/IOSurfaceRef.h>
#import <Metal/Metal.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>

QGC_LOGGING_CATEGORY(GstIOSurfaceLog, "Video.GStreamer.HwBuffers.GstIOSurface")

namespace {

using GstHw::kMaxPlanes;

// One-shot per failure cause; teardown emits the running counter. Shared causes come from
// GstHw::MapDiagnostics; the remainder are IOSurface/Metal-specific.
struct IOSurfaceDiagnostics : GstHw::MapDiagnostics
{
    std::atomic<bool> loggedNoMemory{false};
    std::atomic<bool> loggedPixelBufferExtractFail{false};
    std::atomic<bool> loggedNoIOSurface{false};
    std::atomic<bool> loggedNoMtlDevice{false};
    std::atomic<bool> loggedUnsupportedFormat{false};
    std::atomic<bool> loggedRhiCreateFromFail{false};
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
    FrameTextures(QRhi *rhi, QSize size, QVideoFrameFormat::PixelFormat pixelFormat,
                  std::array<id<MTLTexture>, kMaxPlanes> mtex,
                  std::array<CVMetalTextureRef, kMaxPlanes> cvtex, int count)
        : _rhi(rhi)
        , _size(size)
        , _pixelFormat(pixelFormat)
        , _mtex(mtex)
        , _cvtex(cvtex)
    {
        _count = count;
        const auto *desc = QVideoTextureHelper::textureDescription(pixelFormat);
        if (!desc) {
            QGC_HW_WARN_ONCE(GstIOSurfaceLog, s_diag.loggedTextureCreateFail,
                      "FrameTextures: QVideoTextureHelper has no description for format" << int(pixelFormat));
            return;
        }
        for (int i = 0; i < _count; ++i) {
            const QSize planeSize = desc->rhiPlaneSize(size, i, rhi);
            _textures[i].reset(rhi->newTexture(desc->rhiTextureFormat(i, rhi), planeSize, 1, {}));
            const quint64 handle = reinterpret_cast<quint64>(static_cast<void*>(mtex[i]));
            if (_textures[i] && !_textures[i]->createFrom({handle, 0})) {
                QGC_HW_WARN_ONCE(GstIOSurfaceLog, s_diag.loggedRhiCreateFromFail,
                          "FrameTextures: QRhiTexture::createFrom failed for plane" << i
                          << "(planeSize=" << planeSize << ")");
                _textures[i].reset();
            }
        }
    }

    ~FrameTextures() override
    {
        // CVMetalTextureRefs hold the underlying MTLTexture and the CVPixelBuffer's IOSurface
        // refcount; releasing returns them to the cache for reuse. Base dtor clears _srcSample.
        for (int i = 0; i < _count; ++i) {
            if (_cvtex[i]) CFRelease(_cvtex[i]);
            _mtex[i] = nil;
        }
    }

    void onFrameEndInvoked() override
    {
        // Release CVMetalTextureRefs eagerly to return IOSurface pool slots; defer to dtor risks pool pressure.
        for (int i = 0; i < _count; ++i) {
            if (_cvtex[i]) { CFRelease(_cvtex[i]); _cvtex[i] = nullptr; }
        }
        GstHwFrameTexturesBase::onFrameEndInvoked();
    }

    HwVideoBufferPath sourcePath() const override { return HwVideoBufferPath::IOSurface; }

    // Reuse eligibility: identical rhi+size+format and identical, non-nil MTLTexture handles
    // per plane. VideoToolbox recycles CVPixelBuffer/IOSurface across frames; the
    // CVMetalTextureCache returns the same MTLTexture when the underlying IOSurface repeats,
    // so the QRhiTexture wrapper can be reused with new sample data.
    bool matches(QRhi *rhi, QSize size, QVideoFrameFormat::PixelFormat pixelFormat,
                 const std::array<id<MTLTexture>, kMaxPlanes> &mtex, int count) const
    {
        if (_rhi != rhi || _size != size || _pixelFormat != pixelFormat || _count != count) {
            return false;
        }
        for (int i = 0; i < _count; ++i) {
            if (_mtex[i] == nil || _mtex[i] != mtex[i] || !_textures[i]) {
                return false;
            }
        }
        return true;
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
    QRhi *_rhi = nullptr;
    QSize _size;
    QVideoFrameFormat::PixelFormat _pixelFormat = QVideoFrameFormat::Format_Invalid;
    std::array<id<MTLTexture>, kMaxPlanes> _mtex;
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

namespace {
struct IOSurfaceCacheResetRegistrar
{
    IOSurfaceCacheResetRegistrar() { GstContextBridgeRegistry::registerCacheReset(&GstIOSurfaceVideoBuffer::resetTextureCache); }
};
const IOSurfaceCacheResetRegistrar s_ioSurfaceCacheResetRegistrar;
}  // namespace

QVideoFrameTexturesUPtr GstIOSurfaceVideoBuffer::mapTextures(QRhi &rhi, QVideoFrameTexturesUPtr &old)
{
    // Qt's contract: mapTextures runs on the QRhi (render) thread. Bail rather than crash if ever called off-thread.
    if (!rhi.thread()->isCurrentThread()) {
        return GstHwPathTelemetry::fail(HwVideoBufferPath::IOSurface);
    }
    // *** UNTESTED on macOS hardware. CI compiles this path; runtime validation TBD. ***
    // Pulls a CVPixelBufferRef from the GstBuffer (gst-vt path), grabs its
    // IOSurfaceRef, imports each plane via [MTLDevice newTextureWithDescriptor:
    // iosurface:plane:], and wraps each MTLTexture in a QRhiTexture.
    // IOSurface is device-agnostic so no shared-MTLDevice bridge is required.
    GstBuffer *buffer = nullptr;
    if (!checkMapPreconditions(rhi, static_cast<int>(QRhi::Metal), GstIOSurfaceLog(), s_diag, buffer)) {
        return GstHwPathTelemetry::fail(HwVideoBufferPath::IOSurface);
    }

    // Prefer gst-applemedia's public GstCoreVideoMeta when its header is installed (included at file top); otherwise
    // fall back to the hand-mirrored layout (gst-plugins-bad <= 1.28 ships no public applemedia header), verified
    // against upstream coremediabuffer.h: { GstMeta meta; GstBuffer *buf; CVBufferRef cvbuf; CVPixelBufferRef pixbuf; }.
#if defined(QGC_HAS_PUBLIC_CORE_VIDEO_META)
    using _QgcCoreVideoMeta = GstCoreVideoMeta;
#else
    struct _QgcCoreVideoMeta { GstMeta meta; GstBuffer *buf; CVBufferRef cvbuf; CVPixelBufferRef pixbuf; };
    // Punning relies on cvbuf/pixbuf sitting right after the GstMeta+GstBuffer* prefix; a layout drift here would read
    // garbage CFTypeRefs, so fail the build loudly instead.
    static_assert(sizeof(_QgcCoreVideoMeta) == sizeof(GstMeta) + sizeof(GstBuffer *) + 2 * sizeof(CVBufferRef),
                  "GstCoreVideoMeta layout drift — re-check gst-plugins-bad coremediabuffer.h");
    static_assert(offsetof(_QgcCoreVideoMeta, pixbuf) > offsetof(_QgcCoreVideoMeta, cvbuf),
                  "GstCoreVideoMeta pixbuf must follow cvbuf");
#endif
    static std::atomic<GType> s_coreVideoMetaApiType{0};
    GType coreVideoMetaApiType = s_coreVideoMetaApiType.load(std::memory_order_acquire);
    if (G_UNLIKELY(coreVideoMetaApiType == 0)) {
        coreVideoMetaApiType = g_type_from_name("GstCoreVideoMetaAPI");
        s_coreVideoMetaApiType.store(coreVideoMetaApiType, std::memory_order_release);
    }
    auto *cvMeta = reinterpret_cast<_QgcCoreVideoMeta *>(
        coreVideoMetaApiType ? gst_buffer_get_meta(buffer, coreVideoMetaApiType) : nullptr);
    if (!cvMeta) {
        QGC_HW_WARN_ONCE(GstIOSurfaceLog, s_diag.loggedNoMemory, "mapTextures: GstCoreVideoMeta not found on buffer");
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

    std::array<id<MTLTexture>, kMaxPlanes> mtex{};
    std::array<CVMetalTextureRef, kMaxPlanes> cvtex{};
    auto cleanupAndFail = [&](int upto) {
        for (int j = 0; j < upto; ++j) {
            if (cvtex[j]) { CFRelease(cvtex[j]); cvtex[j] = nullptr; }
            mtex[j] = nil;
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
        mtex[i] = t;
    }

    CFRelease(pixelBuffer);

    if (auto *prev = GstHwFrameTexturesBase::reusableBundle<FrameTextures>(old, HwVideoBufferPath::IOSurface)) {
        if (prev->matches(&rhi, _format.frameSize(), _format.pixelFormat(), mtex, planeCount)) {
            GstHwPathTelemetry::recordTextureReuse(HwVideoBufferPath::IOSurface);
            prev->adoptCvRefs(cvtex);
            prev->setSourceSample(takeSample());
            QVideoFrameTexturesUPtr reused = std::move(old);
            return reused;
        }
    }

    auto textures = std::make_unique<FrameTextures>(&rhi, _format.frameSize(),
                                                    _format.pixelFormat(), mtex, cvtex, planeCount);
    // Per-plane: NV12 chroma can fail while luma succeeds. Returning a partial bundle
    // would render with missing planes and no failure-counter increment.
    for (int i = 0; i < planeCount; ++i) {
        if (!textures->texture(static_cast<uint>(i))) {
            qCWarning(GstIOSurfaceLog) << "createFrom failed for plane" << i
                                       << "format=" << int(_format.pixelFormat());
            return GstHwPathTelemetry::fail(HwVideoBufferPath::IOSurface);
        }
    }

    logFirstSuccess(s_diag.loggedFirstSuccess, GstIOSurfaceLog(), "IOSurface", _format.frameSize(),
                    _format.pixelFormat(), planeCount);
    textures->setSourceSample(takeSample());
    return textures;
}



#endif // (Q_OS_MACOS || Q_OS_IOS) && QGC_HAS_GST_IOSURFACE_GPU_PATH
