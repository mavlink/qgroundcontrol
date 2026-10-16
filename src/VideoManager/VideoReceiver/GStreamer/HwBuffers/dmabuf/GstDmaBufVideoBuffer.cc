#include "GstDmaBufVideoBuffer.h"

#include "GstDmaBufVulkanImport.h"
#include "GstDmaFourcc.h"
#include "GstEglHelpers.h"
#include "GstGlFrameTextures.h"
#include "GstHwImportCache.h"
#include "GstHwImportPreflight.h"
#include "GstHwPathRegistry.h"
#include "GstHwPathTelemetry.h"
#include "GstHwVideoBuffer.h"
#include "GstHwVideoInfo.h"
#include "HwBuffers.h"
#include "QGCRhiCapture.h"

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)

#include <gst/allocators/gstdmabuf.h>
#include <gst/video/video.h>
#include <private/qvideotexturehelper_p.h>
#include <rhi/qrhi.h>

#include <QtCore/QByteArray>
#include <QtCore/QLoggingCategory>
#include <QtCore/QMutex>
#include <QtCore/QSize>
#include <QtGui/QGuiApplication>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>
#include <QtGui/qopenglcontext_platform.h>
#include <qpa/qplatformnativeinterface.h>

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
#include <span>

#ifndef EGL_ANDROID_native_fence_sync
#define EGL_SYNC_NATIVE_FENCE_ANDROID 0x3144
#define EGL_SYNC_NATIVE_FENCE_FD_ANDROID 0x3145
#define EGL_NO_NATIVE_FENCE_FD_ANDROID -1
#endif

QGC_LOGGING_CATEGORY(GstDmaBufLog, "Video.GStreamer.HwBuffers.GstDmaBuf")

namespace {

using GstHw::kMaxPlanes;

GstHw::MapDiagnostics s_diag;

// EGL_BAD_MATCH from eglCreateImage = broken EGL/VA-API combo (QTBUG-112312); set once no import layout is left (see
// disableAfterPerPlaneBadMatch), then short-circuits mapTextures and validatePlaneHandles.
std::atomic<bool> s_permanentlyDisabled{false};
// A single-fd BAD_MATCH only rules out the single-EGLImage import; later frames go straight to per-plane.
std::atomic<bool> s_singleFdBroken{false};

// A per-plane BAD_MATCH leaves no import layout to fall back to, so it disables the path.
void disableAfterPerPlaneBadMatch(EGLDisplay eglDpy, int plane)
{
    if (!s_permanentlyDisabled.exchange(true, std::memory_order_relaxed)) {
        GstHwPathTelemetry::recordFallbackReason(HwVideoBufferPath::DmaBuf,
                                                 GstHwPathTelemetry::HwFallbackReason::EglBadMatch);
        qCWarning(GstDmaBufLog) << "eglCreateImage returned EGL_BAD_MATCH on per-plane plane" << plane
                                << "— disabling DMABuf zero-copy for this process"
                                << "(EGL_VENDOR=" << eglQueryString(eglDpy, EGL_VENDOR)
                                << "EGL_VERSION=" << eglQueryString(eglDpy, EGL_VERSION) << ")";
    }
}

// EGLDisplay is process-stable; cache the value (not a context pointer) to avoid dangling-key hazard.
std::atomic<EGLDisplay> s_cachedDisplay{EGL_NO_DISPLAY};
std::atomic<bool> s_loggedSingleFdDisabled{false};
std::atomic<bool> s_loggedProducerWaitTimeout{false};
std::atomic<bool> s_loggedVulkanSyncRefused{false};

// First-import-per-epoch bind validation: glGetError flushes the pipeline, so steady-state frames skip it.
std::atomic<bool> s_bindValidated{false};
std::atomic<bool> s_loggedBindFailed{false};

constexpr const char* kModifiersExt = "EGL_EXT_image_dma_buf_import_modifiers";

constexpr const char* kNativeFenceExt = "EGL_ANDROID_native_fence_sync";

// EGLImage cache (QGC_GST_DMABUF_CACHE=1). Keyed on fd plus the full EGL layout and evicted when the GstMemory
// finalizes, so a reused fd number never hits a dead buffer's image. Off by default: it is untested (only the
// QGC_GST_GPU_BIN=dmabuf bin reaches this importer, and CI has no DMABuf producer with a GL QRhi).
bool imageCacheEnabled() noexcept
{
    return HwBuffers::hwBufferEnvConfig().dmaBufCache;
}

bool texStorageImportAllowed(bool contextIsOpenGles, bool hasTexStorageExt) noexcept
{
    return !contextIsOpenGles && hasTexStorageExt;
}

// LINEAR only, even with EGL_EXT_image_dma_buf_import_modifiers: binding a tiled VA EGLImage can segfault Mesa/Gallium.
bool directGlImportAllowed(guint64 drmModifier) noexcept
{
    return drmModifier == 0;
}

struct DmaImageKey
{
    int fd = -1;
    int width = 0;
    int height = 0;
    int fourcc = 0;
    int planeCount = 0;
    std::array<gsize, kMaxPlanes> offsets{};
    std::array<int, kMaxPlanes> strides{};

