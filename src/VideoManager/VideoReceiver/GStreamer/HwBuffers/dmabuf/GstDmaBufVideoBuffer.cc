#include "GstDmaBufVideoBuffer.h"

#include "GstContextBridgeRegistry.h"
#include "GstDmaBufVulkanImport.h"
#include "GstDmaFourcc.h"
#include "GstEglHelpers.h"
#include "GstGlFrameTextures.h"
#include "GstHwImportCache.h"
#include "GstHwImportPreflight.h"
#include "GstHwPathTelemetry.h"
#include "GstHwVideoBuffer.h"
#include "GstVulkanFrameTextures.h"
#include "HwBuffers.h"

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)

#include <QtCore/QByteArray>
#include <QtCore/QLoggingCategory>
#include <QtCore/QMutex>
#include <QtCore/QScopeGuard>
#include <QtCore/QSize>
#include <QtGui/QGuiApplication>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>
#include <QtGui/qopenglcontext_platform.h>
#include <gst/allocators/gstdmabuf.h>
#include <gst/video/video.h>
#include <private/qvideotexturehelper_p.h>
#include <rhi/qrhi.h>

#include "QGCLoggingCategory.h"
#if GST_CHECK_VERSION(1, 24, 0)
#include <gst/video/video-info-dma.h>
#endif

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <drm_fourcc.h>
#include <limits>
#include <unistd.h>
#include <utility>
#include <vector>

// Producer dma-buf fence export: optional. The header exists on Ubuntu 22.04 but predates the
// EXPORT_SYNC_FILE uAPI (kernel ~6.0), so gate on the ioctl token, not just header presence.
#if __has_include(<linux/dma-buf.h>)
#include <linux/dma-buf.h>
#include <sys/ioctl.h>
#ifdef DMA_BUF_IOCTL_EXPORT_SYNC_FILE
#define QGC_HAS_DMABUF_EXPORT_SYNC 1
#endif
#endif

#ifndef EGL_ANDROID_native_fence_sync
#define EGL_SYNC_NATIVE_FENCE_ANDROID 0x3144
#define EGL_SYNC_NATIVE_FENCE_FD_ANDROID 0x3145
#define EGL_NO_NATIVE_FENCE_FD_ANDROID -1
#endif

QGC_LOGGING_CATEGORY(GstDmaBufLog, "Video.GStreamer.HwBuffers.GstDmaBuf")

namespace {

using GstHw::kMaxPlanes;

std::atomic<bool> s_loggedBadBackend{false};

// EGL_BAD_MATCH from eglCreateImage = broken EGL/VA-API combo (QTBUG-112312); short-circuits mapTextures on first hit.
std::atomic<bool> s_permanentlyDisabled{false};

void maybePermanentlyDisable(EGLDisplay eglDpy, int plane, const char* pathTag)
{
    if (!s_permanentlyDisabled.exchange(true, std::memory_order_relaxed)) {
        GstHwPathTelemetry::recordFallbackReason(HwVideoBufferPath::DmaBuf,
                                                 GstHwPathTelemetry::HwFallbackReason::EglBadMatch);
        qCWarning(GstDmaBufLog) << "eglCreateImage returned EGL_BAD_MATCH on" << pathTag << "plane" << plane
                                << "— disabling DMABuf zero-copy for this process"
                                << "(EGL_VENDOR=" << eglQueryString(eglDpy, EGL_VENDOR)
                                << "EGL_VERSION=" << eglQueryString(eglDpy, EGL_VERSION) << ")";
    }
}

// EGLDisplay is process-stable; cache the value (not a context pointer) to avoid dangling-key hazard.
std::atomic<EGLDisplay> s_cachedDisplay{EGL_NO_DISPLAY};
std::atomic<bool> s_loggedSingleFdDisabled{false};
std::atomic<bool> s_loggedFenceTimeout{false};

// First-import-per-epoch bind validation: glGetError flushes the pipeline, so steady-state frames skip it.
std::atomic<bool> s_bindValidated{false};
std::atomic<bool> s_loggedBindFailed{false};

constexpr const char* kModifiersExt = "EGL_EXT_image_dma_buf_import_modifiers";

constexpr const char* kFenceSyncExt = "EGL_KHR_fence_sync";

constexpr const char* kNativeFenceExt = "EGL_ANDROID_native_fence_sync";

// EGLImage cache (DEFAULT OFF, QGC_GST_DMABUF_CACHE=1): fd/GstMemory ABA means a recycled pool buffer can serve a
// stale image, so we key on fd plus the full EGL layout and weak-ref the GstMemory to evict before the fd is reused.
bool imageCacheEnabled() noexcept
{
    return HwBuffers::hwBufferEnvConfig().dmaBufCache;
}

bool texStorageImportAllowed(bool contextIsOpenGles, bool hasTexStorageExt, guint64 drmModifier) noexcept
{
    return drmModifier == 0 && !contextIsOpenGles && hasTexStorageExt;
}

bool directGlImportAllowed(bool hasModifiersExt, guint64 drmModifier) noexcept
{
    Q_UNUSED(hasModifiersExt);
    return drmModifier == 0;
}

struct DmaImageKey
{
    int fd = -1;
    guint64 modifier = 0;
    int width = 0;
    int height = 0;
    int fourcc = 0;
    int planeCount = 0;
    std::array<gsize, kMaxPlanes> offsets{};
    std::array<int, kMaxPlanes> strides{};

