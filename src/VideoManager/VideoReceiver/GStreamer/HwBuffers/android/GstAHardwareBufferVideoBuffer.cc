#include "GstAHardwareBufferVideoBuffer.h"

#if defined(QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH)

#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <QtCore/QLoggingCategory>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QScopeGuard>
#include <QtCore/QSize>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>
#include <android/hardware_buffer.h>
#include <array>
#include <atomic>
#include <vector>
#include <gst/android/gstandroid.h>
#include <gst/gl/gstglsyncmeta.h>
#include <gst/video/video.h>
#include <limits>
#include <private/qvideotexturehelper_p.h>
#include <rhi/qrhi.h>
#include <rhi/qrhi_platform.h>

#include "GstContextBridgeRegistry.h"
#include "GstEglHelpers.h"
#include "GstHwFrameTexturesBase.h"
#include "GstHwImportPreflight.h"
#include "GstHwPathTelemetry.h"
#include "GstHwVideoBuffer.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GstAHWBufLog, "Video.GStreamer.HwBuffers.GstAHWBuf")

namespace {

using GstHw::kMaxPlanes;
constexpr int kImageCacheCapacity = 4;

std::atomic<bool> s_loggedBadBackend{false};

constexpr const char* kNativeBufferExt = "EGL_ANDROID_image_native_buffer";

std::atomic<bool> s_loggedNoFenceSync{false};

// LRU cache (AHardwareBuffer*, EGLDisplay) -> (EGLImage, GL texture): MediaCodec recycles the same AHB so the import
// stays valid, saving ~50-200 us/frame on Mali/Adreno.
struct AhbCacheEntry
{
    QRhi* rhi = nullptr;
    AHardwareBuffer* ahb = nullptr;
    EGLDisplay display = EGL_NO_DISPLAY;
    EGLImageKHR image = EGL_NO_IMAGE_KHR;
    GLuint textureName = 0;
    quint64 lastUsedTick = 0;
    int inUse = 0;  // live FrameTextures wrappers referencing this name; never evict while > 0
};

// Guards s_imageCache, s_deferredImageCache, and s_imageCacheTick.
QMutex s_imageCacheMutex;
std::array<AhbCacheEntry, kImageCacheCapacity> s_imageCache{};
std::vector<AhbCacheEntry> s_deferredImageCache;
quint64 s_imageCacheTick = 0;

// Set by resetImageCache() from any thread; teardown deferred to the next mapTextures() since glDeleteTextures
// off-thread leaks and deleting a live-wrapped name is a UAF.
std::atomic<bool> s_imageCacheResetPending{false};

// Render-thread-only; callers must hold s_imageCacheMutex AND have Qt's GL context current.
void destroyCacheEntryLocked(AhbCacheEntry& e, PFNEGLDESTROYIMAGEKHRPROC destroyImage, QOpenGLFunctions* gl)
{
    if (e.image != EGL_NO_IMAGE_KHR && e.display != EGL_NO_DISPLAY && destroyImage) {
        destroyImage(e.display, e.image);
    }
    if (e.textureName != 0 && gl) {
        gl->glDeleteTextures(1, &e.textureName);
    }
    if (e.ahb) {
        AHardwareBuffer_release(e.ahb);
    }
    e = AhbCacheEntry{};
}

void drainDeferredImageCacheLocked(PFNEGLDESTROYIMAGEKHRPROC destroyImage, QOpenGLFunctions* gl, QRhi* rhi = nullptr)
{
    if (!destroyImage || !gl)
        return;

    for (auto it = s_deferredImageCache.begin(); it != s_deferredImageCache.end();) {
        if (it->inUse > 0 || (rhi && it->rhi != rhi)) {
            ++it;
            continue;
        }

        destroyCacheEntryLocked(*it, destroyImage, gl);
        it = s_deferredImageCache.erase(it);
    }
}

GLuint cachedTextureNameLocked(QRhi* rhi, AHardwareBuffer* ahb, EGLDisplay eglDpy)
{
    for (std::size_t i = 0; i < s_imageCache.size(); ++i) {
        auto& e = s_imageCache[i];
        if (e.rhi == rhi && e.ahb == ahb && e.display == eglDpy && e.textureName != 0) {
            e.lastUsedTick = ++s_imageCacheTick;
            return e.textureName;
        }
    }
    return 0;
}

// Insert into cache (evicts LRU when full), taking ownership of image/textureName; caller holds s_imageCacheMutex with
// GL current. False (no ownership taken) when every entry is pinned in-use.
bool insertCacheEntryLocked(QRhi* rhi, AHardwareBuffer* ahb, EGLDisplay eglDpy, EGLImageKHR image, GLuint textureName,
                            PFNEGLDESTROYIMAGEKHRPROC destroyImage, QOpenGLFunctions* gl)
{
    AhbCacheEntry* target = nullptr;
    for (std::size_t i = 0; i < s_imageCache.size(); ++i) {
        auto& e = s_imageCache[i];
        if (e.textureName == 0) {
            target = &e;
            break;
        }
    }
    if (!target) {
        // Evict the LRU entry that no live frame still references; in-use names would UAF on render.
        for (std::size_t i = 0; i < s_imageCache.size(); ++i) {
            auto& e = s_imageCache[i];
            if (e.inUse == 0 && (!target || e.lastUsedTick < target->lastUsedTick))
                target = &e;
        }
        if (!target) {
            qCWarning(GstAHWBufLog) << "AHB texture cache full and all entries in use; skipping cache insert";
            return false;
        }
        destroyCacheEntryLocked(*target, destroyImage, gl);
    }
    AHardwareBuffer_acquire(ahb);
    target->rhi = rhi;
    target->ahb = ahb;
    target->display = eglDpy;
    target->image = image;
    target->textureName = textureName;
    target->lastUsedTick = ++s_imageCacheTick;
    return true;
}

void clearImageCacheLocked(PFNEGLDESTROYIMAGEKHRPROC destroyImage, QOpenGLFunctions* gl)
{
    drainDeferredImageCacheLocked(destroyImage, gl);

    for (std::size_t i = 0; i < s_imageCache.size(); ++i) {
        auto& entry = s_imageCache[i];
        if (entry.inUse > 0) {
            // Live FrameTextures still holds this name; destroy when its pin is released.
            qCDebug(GstAHWBufLog) << "AHB texture cache reset deferred for in-use texture" << entry.textureName;
            s_deferredImageCache.push_back(entry);
            entry = AhbCacheEntry{};
            continue;
        }
        destroyCacheEntryLocked(entry, destroyImage, gl);
    }
    drainDeferredImageCacheLocked(destroyImage, gl);
    s_imageCacheTick = 0;
}

// Pin a cache entry by texture name so LRU eviction/reset can't free a name a live frame holds; caller holds
// s_imageCacheMutex. Unmatched names are no-ops.
void acquireCacheEntryLocked(GLuint name)
{
    if (name == 0)
        return;
    for (std::size_t i = 0; i < s_imageCache.size(); ++i) {
        auto& e = s_imageCache[i];
        if (e.textureName == name) {
            ++e.inUse;
            return;
        }
    }
    for (auto& e : s_deferredImageCache) {
        if (e.textureName == name) {
            ++e.inUse;
            return;
        }
    }
}

void releaseCacheEntryLocked(GLuint name)
{
    if (name == 0)
        return;
    for (std::size_t i = 0; i < s_imageCache.size(); ++i) {
        auto& e = s_imageCache[i];
        if (e.textureName == name && e.inUse > 0) {
            --e.inUse;
            return;
        }
    }
    for (auto& e : s_deferredImageCache) {
        if (e.textureName == name && e.inUse > 0) {
            --e.inUse;
            return;
        }
    }
}

class FrameTextures final : public GstHwFrameTexturesBase
{
public:
    // Always single-plane external-OES; the GL texture name is owned by the process-wide AhbCache, not this object
    // (shared across frames on AHB recycle).
    FrameTextures(QRhi* rhi, QSize size, QVideoFrameFormat::PixelFormat pixelFormat, GLuint name)
        : _rhi(rhi), _size(size), _pixelFormat(pixelFormat), _name(name)
    {
        _count = 1;
        const auto* desc = QVideoTextureHelper::textureDescription(pixelFormat);
        if (!desc) {
            qCWarning(GstAHWBufLog) << "no QVideoTextureHelper description for format" << pixelFormat;
            return;
        }
        const QSize planeSize = desc->rhiPlaneSize(size, 0, rhi);
        // ExternalOES: bind GL_TEXTURE_EXTERNAL_OES + emit SamplerExternalOES, else OES_EGL_image_external samples
        // black on GLES2.
        _textures[0].reset(rhi->newTexture(
            desc->rhiTextureFormat(0, rhi, QVideoTextureHelper::TextureDescription::FallbackPolicy::Disable), planeSize,
            1, QRhiTexture::ExternalOES));
        if (_textures[0] && !_textures[0]->createFrom({name, 0})) {
            qCWarning(GstAHWBufLog) << "QRhiTexture::createFrom failed for AHardwareBuffer plane 0";
            _textures[0].reset();
        }
        QMutexLocker lock(&s_imageCacheMutex);
        acquireCacheEntryLocked(_name);
    }