    bool operator==(const DmaImageKey& o) const noexcept
    {
        return fd == o.fd && width == o.width && height == o.height && fourcc == o.fourcc &&
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

using DmaImageCache = GstHw::GstHwImportCache<DmaImageKey, DmaCachedImage, kImageCacheCapacity>;

QMutex s_dmaCacheMutex;
DmaImageCache s_dmaImageCache{destroyCachedDmaImage, glContextCurrent};

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
constexpr EGLAttrib kLinearModifierLo = static_cast<EGLAttrib>(DRM_FORMAT_MOD_LINEAR & 0xFFFFFFFFu);
constexpr EGLAttrib kLinearModifierHi = static_cast<EGLAttrib>((DRM_FORMAT_MOD_LINEAR >> 32) & 0xFFFFFFFFu);

/// Fixed-capacity, EGL_NONE-terminated eglCreateImage attribute list; sized for the single-fd worst case (3 header
/// keys plus FD/OFFSET/PITCH/MODIFIER_LO/MODIFIER_HI for every plane).
class EglAttribList
{
public:
    void push(EGLAttrib key, EGLAttrib value) noexcept
    {
        if (_size + 2 >= _attribs.size()) {
            _overflowed = true;
            return;
        }
        _attribs[_size++] = key;
        _attribs[_size++] = value;
    }

    /// Terminates the list; false when a push() did not fit.
    bool finish() noexcept
    {
        _attribs[_size] = EGL_NONE;
        return !_overflowed;
    }

    const EGLAttrib* data() const noexcept { return _attribs.data(); }

private:
    std::array<EGLAttrib, 2 * (3 + kMaxPlanes * 5) + 1> _attribs{};
    std::size_t _size = 0;
    bool _overflowed = false;
};

/// An EGLImage held across one bind: pinned in the image cache when it came from there, else owned by the caller.
struct AcquiredImage
{
    EGLImage image = EGL_NO_IMAGE_KHR;
    DmaImageCache::EntryId cacheEntry = 0;
};

/// Cached image for @p key (pinned so a concurrent reset can't destroy it mid-bind) or a fresh import; image is
/// EGL_NO_IMAGE_KHR when eglCreateImage failed.
AcquiredImage acquireImage(EGLDisplay eglDpy, const DmaImageKey& key, const EglAttribList& attribs)
{
    const bool cacheOn = imageCacheEnabled();
    if (cacheOn) {
        const QMutexLocker lock(&s_dmaCacheMutex);
        if (const auto hit = s_dmaImageCache.find(key); hit && hit.resource->display == eglDpy) {
            s_dmaImageCache.pin(hit.id);
            GstHwPathTelemetry::record(HwVideoBufferPath::DmaBuf, GstHwPathTelemetry::Counter::ImageCacheHits);
            return {hit.resource->image, hit.id};
        }
    }
    const EGLImage image = eglCreateImage(eglDpy, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, nullptr, attribs.data());
    if (cacheOn) {
        GstHwPathTelemetry::record(HwVideoBufferPath::DmaBuf, GstHwPathTelemetry::Counter::ImageCacheMisses);
    }
    return {image, 0};
}

/// Ends the bind. A failed bind evicts/destroys the suspect image so it can't serve later frames; otherwise a fresh
/// image is cached under @p key until @p mem finalizes (or destroyed when caching is off).
void releaseImage(EGLDisplay eglDpy, const DmaImageKey& key, const AcquiredImage& acquired, GstMemory* mem,
                  bool bindFailed)
{
    if (acquired.cacheEntry != 0) {
        const QMutexLocker lock(&s_dmaCacheMutex);
        if (bindFailed) {
            s_dmaImageCache.eraseIf([&](const DmaImageKey& k, const DmaCachedImage&) { return k == key; });
        }
        s_dmaImageCache.unpin(acquired.cacheEntry);
        s_dmaImageCache.drainDeferred();
        return;
    }
    if (bindFailed || !imageCacheEnabled()) {
        eglDestroyImage(eglDpy, acquired.image);
        return;
    }
    if (mem && !gst_mini_object_get_qdata(GST_MINI_OBJECT(mem), cachedMemoryQuark())) {
        gst_mini_object_set_qdata(GST_MINI_OBJECT(mem), cachedMemoryQuark(), mem, onSourceMemoryFinalized);
    }
    const QMutexLocker lock(&s_dmaCacheMutex);
    s_dmaImageCache.insert(key, DmaCachedImage{eglDpy, acquired.image, mem});
}

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
    GstHwPathTelemetry::record(HwVideoBufferPath::DmaBuf, GstHwPathTelemetry::Counter::GpuWaits);
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
            GstHwPathTelemetry::record(HwVideoBufferPath::DmaBuf, GstHwPathTelemetry::Counter::ProducerFenceTimeouts);
            QGC_HW_WARN_ONCE(GstDmaBufLog, s_loggedProducerWaitTimeout,
                             "dma-buf producer fence wait" << (ret == 0 ? "timed out" : "failed") << "errno="
                                                           << pollError << "— no producer barrier for this frame");
            return false;
        }
    }
    GstHwPathTelemetry::record(HwVideoBufferPath::DmaBuf, GstHwPathTelemetry::Counter::CpuWaits);
    return true;
}