    bool operator==(const DmaImageKey& o) const noexcept
    {
        return fd == o.fd && modifier == o.modifier && width == o.width && height == o.height && fourcc == o.fourcc &&
               planeCount == o.planeCount && offsets == o.offsets && strides == o.strides;
    }
};

template <typename T>
void hashCombine(std::size_t& h, const T& value) noexcept
{
    h ^= std::hash<T>{}(value) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
}

struct DmaImageKeyHash
{
    std::size_t operator()(const DmaImageKey& k) const noexcept
    {
        std::size_t h = std::hash<int>{}(k.fd);
        hashCombine(h, k.modifier);
        hashCombine(h, k.width);
        hashCombine(h, k.height);
        hashCombine(h, k.fourcc);
        hashCombine(h, k.planeCount);
        for (int p = 0; p < k.planeCount; ++p) {
            hashCombine(h, k.offsets[p]);
            hashCombine(h, k.strides[p]);
        }
        return h;
    }
};

struct DmaCachedImage
{
    EGLDisplay display = EGL_NO_DISPLAY;
    EGLImage image = EGL_NO_IMAGE_KHR;
    GstMemory* mem = nullptr;  // weak-ref source; never dereferenced after notify
};

constexpr int kImageCacheCapacity = 8;

QMutex s_dmaCacheMutex;
// EGLImages orphaned by an off-thread weak-notify; destroyed on the render thread (drainOrphanedImagesLocked).
std::vector<std::pair<EGLDisplay, EGLImage>> s_orphanedImages;

void onSourceMemoryFinalized(gpointer userData, GstMiniObject* finalizedMem);

// Render-thread; caller holds s_dmaCacheMutex with Qt's GL context current.
void drainOrphanedImagesLocked()
{
    for (const auto& [display, image] : s_orphanedImages) {
        if (image != EGL_NO_IMAGE_KHR && display != EGL_NO_DISPLAY) {
            eglDestroyImage(display, image);
        }
    }
    s_orphanedImages.clear();
}

// Eviction/erase/clear hook. Render thread (GL context current): destroy the EGLImage and drop the GstMemory weak-ref.
// Off-thread weak-notify (no GL context): defer the image to s_orphanedImages and leave the weak-ref alone — the notify
// itself consumes it.
void destroyCachedDmaImage(const DmaImageKey&, DmaCachedImage& entry)
{
    const bool onRenderThread = QOpenGLContext::currentContext() != nullptr;
    if (entry.image != EGL_NO_IMAGE_KHR && entry.display != EGL_NO_DISPLAY) {
        if (onRenderThread) {
            eglDestroyImage(entry.display, entry.image);
        } else {
            s_orphanedImages.emplace_back(entry.display, entry.image);
        }
    }
    if (entry.mem && onRenderThread) {
        gst_mini_object_weak_unref(GST_MINI_OBJECT(entry.mem), onSourceMemoryFinalized, nullptr);
    }
}

GstHw::GstHwImportCache<DmaImageKey, DmaCachedImage, DmaImageKeyHash> s_dmaImageCache{kImageCacheCapacity,
                                                                                      destroyCachedDmaImage};

// Weak-notify on the pool/streaming thread: no GL context, so the stale EGLImage is queued for render-thread
// destruction (drainOrphanedImagesLocked) rather than destroyed here.
void onSourceMemoryFinalized(gpointer userData, GstMiniObject* finalizedMem)
{
    const QMutexLocker lock(&s_dmaCacheMutex);
    s_dmaImageCache.eraseIf([&](const DmaImageKey&, const DmaCachedImage& e) {
        return e.mem == reinterpret_cast<GstMemory*>(finalizedMem);
    });
    Q_UNUSED(userData);
}

// Render-thread; caller holds s_dmaCacheMutex with Qt's GL context current.
void clearDmaImageCacheLocked(EGLDisplay eglDpy)
{
    s_dmaImageCache.clear();
    drainOrphanedImagesLocked();
    Q_UNUSED(eglDpy);
}

EGLImage cachedDmaImageLocked(const DmaImageKey& key, EGLDisplay eglDpy)
{
    if (DmaCachedImage* entry = s_dmaImageCache.find(key);
        entry && entry->display == eglDpy && entry->image != EGL_NO_IMAGE_KHR) {
        return entry->image;
    }
    return EGL_NO_IMAGE_KHR;
}

void insertDmaImageLocked(const DmaImageKey& key, EGLDisplay eglDpy, EGLImage image, GstMemory* mem)
{
    if (mem) {
        gst_mini_object_weak_ref(GST_MINI_OBJECT(mem), onSourceMemoryFinalized, nullptr);
    }
    s_dmaImageCache.insert(key, DmaCachedImage{eglDpy, image, mem});
}

std::atomic<bool> s_dmaCacheResetPending{false};

bool singleFdImportEnabled() noexcept
{
    return HwBuffers::hwBufferEnvConfig().dmaBufSingleEglImage;
}

// Per-plane EGL attrib keys — spec defines distinct enums per plane index.
static constexpr EGLint kPlFd[4] = {
    EGL_DMA_BUF_PLANE0_FD_EXT,
    EGL_DMA_BUF_PLANE1_FD_EXT,
    EGL_DMA_BUF_PLANE2_FD_EXT,
    EGL_DMA_BUF_PLANE3_FD_EXT,
};
static constexpr EGLint kPlOffset[4] = {
    EGL_DMA_BUF_PLANE0_OFFSET_EXT,
    EGL_DMA_BUF_PLANE1_OFFSET_EXT,
    EGL_DMA_BUF_PLANE2_OFFSET_EXT,
    EGL_DMA_BUF_PLANE3_OFFSET_EXT,
};
static constexpr EGLint kPlPitch[4] = {
    EGL_DMA_BUF_PLANE0_PITCH_EXT,
    EGL_DMA_BUF_PLANE1_PITCH_EXT,
    EGL_DMA_BUF_PLANE2_PITCH_EXT,
    EGL_DMA_BUF_PLANE3_PITCH_EXT,
};
static constexpr EGLint kPlModLo[4] = {
    EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT,
    EGL_DMA_BUF_PLANE1_MODIFIER_LO_EXT,
    EGL_DMA_BUF_PLANE2_MODIFIER_LO_EXT,
    EGL_DMA_BUF_PLANE3_MODIFIER_LO_EXT,
};
static constexpr EGLint kPlModHi[4] = {
    EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT,
    EGL_DMA_BUF_PLANE1_MODIFIER_HI_EXT,
    EGL_DMA_BUF_PLANE2_MODIFIER_HI_EXT,
    EGL_DMA_BUF_PLANE3_MODIFIER_HI_EXT,
};

class FrameTextures final : public GstGlFrameTextures
{
public:
    FrameTextures(QRhi* rhi, QSize size, QVideoFrameFormat::PixelFormat pixelFormat,
                  std::array<GLuint, kMaxPlanes> names, int count)
        : GstGlFrameTextures(rhi, size, pixelFormat, names, count, FallbackPolicy::Disable)
    {}

    ~FrameTextures() override
    {
        releaseGLTextures();  // safety net if onFrameEndInvoked() was never called
    }

    void onFrameEndInvoked() override
    {
        releaseGLTextures();
        GstHwFrameTexturesBase::onFrameEndInvoked();
    }

private:
    void releaseGLTextures()
    {
        if (_released || !_rhi || _count == 0)
            return;
        // Bind the QRhi GL context so currentContext() returns it (mirrors ~QGstQVideoFrameTextures), avoiding the
        // backend-conditional NativeHandles cast.
        _rhi->makeThreadLocalNativeContextCurrent();
        if (QOpenGLContext* ctx = QOpenGLContext::currentContext()) {
            ctx->functions()->glDeleteTextures(int(_count), _names.data());
            _released = true;  // only after deletion, so the dtor can retry
        }
    }