    ~FrameTextures() override
    {
        QOpenGLFunctions* gl = nullptr;
        if (_rhi) {
            _rhi->makeThreadLocalNativeContextCurrent();
            if (QOpenGLContext* ctx = QOpenGLContext::currentContext()) {
                gl = ctx->functions();
            }
        }
        static const auto eglDestroyImageKHR_ =
            reinterpret_cast<PFNEGLDESTROYIMAGEKHRPROC>(eglGetProcAddress("eglDestroyImageKHR"));

        QMutexLocker lock(&s_imageCacheMutex);
        releaseCacheEntryLocked(_name);
        drainDeferredImageCacheLocked(eglDestroyImageKHR_, gl, _rhi);
    }

    HwVideoBufferPath sourcePath() const override { return HwVideoBufferPath::AHardwareBuffer; }

    // Reuse eligibility: same rhi+size+format and identical GL texture name (AhbCache returns the same name on AHB
    // recycle).
    bool matches(QRhi* rhi, QSize size, QVideoFrameFormat::PixelFormat pixelFormat, GLuint name) const noexcept
    {
        return _rhi == rhi && _size == size && _pixelFormat == pixelFormat && _name == name && _name != 0 &&
               _textures[0];
    }

private:
    QRhi* _rhi = nullptr;
    QSize _size;
    QVideoFrameFormat::PixelFormat _pixelFormat = QVideoFrameFormat::Format_Invalid;
    GLuint _name = 0;
};

// MediaCodec writes the AHB asynchronously; without a producer-side wait the EGLImage import can sample a half-written,
// recycled buffer (tearing). Only the buffer's GstGLSyncMeta is a real producer fence — it carries the decoder's
// GPU-completion point. A consumer-side EGL fence (glFlush + eglCreateSync in the GL context) cannot observe
// MediaCodec's async write, so it would synchronise nothing while masking the hazard; we deliberately do not use one.
// When no sync meta is present, warn once and proceed (no bogus barrier).
void waitProducerComplete(GstBuffer* buffer)
{
    if (GstGLSyncMeta* syncMeta = buffer ? gst_buffer_get_gl_sync_meta(buffer) : nullptr) {
        if (GstGLContext* glObj = syncMeta->context) {
            gst_gl_sync_meta_wait(syncMeta, glObj);
            return;
        }
    }

    QGC_HW_WARN_ONCE(GstAHWBufLog, s_loggedNoFenceSync,
                     "AHardwareBuffer: no GstGLSyncMeta producer fence; frames may tear on recycle (no consumer-side "
                     "barrier can substitute)");
}

}  // namespace