// EGLDisplay is process-stable; resolve once per epoch (resetCachedState). First writer wins; racing renders that
// resolve the same display make this idempotent.
EGLDisplay resolveRenderDisplay(QOpenGLContext* glContext, EGLDisplay fallback)
{
    EGLDisplay eglDpy = s_cachedDisplay.load(std::memory_order_acquire);
    if (eglDpy != EGL_NO_DISPLAY) {
        return eglDpy;
    }
    eglDpy = GstEglHelpers::renderDisplay(glContext, fallback);
    if (eglDpy != EGL_NO_DISPLAY) {
        EGLDisplay expected = EGL_NO_DISPLAY;
        s_cachedDisplay.compare_exchange_strong(expected, eglDpy, std::memory_order_release);
    }
    return eglDpy;
}

// Prefer a GPU-side wait on the producer's exported fence; otherwise block on its implicit write fences.
void waitForProducer(EGLDisplay eglDpy, GstBuffer* buffer)
{
    if (!tryExplicitFenceWait(eglDpy, gst_dmabuf_memory_get_fd(gst_buffer_peek_memory(buffer, 0)))) {
        waitForProducerWrites(buffer);
    }
}

/// Binds an EGLImage to the GL texture currently bound at a target.
struct EglImageBinder
{
    PFNGLEGLIMAGETARGETTEXTURE2DOESPROC targetTexture2D = nullptr;
    PFNGLEGLIMAGETARGETTEXSTORAGEEXTPROC targetTexStorage = nullptr;

    void bind(GLenum target, EGLImage image) const
    {
        if (targetTexStorage) {
            targetTexStorage(target, image, nullptr);
        } else {
            targetTexture2D(target, image);
        }
    }
};

EglImageBinder eglImageBinder(const QOpenGLContext& ctx)
{
    static const auto targetTexStorage =
        reinterpret_cast<PFNGLEGLIMAGETARGETTEXSTORAGEEXTPROC>(eglGetProcAddress("glEGLImageTargetTexStorageEXT"));
    // Desktop GL with GL_EXT_EGL_image_storage: bind DMABuf via immutable storage to skip the per-frame texture-storage
    // realloc the mutable OES path forces (OBS measured ~100x on discrete Intel). GLES keeps the OES path: the storage
    // ext isn't reliably exposed and the OES bind is the contract there.
    const bool useTexStorage =
        texStorageImportAllowed(ctx.isOpenGLES(), ctx.hasExtension(QByteArrayLiteral("GL_EXT_EGL_image_storage")));
    return {GstEglHelpers::imageTargetTexture2D(), useTexStorage ? targetTexStorage : nullptr};
}

/// Per-frame state shared by the single-fd and per-plane EGLImage imports. Both import LINEAR buffers only:
/// mapTextures() rejects any other modifier first.
struct EglImport
{
    EGLDisplay display = EGL_NO_DISPLAY;
    GstBuffer* buffer = nullptr;
    const GstVideoInfo* info = nullptr;
    const GstVideoMeta* vmeta = nullptr;
    bool hasModifiersExt = false;
    // Validate binds with glGetError only on the first import per epoch: it flushes the pipeline.
    bool validateBind = false;
    QOpenGLFunctions* gl = nullptr;
    EglImageBinder binder{};
    std::span<const GLuint> names{};

    int fdOf(int memIdx) const { return gst_dmabuf_memory_get_fd(gst_buffer_peek_memory(buffer, guint(memIdx))); }
};

enum class ImportResult
{
    Ok,
    TryPerPlane,
    Fail,
};

bool plausibleLayout(gsize offset, gint stride)
{
    if (stride > 0 && static_cast<quint64>(offset) <= static_cast<quint64>((std::numeric_limits<EGLAttrib>::max)())) {
        return true;
    }
    qCWarning(GstDmaBufLog) << "implausible plane stride/offset (stride=" << stride << "offset=" << offset
                            << "); CPU fallback";
    return false;
}

bool bindPlane(const EglImport& ctx, int plane, EGLImage image, const char* stage)
{
    ctx.gl->glBindTexture(GL_TEXTURE_2D, ctx.names[std::size_t(plane)]);
    ctx.binder.bind(GL_TEXTURE_2D, image);
    // glGetError forces a pipeline flush — only first import per epoch or when category debug is on.
    if (Q_LIKELY(!ctx.validateBind && !GstDmaBufLog().isDebugEnabled())) {
        return true;
    }
    const GLenum glErr = ctx.gl->glGetError();
    if (glErr == GL_NO_ERROR) {
        return true;
    }
    if (ctx.validateBind) {
        QGC_HW_WARN_ONCE(GstDmaBufLog, s_loggedBindFailed,
                         stage << "EGLImage bind failed plane" << plane << "glError=" << Qt::hex << glErr
                               << "eglError=" << eglGetError() << "— CPU fallback");
        return false;
    }
    qCDebug(GstDmaBufLog) << stage << "eglImageTargetTexture2D plane" << plane << "glError=" << Qt::hex << glErr
                          << "eglError=" << eglGetError();
    return true;
}

// Y210/Y410 have no per-plane fallback; warn once that frames will be black.
void warnUnsupportedPackedFormat([[maybe_unused]] int fourcc)
{
#if defined(DRM_FORMAT_Y210)
    static std::atomic<bool> s_warnedY210{false};
    if (fourcc == DRM_FORMAT_Y210) {
        QGC_HW_WARN_ONCE(GstDmaBufLog, s_warnedY210,
                         "EGL DMABuf import of Y210 unsupported on this driver; frames will be black");
    }
#endif
#if defined(DRM_FORMAT_Y410)
    static std::atomic<bool> s_warnedY410{false};
    if (fourcc == DRM_FORMAT_Y410) {
        QGC_HW_WARN_ONCE(GstDmaBufLog, s_warnedY410,
                         "EGL DMABuf import of Y410 unsupported on this driver; frames will be black");
    }
#endif
}

/// One EGLImage over every plane of a single-fd buffer, bound to each plane texture (Mesa VA-API NV12 ships one fd;
/// iHD rejects split-plane R8/GR88 imports).
ImportResult importSingleFd(const EglImport& ctx, int fourcc)
{
    const int planeCount = int(ctx.names.size());
    const int fd0 = ctx.fdOf(0);
    DmaImageKey key{.fd = fd0,
                    .width = GST_VIDEO_INFO_WIDTH(ctx.info),
                    .height = GST_VIDEO_INFO_HEIGHT(ctx.info),
                    .fourcc = fourcc,
                    .planeCount = planeCount};
    EglAttribList attribs;
    attribs.push(EGL_WIDTH, key.width);
    attribs.push(EGL_HEIGHT, key.height);
    attribs.push(EGL_LINUX_DRM_FOURCC_EXT, fourcc);
    for (int p = 0; p < planeCount; ++p) {
        const auto [offset, stride] = GstHw::planeLayout(*ctx.info, ctx.vmeta, p);
        if (!plausibleLayout(offset, stride)) {
            return ImportResult::Fail;
        }
        key.offsets[p] = offset;
        key.strides[p] = stride;
        attribs.push(kPlFd[p], fd0);
        attribs.push(kPlOffset[p], static_cast<EGLAttrib>(offset));
        attribs.push(kPlPitch[p], static_cast<EGLAttrib>(stride));
        if (ctx.hasModifiersExt) {  // interleaved per spec: modifier attribs follow FD/OFFSET/PITCH for same plane
            attribs.push(kPlModLo[p], kLinearModifierLo);
            attribs.push(kPlModHi[p], kLinearModifierHi);
        }
    }
    if (!attribs.finish()) {
        qCWarning(GstDmaBufLog) << "single-fd EGL attrib list overflow; falling back to per-plane";
        return ImportResult::TryPerPlane;
    }

    const AcquiredImage acquired = acquireImage(ctx.display, key, attribs);
    if (acquired.image == EGL_NO_IMAGE_KHR) {
        warnUnsupportedPackedFormat(fourcc);
        const EGLint err = eglGetError();
        qCWarning(GstDmaBufLog) << "single-fd eglCreateImage failed (" << Qt::hex << err
                                << "); falling back to per-plane";
        if (err == EGL_BAD_MATCH) {
            s_singleFdBroken.store(true, std::memory_order_relaxed);
        }
        return ImportResult::TryPerPlane;
    }

    bool bound = true;
    for (int p = 0; p < planeCount && bound; ++p) {
        bound = bindPlane(ctx, p, acquired.image, "single-fd");
    }
    releaseImage(ctx.display, key, acquired, gst_buffer_peek_memory(ctx.buffer, 0), !bound);
    return bound ? ImportResult::Ok : ImportResult::Fail;
}

/// One EGLImage per plane; @p sharedFd when every plane lives in memory 0 at its own offset (single-fd packing).
bool importPerPlane(const EglImport& ctx, bool sharedFd)
{
    for (int i = 0; i < int(ctx.names.size()); ++i) {
        const int memIdx = sharedFd ? 0 : i;
        const int fd = ctx.fdOf(memIdx);
        const auto [planeOffset, stride] = GstHw::planeLayout(*ctx.info, ctx.vmeta, i);
        // Dedicated-fd planes already start at the plane data; the offset only applies to single-fd packing.
        const gsize offset = sharedFd ? planeOffset : 0;
        if (!plausibleLayout(offset, stride)) {
            return false;
        }
        const int fourcc = GstHw::drmFourccForPlane(*ctx.info, i);
        if (fourcc == -1) {
            qCWarning(GstDmaBufLog) << "no DRM fourcc for format"
                                    << gst_video_format_to_string(GST_VIDEO_INFO_FORMAT(ctx.info));
            return false;
        }

        // Per-plane key: same fd may back several planes (NV12 single-fd), so include geometry and layout.
        DmaImageKey key{.fd = fd,
                        .width = GstHw::planeWidth(*ctx.info, i),
                        .height = GstHw::planeHeight(*ctx.info, i),
                        .fourcc = fourcc,
                        .planeCount = 1};
        key.offsets[0] = offset;
        key.strides[0] = stride;
        EglAttribList attribs;
        attribs.push(EGL_WIDTH, key.width);
        attribs.push(EGL_HEIGHT, key.height);
        attribs.push(EGL_LINUX_DRM_FOURCC_EXT, fourcc);
        attribs.push(kPlFd[0], fd);
        attribs.push(kPlOffset[0], static_cast<EGLAttrib>(offset));
        attribs.push(kPlPitch[0], static_cast<EGLAttrib>(stride));
        if (ctx.hasModifiersExt) {
            // Modifier keys index EGLImage plane slots, not source planes; data sits in slot 0 so the modifier goes to
            // PLANE0 ([i] would push PLANE1+ attribs that AMD RADV rejects).
            attribs.push(kPlModLo[0], kLinearModifierLo);
            attribs.push(kPlModHi[0], kLinearModifierHi);
        }
        if (!attribs.finish()) {
            qCWarning(GstDmaBufLog) << "per-plane EGL attrib list overflow; CPU fallback";
            return false;
        }

        const AcquiredImage acquired = acquireImage(ctx.display, key, attribs);
        if (acquired.image == EGL_NO_IMAGE_KHR) {
            const EGLint err = eglGetError();
            qCWarning(GstDmaBufLog) << "eglCreateImage failed plane" << i << "err" << Qt::hex << err;
            if (err == EGL_BAD_MATCH) {
                disableAfterPerPlaneBadMatch(ctx.display, i);
            }
            return false;
        }
        const bool bound = bindPlane(ctx, i, acquired.image, "per-plane");
        releaseImage(ctx.display, key, acquired, gst_buffer_peek_memory(ctx.buffer, guint(memIdx)), !bound);
        if (!bound) {
            return false;
        }
    }
    return true;
}

}  // namespace