    bool _released = false;
};

// RPi eglfs quirk: V3D EGL implicitly converts YUYV/UYVY DMABuf to RGBA, so declare RGBA8888 to pick Qt's RGB shader
// and avoid a double YUV->RGB (mirrors Qt's eglfs branch).
QVideoFrameFormat applyEglfsFormatQuirk(const QVideoFrameFormat& format)
{
    static const bool isEglfsQPA = QGuiApplication::platformName() == QLatin1String("eglfs");
    if (!isEglfsQPA)
        return format;
    const auto fmt = format.pixelFormat();
    if (fmt != QVideoFrameFormat::Format_UYVY && fmt != QVideoFrameFormat::Format_YUYV) {
        return format;
    }
    QVideoFrameFormat spoofed(format.frameSize(), QVideoFrameFormat::Format_RGBA8888);
    spoofed.setStreamFrameRate(format.streamFrameRate());
    spoofed.setColorRange(format.colorRange());
    spoofed.setColorSpace(format.colorSpace());
    spoofed.setColorTransfer(format.colorTransfer());
    spoofed.setViewport(format.viewport());
    return spoofed;
}

std::atomic<bool> s_loggedExplicitFence{false};

// Best-effort GPU-side wait on the producer's fence: export a sync_file fd from the dma-buf, import it as an
// EGL_SYNC_NATIVE_FENCE_ANDROID sync and eglWaitSyncKHR (server wait). Returns true only on a confirmed GPU-side wait
// that makes the mmap CPU barrier redundant; any failure leaves the caller's existing barrier path untouched.
bool tryExplicitFenceWait(EGLDisplay eglDpy, int dmaFd)
{
#if defined(QGC_HAS_DMABUF_EXPORT_SYNC)
    if (eglDpy == EGL_NO_DISPLAY || dmaFd < 0) {
        return false;
    }
    if (!GstEglHelpers::displaySupportsExtension(eglDpy, kNativeFenceExt)) {
        return false;
    }
    static const auto createSyncKHR = reinterpret_cast<PFNEGLCREATESYNCKHRPROC>(eglGetProcAddress("eglCreateSyncKHR"));
    static const auto waitSyncKHR = reinterpret_cast<PFNEGLWAITSYNCKHRPROC>(eglGetProcAddress("eglWaitSyncKHR"));
    static const auto destroySyncKHR =
        reinterpret_cast<PFNEGLDESTROYSYNCKHRPROC>(eglGetProcAddress("eglDestroySyncKHR"));
    if (!createSyncKHR || !waitSyncKHR || !destroySyncKHR) {
        return false;
    }

    dma_buf_export_sync_file req = {};
    req.flags = DMA_BUF_SYNC_READ;  // wait only on writers (the decoder), not on prior readers
    req.fd = -1;
    if (ioctl(dmaFd, DMA_BUF_IOCTL_EXPORT_SYNC_FILE, &req) != 0 || req.fd < 0) {
        return false;
    }

    // EGL takes ownership of req.fd on a successful create (closes it via eglDestroySync); close it ourselves only if
    // create fails.
    const EGLint attribs[] = {EGL_SYNC_NATIVE_FENCE_FD_ANDROID, req.fd, EGL_NONE};
    EGLSyncKHR sync = createSyncKHR(eglDpy, EGL_SYNC_NATIVE_FENCE_ANDROID, attribs);
    if (sync == EGL_NO_SYNC_KHR) {
        ::close(req.fd);
        return false;
    }
    const EGLint waited = waitSyncKHR(eglDpy, sync, 0);
    destroySyncKHR(eglDpy, sync);
    if (waited != EGL_TRUE) {
        return false;
    }
    QGC_HW_WARN_ONCE(GstDmaBufLog, s_loggedExplicitFence,
                     "DMABuf: using producer dma-buf fence for GPU-side sync (mmap barrier skipped)");
    GstHwPathTelemetry::recordExplicitFenceWait(HwVideoBufferPath::DmaBuf);
    GstHwPathTelemetry::recordSyncWait(HwVideoBufferPath::DmaBuf, /*gpuSide=*/true);
    return true;
#else
    Q_UNUSED(eglDpy);
    Q_UNUSED(dmaFd);
    return false;
#endif
}

}  // namespace

GstDmaBufVideoBuffer::GstDmaBufVideoBuffer(GstSample* sample, const GstVideoInfo& videoInfo,
                                           const QVideoFrameFormat& format, EGLDisplay eglDisplay)
    : GstHwVideoBuffer(QVideoFrame::RhiTextureHandle, sample, videoInfo, applyEglfsFormatQuirk(format)),
      _eglDisplay(eglDisplay)
{
#if GST_CHECK_VERSION(1, 24, 0)
    if (GstCaps* caps = gst_sample_get_caps(sample); caps && gst_video_is_dma_drm_caps(caps)) {
        GstVideoInfoDmaDrm drmInfo = {};
        gst_video_info_dma_drm_init(&drmInfo);
        // DRM_FORMAT_MOD_INVALID would yield EGL_BAD_PARAMETER on strict drivers (AMD RADV); leave _drmModifier at 0
        // (LINEAR).
        if (gst_video_info_dma_drm_from_caps(&drmInfo, caps) && drmInfo.drm_modifier != DRM_FORMAT_MOD_INVALID) {
            _drmModifier = drmInfo.drm_modifier;
        }
    }
#endif
}

void GstDmaBufVideoBuffer::resetCachedState() noexcept
{
    s_cachedDisplay.store(EGL_NO_DISPLAY, std::memory_order_release);
    s_permanentlyDisabled.store(false, std::memory_order_release);
    s_loggedSingleFdDisabled.store(false, std::memory_order_release);
#if defined(QGC_HAS_GST_VULKAN_GPU_PATH)
    GstDmaBufVulkan::resetLoggedState();
#endif
    s_loggedBadBackend.store(false, std::memory_order_release);
    s_loggedFenceTimeout.store(false, std::memory_order_release);
    s_bindValidated.store(false, std::memory_order_release);
    s_loggedBindFailed.store(false, std::memory_order_release);
    if (QOpenGLContext::currentContext()) {
        // GL context current (render-thread reset, e.g. sceneGraphInvalidated): drain now so cached/orphaned EGLImages
        // can't leak when no DMABuf frame is ever mapped again.
        const QMutexLocker lock(&s_dmaCacheMutex);
        clearDmaImageCacheLocked(EGL_NO_DISPLAY);
        s_dmaCacheResetPending.store(false, std::memory_order_release);
    } else {
        // Defer EGLImage teardown to the next render-thread mapTextures(); eglDestroyImage off the GL thread is
        // unsafe.
        s_dmaCacheResetPending.store(true, std::memory_order_release);
    }
}

namespace {
struct DmaBufCacheResetRegistrar
{
    DmaBufCacheResetRegistrar()
    {
        GstContextBridgeRegistry::registerCacheReset(&GstDmaBufVideoBuffer::resetCachedState);
    }
};

const DmaBufCacheResetRegistrar s_dmaBufCacheResetRegistrar;
}  // namespace

#ifdef QGC_GST_BUILD_TESTING
bool GstDmaBufVideoBuffer::singleFdImportEnabledForTest() noexcept
{
    // Read live: production singleFdImportEnabled() reads the parse-once env cache, which can't observe
    // per-case env mutations under test. Mirrors HwBuffers truthy() with default=on.
    const QByteArray v = qgetenv("QGC_GST_DMABUF_SINGLE_EGLIMAGE").trimmed().toLower();
    if (v.isEmpty()) {
        return true;
    }
    return v != "0" && v != "false" && v != "off" && v != "no";
}

bool GstDmaBufVideoBuffer::texStorageImportAllowedForTest(bool contextIsOpenGles, bool hasTexStorageExt,
                                                          guint64 drmModifier) noexcept
{
    return texStorageImportAllowed(contextIsOpenGles, hasTexStorageExt, drmModifier);
}

bool GstDmaBufVideoBuffer::directGlImportAllowedForTest(bool hasModifiersExt, guint64 drmModifier) noexcept
{
    return directGlImportAllowed(hasModifiersExt, drmModifier);
}
#endif

bool GstDmaBufVideoBuffer::validatePlaneHandles() const
{
    // EGL_BAD_MATCH latch: fail validation so the factory demotes to CPU instead of building doomed DmaBuf frames.
    if (s_permanentlyDisabled.load(std::memory_order_relaxed)) {
        return false;
    }
    const bool planesOk = validatePlanes([](GstMemory* mem) {
        if (!mem || !gst_is_dmabuf_memory(mem))
            return false;
        return gst_dmabuf_memory_get_fd(mem) >= 0;
    });
    if (!planesOk)
        return false;
    // Reject tiled (non-LINEAR) DMABuf without EGL_EXT_image_dma_buf_import_modifiers — mapTextures() needs the
    // modifier attribs to import tiled layouts.
#if GST_CHECK_VERSION(1, 24, 0)
    // Without the modifiers ext, mmap of tiled strides produces garbage and faults libgstvideo.
    if (_drmModifier != 0 &&
        (_eglDisplay == EGL_NO_DISPLAY || !GstEglHelpers::displaySupportsExtension(_eglDisplay, kModifiersExt))) {
        return false;
    }
#endif
    return true;
}