GstAHardwareBufferVideoBuffer::GstAHardwareBufferVideoBuffer(GstSample* sample, const GstVideoInfo& videoInfo,
                                                             const QVideoFrameFormat& format, EGLDisplay eglDisplay)
    : GstHwVideoBuffer(QVideoFrame::RhiTextureHandle, sample, videoInfo, format), _eglDisplay(eglDisplay)
{}

// The AHB is imported directly as an EGLImage (no SurfaceTexture::updateTexImage()), so no per-buffer
// getTransformMatrix() exists here; external-OES sampling of a directly-imported buffer is top-left origin while GL
// texcoords are bottom-left, so apply the static Y-flip Qt's SurfaceTexture path also applies (flipV, row-major).
QMatrix4x4 GstAHardwareBufferVideoBuffer::externalTextureMatrix() const
{
    // GStreamer exposes no per-buffer decoder transform at this layer, so we apply the static external-OES Y-flip
    // (top-left vs GL bottom-left origin). If a device ever needs crop-inset/rotation, source it from GstVideoCropMeta.
    static const QMatrix4x4 flipV(1.0f, 0.0f, 0.0f, 0.0f,
                                  0.0f, -1.0f, 0.0f, 1.0f,
                                  0.0f, 0.0f, 1.0f, 0.0f,
                                  0.0f, 0.0f, 0.0f, 1.0f);
    return flipV;
}