GstDmaBufVideoBuffer::GstDmaBufVideoBuffer(GstSample* sample, const GstVideoInfo& videoInfo,
                                           const QVideoFrameFormat& format, EGLDisplay eglDisplay)
    : GstHwVideoBuffer(QVideoFrame::RhiTextureHandle, sample, videoInfo, format)
    , _eglDisplay(eglDisplay)
{
    // Advertised for the EGLImage import only; the base class keeps the memory's format, so map() refuses the CPU
    // upload Qt would otherwise read as RGBA.
    _format = applyEglfsFormatQuirk(_format);
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

// Construction-time hint only; render-time mapTextures() prefers eglGetCurrentDisplay() to avoid xcb_egl EGLDisplay
// mismatches.
void prepareDmaBufContext(HwVideoBufferContext& context)
{
    EGLDisplay dpy = EGL_NO_DISPLAY;
    const QString platform = QGuiApplication::platformName();
    if (platform == QLatin1String("wayland") || platform == QLatin1String("wayland-egl")) {
        if (auto* ni = QGuiApplication::platformNativeInterface()) {
            dpy = static_cast<EGLDisplay>(ni->nativeResourceForIntegration("egldisplay"));
        }
    }
    if (dpy == EGL_NO_DISPLAY) {
        dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    }
    if (dpy == EGL_NO_DISPLAY) {
        qCWarning(GstDmaBufLog) << "GPU zero-copy requested but EGLDisplay unavailable on platform" << platform
                                << "— DMABuf path disabled";
    } else {
        qCInfo(GstDmaBufLog) << "DMABuf zero-copy path available on" << platform
                             << "— actual path chosen at caps negotiation";
    }
    context.dmaBufEglDisplay = dpy;
}

}  // namespace

constinit const HwPathDescriptor GstHwPaths::kDmaBuf = {
    .path = HwVideoBufferPath::DmaBuf,
    .matches = [](GstMemory* mem, const HwVideoBufferContext& context) -> bool {
        return (context.dmaBufEglDisplay != EGL_NO_DISPLAY) && gst_is_dmabuf_memory(mem);
    },
    .construct = [](GstSample* sample, const GstVideoInfo& info, QVideoFrameFormat format,
                    const HwVideoBufferContext& context) -> std::unique_ptr<GstHwVideoBuffer> {
        return std::make_unique<GstDmaBufVideoBuffer>(sample, info, format, context.dmaBufEglDisplay);
    },
    .prepareContext = &prepareDmaBufContext,
    .resetCaches =
        [] {
            GstDmaBufVideoBuffer::resetCachedState();
            GstEglHelpers::resetExtensionCache();
        },
};

#ifdef QGC_UNITTEST_BUILD
bool GstDmaBufVideoBuffer::texStorageImportAllowedForTest(bool contextIsOpenGles, bool hasTexStorageExt) noexcept
{
    return texStorageImportAllowed(contextIsOpenGles, hasTexStorageExt);
}

bool GstDmaBufVideoBuffer::directGlImportAllowedForTest(guint64 drmModifier) noexcept
{
    return directGlImportAllowed(drmModifier);
}

void GstDmaBufVideoBuffer::recordEglBadMatchForTest(bool singleFd) noexcept
{
    if (singleFd) {
        s_singleFdBroken.store(true, std::memory_order_relaxed);
    } else {
        disableAfterPerPlaneBadMatch(EGL_NO_DISPLAY, 0);
    }
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

#if defined(QGC_HAS_GST_VULKAN_GPU_PATH)
    if (vulkan) {
        // Vulkan does not wait on implicit fences of imported dma-buf memory: import only once the poll() barrier
        // confirms the producer's writes completed.
        if (!waitForProducerWrites(buffer)) {
            GstHwPathTelemetry::recordFallbackReason(HwVideoBufferPath::DmaBuf,
                                                     GstHwPathTelemetry::HwFallbackReason::VulkanNoSync);
            QGC_HW_WARN_ONCE(GstDmaBufLog, s_loggedVulkanSyncRefused,
                             "Vulkan import refused: producer writes not known complete — CPU fallback");
            return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
        }
        return importVulkan(rhi, old);
    }
#endif

    QOpenGLContext* glContext = GstEglHelpers::bindRhiContext(rhi);

    // Mirror Qt's QGstVideoBuffer::mapTextures() (logGlAndEglErrors) — surface latent GL/EGL state from prior frames.
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

    const EGLDisplay eglDpy = resolveRenderDisplay(glContext, _eglDisplay);
    if (eglDpy == EGL_NO_DISPLAY) {
        return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
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
    if (!directGlImportAllowed(_drmModifier)) {
        // Reaching here means caps/filtering changed between the validate and render threads or an upstream element
        // pushed a modifier layout we do not advertise. Fall back before either GL import path can bind the EGLImage.
        static std::atomic<bool> warnedModifierRejected{false};
        if (!warnedModifierRejected.exchange(true, std::memory_order_relaxed)) {
            qCWarning(GstDmaBufLog) << "DMABuf modifier" << Qt::hex << _drmModifier << "rejected for direct GL import"
                                    << (hasModifiersExt ? "(EGL modifier import present but disabled for safety)"
                                                        : "(EGL_EXT_image_dma_buf_import_modifiers absent)")
                                    << "— falling back to CPU";
        }
        GstHwPathTelemetry::recordFallbackReason(HwVideoBufferPath::DmaBuf,
                                                 GstHwPathTelemetry::HwFallbackReason::ModifierRejected);
        return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
    }

    // Producer barrier (Intel iHD legacy LINEAR is not implicitly fenced by the EGL import).
    // QGC_GST_DMABUF_NO_MMAP_FENCE=1 skips it where PRIME 2 fence FDs make it redundant.
    if (!HwBuffers::hwBufferEnvConfig().dmaBufNoMmapFence) {
        waitForProducer(eglDpy, buffer);
    }

    const EglImageBinder binder = eglImageBinder(*glContext);
    if (!binder.targetTexture2D) {
        qCWarning(GstDmaBufLog) << "glEGLImageTargetTexture2DOES unavailable";
        return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
    }

    // gst_video_frame_map(GST_MAP_READ) on DMABuf mmaps and defeats zero-copy; read offsets from GstVideoMeta.
    // Qt 6.12's upstream GStreamer path also prefers GstVideoMeta/video-info offsets; preserve this no-mmap behavior when
    // reconciling against newer Qt sources.
    const int planeCount = std::clamp(int(GST_VIDEO_INFO_N_PLANES(&_videoInfo)), 1, kMaxPlanes);
    const int memCount = int(gst_buffer_n_memory(buffer));
    // memCount==1: either multi-plane shared fd (NV12) or packed single-plane (YUY2/UYVY/…).
    const int singleFdFourcc = (memCount == 1) ? GstHw::drmFourccForSingleFd(_videoInfo) : -1;
    bool singleFdPacking = (singleFdFourcc != -1);
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

    // Fresh texture names per frame: each bundle owns and deletes its own, and reusing them across bundles isn't worth
    // the ~us/frame.
    std::array<GLuint, kMaxPlanes> names{};
    QOpenGLFunctions functions(glContext);
    functions.glGenTextures(planeCount, names.data());

    const EglImport import{.display = eglDpy,
                           .buffer = buffer,
                           .info = &_videoInfo,
                           .vmeta = gst_buffer_get_video_meta(buffer),
                           .hasModifiersExt = hasModifiersExt,
                           .validateBind = !s_bindValidated.load(std::memory_order_relaxed),
                           .gl = &functions,
                           .binder = binder,
                           .names = std::span<const GLuint>(names.data(), std::size_t(planeCount))};
    ImportResult result = ImportResult::TryPerPlane;
    // Skipped once a single-fd import hit EGL_BAD_MATCH.
    if (singleFdPacking && !s_singleFdBroken.load(std::memory_order_relaxed)) {
        result = importSingleFd(import, singleFdFourcc);
    }
    if (result == ImportResult::TryPerPlane) {
        result = importPerPlane(import, singleFdPacking) ? ImportResult::Ok : ImportResult::Fail;
    }
    if (result != ImportResult::Ok) {
        functions.glDeleteTextures(planeCount, names.data());
        return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
    }
    if (import.validateBind) {
        s_bindValidated.store(true, std::memory_order_relaxed);
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