QVideoFrameTexturesUPtr GstDmaBufVideoBuffer::mapTextures(QRhi& rhi, QVideoFrameTexturesUPtr& /*old*/)
{
    const GstHwPathTelemetry::ScopedMapTimer mapTimer(HwVideoBufferPath::DmaBuf);
    // Qt's contract: mapTextures runs on the QRhi (render) thread. Bail rather than crash if ever called off-thread.
    if (!rhi.thread()->isCurrentThread()) {
        return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
    }

    if (!_sample) {
        return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
    }

    // Bail once a prior frame hit EGL_BAD_MATCH (broken EGL/VA-API, QTBUG-112312); retrying stalls the pipeline.
    if (s_permanentlyDisabled.load(std::memory_order_relaxed)) {
        return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
    }

    // Modifier cached from caps in the ctor (0 = LINEAR / non-DRM / normalized INVALID).
    const guint64 drmModifier = _drmModifier;

    // Sync barrier for Intel iHD legacy LINEAR (no implicit fence). Prefer an EGL fence over a full-frame mmap;
    // the fence path needs the GL context current, so it runs below. QGC_GST_DMABUF_NO_MMAP_FENCE=1 skips both where
    // PRIME 2 fence FDs make the barrier redundant.
    const bool skipLinearFence = HwBuffers::hwBufferEnvConfig().dmaBufNoMmapFence;
    const bool needLinearFence = (drmModifier == 0 /* DRM_FORMAT_MOD_LINEAR / unknown */ && !skipLinearFence);

#if defined(QGC_HAS_GST_VULKAN_GPU_PATH)
    if (rhi.backend() == QRhi::Vulkan) {
        return importVulkan(rhi);
    }
#endif
    if (rhi.backend() != QRhi::OpenGLES2) {
        QGC_HW_WARN_ONCE(GstDmaBufLog, s_loggedBadBackend,
                         "QRhi backend is not OpenGLES2/Vulkan; zero-copy DMABuf path unsupported");
        return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
    }

    // Bind Qt's GL context before any EGL/GL call, else the calls no-op into a foreign/null context (import succeeds
    // but samples green).
    rhi.makeThreadLocalNativeContextCurrent();

    // Mirror Qt 6.10 qgstvideobuffer.cpp:349 — surface latent GL/EGL state from prior frames.
    // Qt 6.12 moved its DMABuf gate from eglfs-only to qGstEglCanMapDmaBuf() and added modifier checks; keep this
    // richer QGC path in sync when raising the Qt-private-source baseline.
    if (Q_UNLIKELY(GstDmaBufLog().isDebugEnabled())) {
        const GLenum glErr = glGetError();
        const EGLint eglErr = eglGetError();
        if (glErr != GL_NO_ERROR || eglErr != EGL_SUCCESS) {
            qCDebug(GstDmaBufLog) << "mapTextures entry, latent glError=" << Qt::hex << glErr << "eglError=" << eglErr;
        }
    }

    const auto* nativeHandles = static_cast<const QRhiGles2NativeHandles*>(rhi.nativeHandles());
    if (!nativeHandles || !nativeHandles->context) {
        qCWarning(GstDmaBufLog) << "QRhi exposes no GL context";
        return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
    }

    // Prefer QEGLContext::display(): EGLImages from one display can't be sampled by a context on another, and
    // eglGetCurrentDisplay/eglGetDisplay disagree under xcb_egl.
    EGLDisplay eglDpy = s_cachedDisplay.load(std::memory_order_acquire);
    if (eglDpy == EGL_NO_DISPLAY) {
        eglDpy = GstEglHelpers::resolveEglDisplay(nativeHandles->context);
        if (eglDpy == EGL_NO_DISPLAY)
            eglDpy = _eglDisplay;
        if (eglDpy == EGL_NO_DISPLAY) {
            return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
        }
        // First writer wins; racing renders that resolve the same display make this idempotent.
        EGLDisplay expected = EGL_NO_DISPLAY;
        s_cachedDisplay.compare_exchange_strong(expected, eglDpy, std::memory_order_release);
    }

    // Render thread, GL context current: reap EGLImages orphaned by off-thread weak-notifies, then any pending reset.
    if (imageCacheEnabled()) {
        const bool resetPending = s_dmaCacheResetPending.exchange(false, std::memory_order_acq_rel);
        const QMutexLocker lock(&s_dmaCacheMutex);
        if (!s_orphanedImages.empty()) {
            drainOrphanedImagesLocked();
        }
        if (resetPending) {
            clearDmaImageCacheLocked(eglDpy);
        }
    }

    // LINEAR sync barrier: prefer EGL_KHR_fence_sync (insert+client-wait, a GPU-completion wait) over mmapping the
    // whole frame. mmap remains the fallback when the fence ext is absent (a CPU-side read barrier).
    if (needLinearFence) {
        // Prefer importing the producer's dma-buf fence for a pure GPU-side wait; only if that fails do we fall
        // back to the EGL-fence-then-mmap barrier below (never removed — just skipped on a confirmed GPU wait).
        int producerFd = -1;
        if (GstBuffer* fenceBuf = gst_sample_get_buffer(_sample)) {
            if (GstMemory* m0 = gst_buffer_peek_memory(fenceBuf, 0); m0 && gst_is_dmabuf_memory(m0)) {
                producerFd = gst_dmabuf_memory_get_fd(m0);
            }
        }
        const bool producerFenced = tryExplicitFenceWait(eglDpy, producerFd);
        if (!producerFenced) {
            static std::pair<EGLDisplay, bool> fenceExtProbe{EGL_NO_DISPLAY, false};
            if (fenceExtProbe.first != eglDpy) {
                fenceExtProbe = {eglDpy, GstEglHelpers::displaySupportsExtension(eglDpy, kFenceSyncExt)};
            }
            const bool hasFenceExt = fenceExtProbe.second;
            static const auto createSyncKHR =
                reinterpret_cast<PFNEGLCREATESYNCKHRPROC>(eglGetProcAddress("eglCreateSyncKHR"));
            static const auto clientWaitSyncKHR =
                reinterpret_cast<PFNEGLCLIENTWAITSYNCKHRPROC>(eglGetProcAddress("eglClientWaitSyncKHR"));
            static const auto destroySyncKHR =
                reinterpret_cast<PFNEGLDESTROYSYNCKHRPROC>(eglGetProcAddress("eglDestroySyncKHR"));
            bool fenced = false;
            if (hasFenceExt && createSyncKHR && clientWaitSyncKHR && destroySyncKHR) {
                if (EGLSyncKHR sync = createSyncKHR(eglDpy, EGL_SYNC_FENCE_KHR, nullptr); sync != EGL_NO_SYNC_KHR) {
                    // Bounded wait: this runs on Qt's render thread, so a GPU hang must not block forever — time out
                    // and fall through to the mmap barrier instead.
                    const EGLint waitResult = clientWaitSyncKHR(eglDpy, sync, EGL_SYNC_FLUSH_COMMANDS_BIT_KHR,
                                                                static_cast<EGLTimeKHR>(100000000));
                    destroySyncKHR(eglDpy, sync);
                    GstHwPathTelemetry::recordSyncWait(HwVideoBufferPath::DmaBuf, /*gpuSide=*/true);
                    if (waitResult == EGL_CONDITION_SATISFIED_KHR) {
                        fenced = true;
                    } else {
                        GstHwPathTelemetry::recordFenceTimeout(HwVideoBufferPath::DmaBuf);
                        GstHwPathTelemetry::recordFallbackReason(HwVideoBufferPath::DmaBuf,
                                                                 GstHwPathTelemetry::HwFallbackReason::FenceTimeout);
                        QGC_HW_WARN_ONCE(
                            GstDmaBufLog, s_loggedFenceTimeout,
                            "EGL fence wait did not satisfy within 100 ms (GPU stall?) — using mmap barrier");
                    }
                }
            }
            if (!fenced) {
                // Fallback: mmap the frame purely as a CPU-side completion barrier (defeats zero-copy for one map
                // cycle).
                if (GstBuffer* syncBuf = gst_sample_get_buffer(_sample)) {
                    GstVideoFrame syncFrame;
                    if (gst_video_frame_map(&syncFrame, &_videoInfo, syncBuf, GST_MAP_READ)) {
                        gst_video_frame_unmap(&syncFrame);
                        GstHwPathTelemetry::recordSyncWait(HwVideoBufferPath::DmaBuf, /*gpuSide=*/false);
                        GstHwPathTelemetry::recordMmapBarrierHit(HwVideoBufferPath::DmaBuf);
                    }
                }
            }
        }
    }

    GstBuffer* buffer = gst_sample_get_buffer(_sample);
    if (!buffer || !gst_is_dmabuf_memory(gst_buffer_peek_memory(buffer, 0))) {
        return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
    }
    const bool hasModifiersExt = GstEglHelpers::displaySupportsExtension(eglDpy, kModifiersExt);
    if (!directGlImportAllowed(hasModifiersExt, drmModifier)) {
        // Reaching here means caps/filtering changed between the validate and render threads or an upstream element
        // pushed a modifier layout we do not advertise. Fall back before either GL import path can bind the EGLImage.
        static std::atomic<bool> warnedModifierRejected{false};
        if (!warnedModifierRejected.exchange(true, std::memory_order_relaxed)) {
            qCWarning(GstDmaBufLog)
                << "DMABuf modifier" << Qt::hex << drmModifier
                << "rejected for direct GL import"
                << (hasModifiersExt ? "(EGL modifier import present but disabled for safety)"
                                    : "(EGL_EXT_image_dma_buf_import_modifiers absent)")
                << "— falling back to CPU";
        }
        GstHwPathTelemetry::recordFallbackReason(HwVideoBufferPath::DmaBuf,
                                                 GstHwPathTelemetry::HwFallbackReason::ModifierRejected);
        return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
    }

    static const auto eglImageTargetTexture2D =
        reinterpret_cast<PFNGLEGLIMAGETARGETTEXTURE2DOESPROC>(eglGetProcAddress("glEGLImageTargetTexture2DOES"));
    if (!eglImageTargetTexture2D) {
        qCWarning(GstDmaBufLog) << "glEGLImageTargetTexture2DOES unavailable";
        return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
    }

    // Desktop GL with GL_EXT_EGL_image_storage: bind LINEAR DMABuf via immutable storage to skip the per-frame
    // texture-storage realloc the mutable OES path forces (OBS measured ~100x on discrete Intel). Tiled DMABuf stays
    // on glEGLImageTargetTexture2DOES; Mesa/Gallium can segfault when tiled VA images take the tex-storage path.
    // GLES also keeps the OES path: the storage ext isn't reliably exposed and the OES bind is the contract there.
    PFNGLEGLIMAGETARGETTEXSTORAGEEXTPROC eglImageTargetTexStorage = nullptr;
    if (const QOpenGLContext* ctx = QOpenGLContext::currentContext()) {
        const bool useTexStorage =
            texStorageImportAllowed(ctx->isOpenGLES(),
                                    ctx->hasExtension(QByteArrayLiteral("GL_EXT_EGL_image_storage")), drmModifier);
        eglImageTargetTexStorage = useTexStorage
                                       ? reinterpret_cast<PFNGLEGLIMAGETARGETTEXSTORAGEEXTPROC>(
                                             eglGetProcAddress("glEGLImageTargetTexStorageEXT"))
                                       : nullptr;
    }
    const auto bindEglImage = [&](GLenum target, EGLImage img) {
        if (eglImageTargetTexStorage)
            eglImageTargetTexStorage(target, img, nullptr);
        else
            eglImageTargetTexture2D(target, img);
    };
    // Validate binds with glGetError only on the first import per epoch (reset via resetCachedState); steady-state
    // frames skip the pipeline flush.
    const bool validateBind = !s_bindValidated.load(std::memory_order_relaxed);

    // gst_video_frame_map(GST_MAP_READ) on DMABuf mmaps and defeats zero-copy; read offsets from GstVideoMeta.
    // Qt 6.12's upstream GStreamer path also prefers GstVideoMeta/video-info offsets; preserve this no-mmap behavior when
    // reconciling against newer Qt sources.
    GstVideoMeta* vmeta = gst_buffer_get_video_meta(buffer);
    const int planeCount = std::clamp(int(GST_VIDEO_INFO_N_PLANES(&_videoInfo)), 1, kMaxPlanes);
    const int memCount = int(gst_buffer_n_memory(buffer));
    // memCount==1: either multi-plane shared fd (NV12) or packed single-plane (YUY2/UYVY/…).
    const int cachedSingleFourcc = (memCount == 1) ? GstHw::drmFourccForSingleFd(_videoInfo) : -1;
    bool singleFdPacking = (cachedSingleFourcc != -1);
    if (singleFdPacking && !singleFdImportEnabled()) {
        if (!s_loggedSingleFdDisabled.exchange(true, std::memory_order_relaxed)) {
            qCInfo(GstDmaBufLog) << "Single-EGLImage DMABuf import disabled by"
                                 << "QGC_GST_DMABUF_SINGLE_EGLIMAGE; using per-plane import"
                                 << "when the buffer layout permits, otherwise CPU fallback";
        }
        singleFdPacking = false;
    }
    if (memCount != planeCount && !singleFdPacking) {
        return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
    }

    // Pre-flight RHI format/size support before any EGLImage import / createFrom() so an unsupported frame demotes to
    // CPU on a query instead of after the GL/EGL work.
    if (!GstHwImportPreflight::preflightOrRecord(&rhi, HwVideoBufferPath::DmaBuf, _format.pixelFormat(),
                                                 _format.frameSize())) {
        return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
    }

    // No texture reuse: caching QRhiTextures across frames caused periodic green frames on real RTSP H.264, and the
    // ~us/frame win isn't worth it.
    std::array<GLuint, kMaxPlanes> names{};
    QOpenGLFunctions functions(nativeHandles->context);
    functions.glGenTextures(planeCount, names.data());

    // Modifier attribs only with the ext present; otherwise impl assumes LINEAR (matches our rejection above).
    const EGLAttrib modLo = static_cast<EGLAttrib>(drmModifier & 0xFFFFFFFFu);
    const EGLAttrib modHi = static_cast<EGLAttrib>((drmModifier >> 32) & 0xFFFFFFFFu);

    bool ok = true;

    // Single-fd fast path: one EGLImage for all planes (Mesa VA-API NV12 ships memCount==1; iHD rejects split-plane
    // R8/GR88 imports).
    if (singleFdPacking) {
        if (planeCount > kMaxPlanes) {
            qCWarning(GstDmaBufLog) << "single-fd format has" << planeCount << "planes (>" << kMaxPlanes
                                    << "); falling back to per-plane";
            goto per_plane_path;
        }
        const int singleFourcc = cachedSingleFourcc;
        if (singleFourcc == -1) {
            goto per_plane_path;
        }
        const int fd0 = gst_dmabuf_memory_get_fd(gst_buffer_peek_memory(buffer, 0));
        // attribPush writes 2 EGLAttribs/call: 3 header keys + up to 5/plane*kMaxPlanes, *2, +1 EGL_NONE terminator.
        EGLAttrib attribs[2 * (3 + kMaxPlanes * 5) + 1];
        int n = 0;
        bool attribOverflow = false;
        auto attribPush = [&](EGLAttrib k, EGLAttrib v) {
            if (n >= int(std::size(attribs)) - 1) {
                attribOverflow = true;
                return;
            }
            attribs[n++] = k;
            attribs[n++] = v;
        };
        attribPush(EGL_WIDTH, GST_VIDEO_INFO_WIDTH(&_videoInfo));
        attribPush(EGL_HEIGHT, GST_VIDEO_INFO_HEIGHT(&_videoInfo));
        attribPush(EGL_LINUX_DRM_FOURCC_EXT, singleFourcc);
        std::array<gsize, kMaxPlanes> planeOffsets{};
        std::array<int, kMaxPlanes> planeStrides{};
        for (int p = 0; p < planeCount; ++p) {
            const auto off = vmeta ? vmeta->offset[p] : GST_VIDEO_INFO_PLANE_OFFSET(&_videoInfo, p);
            const auto pitch = vmeta ? vmeta->stride[p] : GST_VIDEO_INFO_PLANE_STRIDE(&_videoInfo, p);
            if (pitch <= 0 ||
                static_cast<quint64>(off) > static_cast<quint64>((std::numeric_limits<EGLAttrib>::max)())) {
                qCWarning(GstDmaBufLog) << "implausible plane stride/offset (stride=" << pitch << "offset=" << off
                                        << "); CPU fallback";
                ok = false;
                goto textures_built;
            }
            planeOffsets[p] = off;
            planeStrides[p] = pitch;
            attribPush(kPlFd[p], fd0);
            attribPush(kPlOffset[p], static_cast<EGLAttrib>(off));
            attribPush(kPlPitch[p], static_cast<EGLAttrib>(pitch));
            if (hasModifiersExt) {  // interleaved per spec: modifier attribs follow FD/OFFSET/PITCH for same plane
                attribPush(kPlModLo[p], modLo);
                attribPush(kPlModHi[p], modHi);
            }
        }
        if (attribOverflow) {
            qCWarning(GstDmaBufLog) << "single-fd EGL attrib list overflow; falling back to per-plane";
            goto per_plane_path;
        }
        attribs[n++] = EGL_NONE;
        const bool cacheOn = imageCacheEnabled();
        const DmaImageKey key{fd0,
                              drmModifier,
                              GST_VIDEO_INFO_WIDTH(&_videoInfo),
                              GST_VIDEO_INFO_HEIGHT(&_videoInfo),
                              singleFourcc,
                              planeCount,
                              planeOffsets,
                              planeStrides};
        EGLImage singleImage = EGL_NO_IMAGE_KHR;
        bool singleImageCached = false;
        if (cacheOn) {
            const QMutexLocker lock(&s_dmaCacheMutex);
            singleImage = cachedDmaImageLocked(key, eglDpy);
            singleImageCached = (singleImage != EGL_NO_IMAGE_KHR);
        }
        if (singleImage == EGL_NO_IMAGE_KHR) {
            singleImage = eglCreateImage(eglDpy, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, nullptr, attribs);
            if (cacheOn) {
                GstHwPathTelemetry::recordImageCacheMiss(HwVideoBufferPath::DmaBuf);
            }
        } else if (cacheOn) {
            GstHwPathTelemetry::recordImageCacheHit(HwVideoBufferPath::DmaBuf);
        }
        if (singleImage != EGL_NO_IMAGE_KHR) {
            bool bindFailed = false;
            for (int p = 0; p < planeCount; ++p) {
                functions.glBindTexture(GL_TEXTURE_2D, names[p]);
                bindEglImage(GL_TEXTURE_2D, singleImage);
                // glGetError forces a pipeline flush — only first import per epoch or when category debug is on.
                if (Q_UNLIKELY(validateBind || GstDmaBufLog().isDebugEnabled())) {
                    if (const GLenum glErr = functions.glGetError(); glErr != GL_NO_ERROR) {
                        if (validateBind) {
                            QGC_HW_WARN_ONCE(GstDmaBufLog, s_loggedBindFailed,
                                             "single-fd EGLImage bind failed plane"
                                                 << p << "glError=" << Qt::hex << glErr << "eglError=" << eglGetError()
                                                 << "— CPU fallback");
                            bindFailed = true;
                            break;
                        }
                        qCDebug(GstDmaBufLog) << "single-fd eglImageTargetTexture2D plane" << p << "glError=" << Qt::hex
                                              << glErr << "eglError=" << eglGetError();
                    }
                }
            }
            if (bindFailed) {
                // Evict/destroy the suspect image so it can't serve later frames.
                if (cacheOn && singleImageCached) {
                    const QMutexLocker lock(&s_dmaCacheMutex);
                    s_dmaImageCache.eraseIf([&](const DmaImageKey& k, const DmaCachedImage&) { return k == key; });
                } else {
                    eglDestroyImage(eglDpy, singleImage);
                }
                ok = false;
                goto textures_built;
            }
            if (cacheOn && !singleImageCached) {
                const QMutexLocker lock(&s_dmaCacheMutex);
                insertDmaImageLocked(key, eglDpy, singleImage, gst_buffer_peek_memory(buffer, 0));
            } else if (!cacheOn) {
                eglDestroyImage(eglDpy, singleImage);
            }
            goto textures_built;
        }
#if defined(DRM_FORMAT_Y210) || defined(DRM_FORMAT_Y410)
        // Y210/Y410 have no per-plane fallback; warn once that frames will be black.
        {
            static std::atomic<bool> s_warnedY210{false};
            static std::atomic<bool> s_warnedY410{false};
#if defined(DRM_FORMAT_Y210)
            if (singleFourcc == DRM_FORMAT_Y210 && !s_warnedY210.exchange(true, std::memory_order_relaxed))
                qCWarning(GstDmaBufLog) << "EGL DMABuf import of Y210 unsupported on this driver; frames will be black";
#endif
#if defined(DRM_FORMAT_Y410)
            if (singleFourcc == DRM_FORMAT_Y410 && !s_warnedY410.exchange(true, std::memory_order_relaxed))
                qCWarning(GstDmaBufLog) << "EGL DMABuf import of Y410 unsupported on this driver; frames will be black";
#endif
        }
#endif
        const EGLint singleFdErr = eglGetError();
        if (drmModifier != 0) {
            // No per-plane fallback for tiled: GL_TEXTURE_2D import on tiled memory is unsafe (driver
            // crash/corruption).
            qCWarning(GstDmaBufLog) << "single-fd tiled eglCreateImage failed (" << Qt::hex << singleFdErr
                                    << ") — no per-plane fallback for tiled";
            if (singleFdErr == EGL_BAD_MATCH) {
                maybePermanentlyDisable(eglDpy, 0, "single-fd-tiled");
            }
            ok = false;
            goto textures_built;
        }
        qCWarning(GstDmaBufLog) << "single-fd eglCreateImage failed (" << Qt::hex << singleFdErr
                                << "); falling back to per-plane";
        if (singleFdErr == EGL_BAD_MATCH) {
            // BAD_MATCH on single-fd is permanent (per-plane hits the same issue); mark disabled but still try
            // per-plane to surface the second log.
            maybePermanentlyDisable(eglDpy, 0, "single-fd");
        }
    }

per_plane_path:
    for (int i = 0; i < planeCount && ok; ++i) {
        const int memIdx = singleFdPacking ? 0 : i;
        const int fd = gst_dmabuf_memory_get_fd(gst_buffer_peek_memory(buffer, memIdx));
        // Dedicated-fd planes already start at the plane data; the offset only applies to single-fd packing.
        const auto offset =
            singleFdPacking ? (vmeta ? vmeta->offset[i] : GST_VIDEO_INFO_PLANE_OFFSET(&_videoInfo, i)) : 0;
        const auto stride = vmeta ? vmeta->stride[i] : GST_VIDEO_INFO_PLANE_STRIDE(&_videoInfo, i);
        if (stride <= 0 ||
            static_cast<quint64>(offset) > static_cast<quint64>((std::numeric_limits<EGLAttrib>::max)())) {
            qCWarning(GstDmaBufLog) << "implausible plane stride/offset (stride=" << stride << "offset=" << offset
                                    << "); CPU fallback";
            ok = false;
            break;
        }
        const int planeWidth = GST_VIDEO_INFO_COMP_WIDTH(&_videoInfo, i);
        const int planeHeight = GST_VIDEO_INFO_COMP_HEIGHT(&_videoInfo, i);
        const int fourcc = GstHw::drmFourccForPlane(_videoInfo, i);
        if (fourcc == -1) {
            qCWarning(GstDmaBufLog) << "no DRM fourcc for format"
                                    << gst_video_format_to_string(GST_VIDEO_INFO_FORMAT(&_videoInfo));
            ok = false;
            break;
        }

        // EGL attribute list — sized for the maximum (with modifier ext) and zero-terminated.
        EGLAttrib attribs[18];
        int n = 0;
        bool attribOverflow = false;
        auto attribPush = [&](EGLAttrib k, EGLAttrib v) {
            if (n >= int(std::size(attribs)) - 1) {
                attribOverflow = true;
                return;
            }
            attribs[n++] = k;
            attribs[n++] = v;
        };
        attribPush(EGL_WIDTH, planeWidth);
        attribPush(EGL_HEIGHT, planeHeight);
        attribPush(EGL_LINUX_DRM_FOURCC_EXT, fourcc);
        attribPush(kPlFd[0], fd);
        attribPush(kPlOffset[0], static_cast<EGLAttrib>(offset));
        attribPush(kPlPitch[0], static_cast<EGLAttrib>(stride));
        if (hasModifiersExt) {
            // Modifier keys index EGLImage plane slots, not source planes; data sits in slot 0 so the modifier goes to
            // PLANE0 ([i] would push PLANE1+ attribs that AMD RADV rejects).
            attribPush(kPlModLo[0], modLo);
            attribPush(kPlModHi[0], modHi);
        }
        if (attribOverflow) {
            qCWarning(GstDmaBufLog) << "per-plane EGL attrib list overflow; CPU fallback";
            ok = false;
            break;
        }
        attribs[n++] = EGL_NONE;
        const bool cacheOn = imageCacheEnabled();
        // Per-plane key: same fd may back several planes (NV12 single-fd), so include geometry and layout.
        std::array<gsize, kMaxPlanes> planeOffsets{};
        std::array<int, kMaxPlanes> planeStrides{};
        planeOffsets[0] = offset;
        planeStrides[0] = stride;
        const DmaImageKey key{fd, drmModifier, planeWidth, planeHeight, fourcc, 1, planeOffsets, planeStrides};
        EGLImage image = EGL_NO_IMAGE_KHR;
        bool imageCached = false;
        if (cacheOn) {
            const QMutexLocker lock(&s_dmaCacheMutex);
            image = cachedDmaImageLocked(key, eglDpy);
            imageCached = (image != EGL_NO_IMAGE_KHR);
        }
        if (image == EGL_NO_IMAGE_KHR) {
            image = eglCreateImage(eglDpy, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, nullptr, attribs);
            if (cacheOn) {
                GstHwPathTelemetry::recordImageCacheMiss(HwVideoBufferPath::DmaBuf);
            }
        } else if (cacheOn) {
            GstHwPathTelemetry::recordImageCacheHit(HwVideoBufferPath::DmaBuf);
        }
        if (image == EGL_NO_IMAGE_KHR) {
            const EGLint eglErr = eglGetError();
            qCWarning(GstDmaBufLog) << "eglCreateImage failed plane" << i << "err" << Qt::hex << eglErr;
            if (eglErr == EGL_BAD_MATCH) {
                maybePermanentlyDisable(eglDpy, i, "per-plane");
            }
            ok = false;
            break;
        }
        functions.glBindTexture(GL_TEXTURE_2D, names[i]);
        bindEglImage(GL_TEXTURE_2D, image);
        if (Q_UNLIKELY(validateBind || GstDmaBufLog().isDebugEnabled())) {
            if (const GLenum glErr = functions.glGetError(); glErr != GL_NO_ERROR) {
                if (validateBind) {
                    QGC_HW_WARN_ONCE(GstDmaBufLog, s_loggedBindFailed,
                                     "per-plane EGLImage bind failed plane"
                                         << i << "glError=" << Qt::hex << glErr << "eglError=" << eglGetError()
                                         << "— CPU fallback");
                    // Evict/destroy the suspect image so it can't serve later frames.
                    if (cacheOn && imageCached) {
                        const QMutexLocker lock(&s_dmaCacheMutex);
                        s_dmaImageCache.eraseIf([&](const DmaImageKey& k, const DmaCachedImage&) { return k == key; });
                    } else {
                        eglDestroyImage(eglDpy, image);
                    }
                    ok = false;
                    break;
                }
                qCDebug(GstDmaBufLog) << "per-plane eglImageTargetTexture2D plane" << i << "glError=" << Qt::hex
                                      << glErr << "eglError=" << eglGetError();
            }
        }
        if (cacheOn && !imageCached) {
            const QMutexLocker lock(&s_dmaCacheMutex);
            insertDmaImageLocked(key, eglDpy, image, gst_buffer_peek_memory(buffer, memIdx));
        } else if (!cacheOn) {
            eglDestroyImage(eglDpy, image);
        }
    }

textures_built:

    if (ok && validateBind) {
        s_bindValidated.store(true, std::memory_order_relaxed);
    }
    if (!ok) {
        functions.glDeleteTextures(planeCount, names.data());
        return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
    }

    auto frameTextures =
        std::make_unique<FrameTextures>(&rhi, _format.frameSize(), _format.pixelFormat(), names, planeCount);
    // FrameTextures null-resets planes createFrom() rejects; a partial bundle renders black without bumping the failure
    // counter, so bail loudly instead.
    for (int i = 0; i < planeCount; ++i) {
        if (!frameTextures->texture(i)) {
            qCWarning(GstDmaBufLog) << "FrameTextures plane" << i << "createFrom failed — dropping frame";
            // ~FrameTextures glDeleteTextures via releaseGLTextures() — names are owned now.
            return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
        }
    }
    frameTextures->setSourceSample(takeSample());
    return frameTextures;
}

#endif  // QGC_HAS_GST_DMABUF_GPU_PATH