bool GstAHardwareBufferVideoBuffer::validatePlaneHandles() const
{
    // Qt 6.10-6.12 has the right ExternalOES rendering path, but upstream GStreamer still does not expose this
    // AHardwareBuffer memory API. If a future GStreamer/Qt release grows an official API, replace these vendor hooks.
    return validatePlanes([](GstMemory* mem) {
        if (!mem || !gst_is_ahardware_buffer_memory(mem))
            return false;
        return gst_ahardware_buffer_memory_get_buffer(GST_AHARDWARE_BUFFER_MEMORY_CAST(mem)) != nullptr;
    });
}

// Teardown deferred to the render thread at a frame boundary — glDeleteTextures off-thread is unsafe.
void GstAHardwareBufferVideoBuffer::resetImageCache() noexcept
{
    s_imageCacheResetPending.store(true, std::memory_order_release);
}

namespace {
struct AhbCacheResetRegistrar
{
    AhbCacheResetRegistrar() { GstContextBridgeRegistry::registerCacheReset(&GstAHardwareBufferVideoBuffer::resetImageCache); }
};
const AhbCacheResetRegistrar s_ahbCacheResetRegistrar;
}  // namespace

QVideoFrameTexturesUPtr GstAHardwareBufferVideoBuffer::mapTextures(QRhi& rhi, QVideoFrameTexturesUPtr& old)
{
    // Qt's contract: mapTextures runs on the QRhi (render) thread. Bail rather than crash if ever called off-thread.
    if (!rhi.thread()->isCurrentThread()) {
        return GstHwPathTelemetry::fail(HwVideoBufferPath::AHardwareBuffer);
    }

    if (!_sample) {
        return GstHwPathTelemetry::fail(HwVideoBufferPath::AHardwareBuffer);
    }
    if (rhi.backend() != QRhi::OpenGLES2) {
        if (!s_loggedBadBackend.exchange(true, std::memory_order_relaxed)) {
            qCWarning(GstAHWBufLog) << "QRhi backend is not OpenGLES2; AHardwareBuffer path unsupported";
        }
        return GstHwPathTelemetry::fail(HwVideoBufferPath::AHardwareBuffer);
    }

    // Bind Qt's GL context before any EGL/GL call, else the calls silently no-op into a foreign/null context.
    rhi.makeThreadLocalNativeContextCurrent();

    const auto* nativeHandles = static_cast<const QRhiGles2NativeHandles*>(rhi.nativeHandles());
    if (!nativeHandles || !nativeHandles->context) {
        qCWarning(GstAHWBufLog) << "QRhi exposes no GL context";
        return GstHwPathTelemetry::fail(HwVideoBufferPath::AHardwareBuffer);
    }

    // Prefer QEGLContext::display() — sampling on a foreign display silently returns black.
    EGLDisplay eglDpy = GstEglHelpers::resolveEglDisplay(nativeHandles->context);
    if (eglDpy == EGL_NO_DISPLAY)
        eglDpy = _eglDisplay;
    if (eglDpy == EGL_NO_DISPLAY) {
        return GstHwPathTelemetry::fail(HwVideoBufferPath::AHardwareBuffer);
    }

    GstBuffer* buffer = gst_sample_get_buffer(_sample);
    if (!buffer)
        return GstHwPathTelemetry::fail(HwVideoBufferPath::AHardwareBuffer);

    GstMemory* mem0 = gst_buffer_peek_memory(buffer, 0);
    if (!mem0 || !gst_is_ahardware_buffer_memory(mem0)) {
        return GstHwPathTelemetry::fail(HwVideoBufferPath::AHardwareBuffer);
    }

    if (!GstEglHelpers::displaySupportsExtension(eglDpy, kNativeBufferExt)) {
        static std::atomic<bool> s_warnedNativeBufferExt{false};
        if (!s_warnedNativeBufferExt.exchange(true, std::memory_order_relaxed)) {
            qCWarning(GstAHWBufLog) << "EGL_ANDROID_image_native_buffer unavailable; AHardwareBuffer path disabled";
        }
        return GstHwPathTelemetry::fail(HwVideoBufferPath::AHardwareBuffer);
    }

    static const auto eglGetNativeClientBufferANDROID_ =
        reinterpret_cast<PFNEGLGETNATIVECLIENTBUFFERANDROIDPROC>(eglGetProcAddress("eglGetNativeClientBufferANDROID"));
    static const auto eglCreateImageKHR_ =
        reinterpret_cast<PFNEGLCREATEIMAGEKHRPROC>(eglGetProcAddress("eglCreateImageKHR"));
    static const auto eglDestroyImageKHR_ =
        reinterpret_cast<PFNEGLDESTROYIMAGEKHRPROC>(eglGetProcAddress("eglDestroyImageKHR"));
    static const auto glEGLImageTargetTexture2DOES_ =
        reinterpret_cast<PFNGLEGLIMAGETARGETTEXTURE2DOESPROC>(eglGetProcAddress("glEGLImageTargetTexture2DOES"));

    if (!eglGetNativeClientBufferANDROID_ || !eglCreateImageKHR_ || !eglDestroyImageKHR_ ||
        !glEGLImageTargetTexture2DOES_) {
        qCWarning(GstAHWBufLog) << "Required EGL/GL proc addresses unavailable";
        return GstHwPathTelemetry::fail(HwVideoBufferPath::AHardwareBuffer);
    }

    AHardwareBuffer* ahwb = gst_ahardware_buffer_memory_get_buffer(GST_AHARDWARE_BUFFER_MEMORY_CAST(mem0));
    if (!ahwb) {
        qCWarning(GstAHWBufLog) << "gst_ahardware_buffer_memory_get_buffer returned null";
        return GstHwPathTelemetry::fail(HwVideoBufferPath::AHardwareBuffer);
    }

    // Block until the producer's GPU write completes before importing/sampling the recycled AHB.
    waitProducerComplete(buffer);

    QOpenGLFunctions functions(nativeHandles->context);

    // Clearing forces a lookup miss so the prior `old` textures aren't reused over freed GL names.
    {
        QMutexLocker lock(&s_imageCacheMutex);
        if (s_imageCacheResetPending.exchange(false, std::memory_order_acq_rel)) {
            clearImageCacheLocked(eglDestroyImageKHR_, &functions);
        }
    }

    GLuint name = 0;
    {
        QMutexLocker lock(&s_imageCacheMutex);
        name = cachedTextureNameLocked(&rhi, ahwb, eglDpy);
        if (name != 0) {
            acquireCacheEntryLocked(name);  // provisional pin: hold the entry across the unlocked gap below
            GstHwPathTelemetry::recordImageCacheHit(HwVideoBufferPath::AHardwareBuffer);
        }
    }

    if (name == 0) {
        GstHwPathTelemetry::recordImageCacheMiss(HwVideoBufferPath::AHardwareBuffer);

        EGLClientBuffer clientBuffer = eglGetNativeClientBufferANDROID_(ahwb);
        if (!clientBuffer) {
            qCWarning(GstAHWBufLog) << "eglGetNativeClientBufferANDROID returned null";
            return GstHwPathTelemetry::fail(HwVideoBufferPath::AHardwareBuffer);
        }

        const EGLint attribs[] = {EGL_NONE};
        EGLImageKHR image =
            eglCreateImageKHR_(eglDpy, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID, clientBuffer, attribs);
        if (image == EGL_NO_IMAGE_KHR) {
            qCWarning(GstAHWBufLog) << "eglCreateImageKHR failed, err=" << Qt::hex << eglGetError();
            return GstHwPathTelemetry::fail(HwVideoBufferPath::AHardwareBuffer);
        }

        functions.glGenTextures(1, &name);
        functions.glBindTexture(GL_TEXTURE_EXTERNAL_OES, name);
        glEGLImageTargetTexture2DOES_(GL_TEXTURE_EXTERNAL_OES, image);
        if (Q_UNLIKELY(GstAHWBufLog().isDebugEnabled())) {
            if (const GLenum glErr = functions.glGetError(); glErr != GL_NO_ERROR) {
                qCDebug(GstAHWBufLog) << "glEGLImageTargetTexture2DOES glError=" << Qt::hex << glErr
                                      << "eglError=" << eglGetError();
            }
        }

        QMutexLocker lock(&s_imageCacheMutex);
        // Real race: s_imageCache is process-wide and each window's render thread can import the same recycled ahb, so
        // re-check and reuse theirs if present.
        if (GLuint existing = cachedTextureNameLocked(&rhi, ahwb, eglDpy); existing != 0) {
            eglDestroyImageKHR_(eglDpy, image);
            functions.glDeleteTextures(1, &name);
            name = existing;
        } else if (!insertCacheEntryLocked(&rhi, ahwb, eglDpy, image, name, eglDestroyImageKHR_, &functions)) {
            // Cache full and every entry pinned: insert took no ownership, so free what we made here.
            eglDestroyImageKHR_(eglDpy, image);
            functions.glDeleteTextures(1, &name);
            name = 0;
        }
        acquireCacheEntryLocked(name);  // provisional pin (no-op if name == 0)
    }

    if (name == 0) {
        return GstHwPathTelemetry::fail(HwVideoBufferPath::AHardwareBuffer);
    }

    // Provisional pin keeps LRU eviction from freeing the name before FrameTextures takes its own pin.
    const auto pinGuard = qScopeGuard([&] {
        const QMutexLocker lock(&s_imageCacheMutex);
        releaseCacheEntryLocked(name);
        drainDeferredImageCacheLocked(eglDestroyImageKHR_, &functions, &rhi);
    });

    if (auto* prev = GstHwFrameTexturesBase::reusableBundle<FrameTextures>(old, HwVideoBufferPath::AHardwareBuffer)) {
        if (prev->matches(&rhi, _format.frameSize(), QVideoFrameFormat::Format_SamplerExternalOES, name)) {
            GstHwPathTelemetry::recordTextureReuse(HwVideoBufferPath::AHardwareBuffer);
            prev->setSourceSample(takeSample());
            QVideoFrameTexturesUPtr reused = std::move(old);
            return reused;
        }
    }

    // Pre-flight the external-OES RHI format/size before createFrom() so an unsupported import demotes to CPU on a query.
    if (!GstHwImportPreflight::preflightOrRecord(&rhi, HwVideoBufferPath::AHardwareBuffer,
                                                 QVideoFrameFormat::Format_SamplerExternalOES, _format.frameSize(),
                                                 QRhiTexture::ExternalOES)) {
        return GstHwPathTelemetry::fail(HwVideoBufferPath::AHardwareBuffer);
    }

    auto textures =
        std::make_unique<FrameTextures>(&rhi, _format.frameSize(), QVideoFrameFormat::Format_SamplerExternalOES, name);
    if (!textures->texture(0)) {
        qCWarning(GstAHWBufLog) << "createFrom failed for plane 0 (SamplerExternalOES)";
        return GstHwPathTelemetry::fail(HwVideoBufferPath::AHardwareBuffer);
    }
    textures->setSourceSample(takeSample());
    return textures;
}

#endif  // QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH
