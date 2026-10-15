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
#include "QGCRhiCapture.h"

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

#include <cerrno>
#include <poll.h>

#ifndef EGL_ANDROID_native_fence_sync
#define EGL_SYNC_NATIVE_FENCE_ANDROID 0x3144
#define EGL_SYNC_NATIVE_FENCE_FD_ANDROID 0x3145
#define EGL_NO_NATIVE_FENCE_FD_ANDROID -1
#endif

QGC_LOGGING_CATEGORY(GstDmaBufLog, "Video.GStreamer.HwBuffers.GstDmaBuf")

namespace {

using GstHw::kMaxPlanes;

GstHw::MapDiagnostics s_diag;

// EGL_BAD_MATCH from eglCreateImage = broken EGL/VA-API combo (QTBUG-112312); short-circuits mapTextures on first hit.
std::atomic<bool> s_permanentlyDisabled{false};
// A LINEAR single-fd BAD_MATCH only rules out the single-EGLImage import; later frames go straight to per-plane.
std::atomic<bool> s_singleFdBroken{false};

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

enum class EglImportStage
{
    SingleFd,
    PerPlane,
};

// Disable the path only once no import layout is left: tiled single-fd has no per-plane fallback.
void handleEglBadMatch(EGLDisplay eglDpy, int plane, EglImportStage stage, guint64 drmModifier)
{
    if (stage == EglImportStage::SingleFd && drmModifier == 0) {
        s_singleFdBroken.store(true, std::memory_order_relaxed);
        return;
    }
    maybePermanentlyDisable(eglDpy, plane, stage == EglImportStage::SingleFd ? "single-fd-tiled" : "per-plane");
}

// EGLDisplay is process-stable; cache the value (not a context pointer) to avoid dangling-key hazard.
std::atomic<EGLDisplay> s_cachedDisplay{EGL_NO_DISPLAY};
std::atomic<bool> s_loggedSingleFdDisabled{false};
std::atomic<bool> s_loggedProducerWaitTimeout{false};

// First-import-per-epoch bind validation: glGetError flushes the pipeline, so steady-state frames skip it.
std::atomic<bool> s_bindValidated{false};
std::atomic<bool> s_loggedBindFailed{false};

constexpr const char* kModifiersExt = "EGL_EXT_image_dma_buf_import_modifiers";

constexpr const char* kNativeFenceExt = "EGL_ANDROID_native_fence_sync";

// EGLImage cache (DEFAULT OFF, QGC_GST_DMABUF_CACHE=1): fd/GstMemory ABA means a recycled pool buffer can serve a
// stale image, so we key on fd plus the full EGL layout and evict from the GstMemory's finalize before the fd is
// reused.
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

struct DmaCachedImage
{
    EGLDisplay display = EGL_NO_DISPLAY;
    EGLImage image = EGL_NO_IMAGE_KHR;
    GstMemory* mem = nullptr;  // eviction key only; never dereferenced
};

constexpr std::size_t kImageCacheCapacity = 8;

void destroyCachedDmaImage(DmaCachedImage& entry)
{
    if (entry.image != EGL_NO_IMAGE_KHR && entry.display != EGL_NO_DISPLAY) {
        eglDestroyImage(entry.display, entry.image);
    }
}

// EGLImages are destroyed on a render thread with Qt's GL context current; an off-thread finalize notify or reset
// parks them until the next render-thread drain.
bool glContextCurrent(const DmaCachedImage&)
{
    return QOpenGLContext::currentContext() != nullptr;
}

QMutex s_dmaCacheMutex;
GstHw::GstHwImportCache<DmaImageKey, DmaCachedImage, kImageCacheCapacity> s_dmaImageCache{destroyCachedDmaImage,
                                                                                          glContextCurrent};

GQuark cachedMemoryQuark()
{
    static const GQuark quark = g_quark_from_static_string("qgc-dmabuf-eglimage-cache");
    return quark;
}

// qdata destroy-notify, run while the GstMemory finalizes on a pool/streaming thread (no GL context): the stale
// EGLImage is parked for render-thread destruction. Unlike a weak ref, the qdata is never removed from the render
// thread, which would mutate the notify list gst is iterating during finalize.
void onSourceMemoryFinalized(gpointer finalizedMem)
{
    const QMutexLocker lock(&s_dmaCacheMutex);
    s_dmaImageCache.eraseIf(
        [&](const DmaImageKey&, const DmaCachedImage& e) { return static_cast<gpointer>(e.mem) == finalizedMem; });
}

EGLImage cachedDmaImageLocked(const DmaImageKey& key, EGLDisplay eglDpy)
{
    if (const auto hit = s_dmaImageCache.find(key);
        hit && hit.resource->display == eglDpy && hit.resource->image != EGL_NO_IMAGE_KHR) {
        return hit.resource->image;
    }
    return EGL_NO_IMAGE_KHR;
}

void insertDmaImageLocked(const DmaImageKey& key, EGLDisplay eglDpy, EGLImage image, GstMemory* mem)
{
    if (mem && !gst_mini_object_get_qdata(GST_MINI_OBJECT(mem), cachedMemoryQuark())) {
        gst_mini_object_set_qdata(GST_MINI_OBJECT(mem), cachedMemoryQuark(), mem, onSourceMemoryFinalized);
    }
    s_dmaImageCache.insert(key, DmaCachedImage{eglDpy, image, mem});
}

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
        : GstGlFrameTextures(rhi, size, pixelFormat, names, count)
    {}

    ~FrameTextures() override
    {
        if (!_rhi || _count == 0)
            return;
        // Bind the QRhi GL context so currentContext() returns it (mirrors ~QGstQVideoFrameTextures), avoiding the
        // backend-conditional NativeHandles cast.
        _rhi->makeThreadLocalNativeContextCurrent();
        if (QOpenGLContext* ctx = QOpenGLContext::currentContext()) {
            ctx->functions()->glDeleteTextures(int(_count), _names.data());
        }
    }
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
// that makes the CPU barrier redundant; any failure leaves the caller's existing barrier path untouched.
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
                     "DMABuf: using producer dma-buf fence for GPU-side sync (CPU barrier skipped)");
    GstHwPathTelemetry::recordExplicitFenceWait(HwVideoBufferPath::DmaBuf);
    GstHwPathTelemetry::recordSyncWait(HwVideoBufferPath::DmaBuf, /*gpuSide=*/true);
    return true;
#else
    Q_UNUSED(eglDpy);
    Q_UNUSED(dmaFd);
    return false;
#endif
}

// Bounded so a GPU hang cannot wedge Qt's render thread.
constexpr int kProducerWaitTimeoutMs = 100;

// CPU-side barrier on the producer's implicit write fences: poll(POLLIN) on a dma-buf fd returns once its write fences
// signal (the wait linux/dma-buf.h documents). Unlike mmap/DMA_BUF_IOCTL_SYNC it triggers no exporter cache maintenance
// or buffer migration. False when a wait timed out or failed.
bool waitForProducerWrites(GstBuffer* buffer)
{
    int lastFd = -1;
    for (guint i = 0; i < gst_buffer_n_memory(buffer); ++i) {
        GstMemory* mem = gst_buffer_peek_memory(buffer, i);
        const int fd = (mem && gst_is_dmabuf_memory(mem)) ? gst_dmabuf_memory_get_fd(mem) : -1;
        if (fd < 0 || fd == lastFd) {
            continue;
        }
        lastFd = fd;
        pollfd pfd = {fd, POLLIN, 0};
        int ret = 0;
        do {
            ret = ::poll(&pfd, 1, kProducerWaitTimeoutMs);
        } while (ret < 0 && (errno == EINTR || errno == EAGAIN));
        if (ret <= 0) {
            const int pollError = (ret < 0) ? errno : 0;
            GstHwPathTelemetry::recordFenceTimeout(HwVideoBufferPath::DmaBuf);
            QGC_HW_WARN_ONCE(GstDmaBufLog, s_loggedProducerWaitTimeout,
                             "dma-buf producer fence wait" << (ret == 0 ? "timed out" : "failed") << "errno="
                                                           << pollError << "— no producer barrier for this frame");
            return false;
        }
    }
    GstHwPathTelemetry::recordSyncWait(HwVideoBufferPath::DmaBuf, /*gpuSide=*/false);
    GstHwPathTelemetry::recordMmapBarrierHit(HwVideoBufferPath::DmaBuf);
    return true;
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
    s_singleFdBroken.store(false, std::memory_order_release);
    s_loggedSingleFdDisabled.store(false, std::memory_order_release);
#if defined(QGC_HAS_GST_VULKAN_GPU_PATH)
    GstDmaBufVulkan::resetCachedState();
#endif
    s_loggedProducerWaitTimeout.store(false, std::memory_order_release);
    s_bindValidated.store(false, std::memory_order_release);
    s_loggedBindFailed.store(false, std::memory_order_release);
    // Destroyed now on a render-thread reset (e.g. sceneGraphInvalidated); otherwise parked for the next mapTextures().
    const QMutexLocker lock(&s_dmaCacheMutex);
    s_dmaImageCache.clear();
    s_dmaImageCache.drainDeferred();
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

void GstDmaBufVideoBuffer::recordEglBadMatchForTest(bool singleFd, guint64 drmModifier) noexcept
{
    handleEglBadMatch(EGL_NO_DISPLAY, 0, singleFd ? EglImportStage::SingleFd : EglImportStage::PerPlane, drmModifier);
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
#if defined(QGC_HAS_GST_VULKAN_GPU_PATH)
    if (QGCRhiCapture::deviceSnapshot().backend.load(std::memory_order_acquire) == static_cast<int>(QRhi::Vulkan) &&
        !GstDmaBufVulkan::canImport(_videoInfo)) {
        return false;
    }
#endif
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

QVideoFrameTexturesUPtr GstDmaBufVideoBuffer::mapTextures(QRhi& rhi, QVideoFrameTexturesUPtr& old)
{
    const GstHwPathTelemetry::ScopedMapTimer mapTimer(HwVideoBufferPath::DmaBuf);
#if defined(QGC_HAS_GST_VULKAN_GPU_PATH)
    const bool vulkan = (rhi.backend() == QRhi::Vulkan);
#else
    constexpr bool vulkan = false;
#endif
    GstBuffer* buffer = nullptr;
    if (!checkMapPreconditions(rhi, static_cast<int>(vulkan ? QRhi::Vulkan : QRhi::OpenGLES2), GstDmaBufLog(), s_diag,
                               buffer)) {
        return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
    }

    // Bail once a prior frame hit EGL_BAD_MATCH (broken EGL/VA-API, QTBUG-112312); retrying stalls the pipeline.
    if (s_permanentlyDisabled.load(std::memory_order_relaxed)) {
        return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
    }

    // Modifier cached from caps in the ctor (0 = LINEAR / non-DRM / normalized INVALID).
    const guint64 drmModifier = _drmModifier;

    // Producer barrier for LINEAR imports (Intel iHD legacy LINEAR is not implicitly fenced by the EGL import); runs
    // below with the GL context current. QGC_GST_DMABUF_NO_MMAP_FENCE=1 skips it where PRIME 2 fence FDs make it
    // redundant.
    const bool skipLinearFence = HwBuffers::hwBufferEnvConfig().dmaBufNoMmapFence;
    const bool needLinearFence = (drmModifier == 0 /* DRM_FORMAT_MOD_LINEAR / unknown */ && !skipLinearFence);

#if defined(QGC_HAS_GST_VULKAN_GPU_PATH)
    if (vulkan) {
        // Vulkan does not wait on implicit fences of imported dma-buf memory, so import only after the producer's
        // writes are confirmed complete (same policy as GstVulkanVideoBuffer: no import without producer sync).
        if (!waitForProducerWrites(buffer)) {
            return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
        }
        return importVulkan(rhi, old);
    }
#endif

    QOpenGLContext* glContext = GstEglHelpers::bindRhiContext(rhi);

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

    if (!glContext) {
        qCWarning(GstDmaBufLog) << "QRhi exposes no GL context";
        return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
    }

    // EGLDisplay is process-stable; resolve once per epoch (resetCachedState). First writer wins; racing renders that
    // resolve the same display make this idempotent.
    EGLDisplay eglDpy = s_cachedDisplay.load(std::memory_order_acquire);
    if (eglDpy == EGL_NO_DISPLAY) {
        eglDpy = GstEglHelpers::renderDisplay(glContext, _eglDisplay);
        if (eglDpy == EGL_NO_DISPLAY) {
            return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
        }
        EGLDisplay expected = EGL_NO_DISPLAY;
        s_cachedDisplay.compare_exchange_strong(expected, eglDpy, std::memory_order_release);
    }

    // Render thread, GL context current: destroy EGLImages parked by off-thread finalize notifies or resets.
    if (imageCacheEnabled()) {
        const QMutexLocker lock(&s_dmaCacheMutex);
        s_dmaImageCache.drainDeferred();
    }

    if (!gst_is_dmabuf_memory(gst_buffer_peek_memory(buffer, 0))) {
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

    // Prefer a GPU-side wait on the producer's exported fence; otherwise block on its implicit write fences.
    if (needLinearFence && !tryExplicitFenceWait(eglDpy, gst_dmabuf_memory_get_fd(gst_buffer_peek_memory(buffer, 0)))) {
        waitForProducerWrites(buffer);
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
    QOpenGLFunctions functions(glContext);
    functions.glGenTextures(planeCount, names.data());

    // Modifier attribs only with the ext present; otherwise impl assumes LINEAR (matches our rejection above).
    const EGLAttrib modLo = static_cast<EGLAttrib>(drmModifier & 0xFFFFFFFFu);
    const EGLAttrib modHi = static_cast<EGLAttrib>((drmModifier >> 32) & 0xFFFFFFFFu);

    bool ok = true;

    // Single-fd fast path: one EGLImage for all planes (Mesa VA-API NV12 ships memCount==1; iHD rejects split-plane
    // R8/GR88 imports). Skipped once a LINEAR single-fd import hit EGL_BAD_MATCH.
    if (singleFdPacking && !s_singleFdBroken.load(std::memory_order_relaxed)) {
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
                handleEglBadMatch(eglDpy, 0, EglImportStage::SingleFd, drmModifier);
            }
            ok = false;
            goto textures_built;
        }
        qCWarning(GstDmaBufLog) << "single-fd eglCreateImage failed (" << Qt::hex << singleFdErr
                                << "); falling back to per-plane";
        if (singleFdErr == EGL_BAD_MATCH) {
            handleEglBadMatch(eglDpy, 0, EglImportStage::SingleFd, drmModifier);
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
                handleEglBadMatch(eglDpy, i, EglImportStage::PerPlane, drmModifier);
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

    // FrameTextures owns the names from here: its dtor deletes them if a plane failed to wrap.
    const GstHwMapTail tail{.path = HwVideoBufferPath::DmaBuf,
                            .frameSize = _format.frameSize(),
                            .pixelFormat = _format.pixelFormat(),
                            .log = &GstDmaBufLog(),
                            .diag = &s_diag,
                            .preflight = false};
    return GstHwFrameTexturesBase::finishMap<FrameTextures>(*this, rhi, old, tail, nullptr, [&] {
        return std::make_unique<FrameTextures>(&rhi, tail.frameSize, tail.pixelFormat, names, planeCount);
    });
}

#endif  // QGC_HAS_GST_DMABUF_GPU_PATH
