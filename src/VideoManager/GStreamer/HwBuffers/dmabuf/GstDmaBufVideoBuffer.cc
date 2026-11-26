#include "GstDmaBufVideoBuffer.h"

#include "GstDmaBufVulkanImport.h"
#include "GstEglHelpers.h"
#include "GstHwFrameTexturesBase.h"
#include "GstHwPathRegistry.h"
#include "GstHwPathTelemetry.h"
#include "GstHwVideoBuffer.h"
#include "GstQtDmaBufImport.h"
#include "QGCRhiCapture.h"

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)

#include <gst/allocators/gstdmabuf.h>
#include <gst/video/video.h>
#include <rhi/qrhi.h>

#include <QtCore/QLoggingCategory>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtGui/QGuiApplication>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>

#include "QGCLoggingCategory.h"
#if GST_CHECK_VERSION(1, 24, 0)
#include <gst/video/video-info-dma.h>
#endif

#include <algorithm>
#include <array>
#include <atomic>
#include <cerrno>
#include <drm_fourcc.h>
#include <limits>
#include <memory>
#include <poll.h>
#include <span>
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

QGC_LOGGING_CATEGORY(GstDmaBufLog, "Video.GStreamer.HwBuffers.GstDmaBuf")

namespace {

using GstHw::kMaxPlanes;

// Set when an import fails in a way no later frame avoids: EGL_BAD_MATCH from a broken EGL/VA-API combination
// (QTBUG-112312), or a driver rejecting the stream's layout frame after frame. Qt's importer warns on every failed
// frame, so the latch also bounds that log. Short-circuits mapTextures() and validatePlaneHandles().
std::atomic<bool> s_importDisabled{false};

/// What decides whether a frame imports; the frames of one stream share it.
struct ImportLayout
{
    guint64 modifier = 0;
    QVideoFrameFormat::PixelFormat pixelFormat = QVideoFrameFormat::Format_Invalid;
    QSize size;

    friend bool operator==(const ImportLayout&, const ImportLayout&) = default;
};

/// Runs of failed imports, one per layout. The streams shown at once (video and thermal) import in turn on the render
/// thread: with a single run, the imports of one stream would end the run of another whose every import fails.
class ImportFailureRuns
{
public:
    /// Counts a failed import of @p layout; the length of its run.
    int noteFailure(const ImportLayout& layout)
    {
        const QMutexLocker lock(&_mutex);
        Run* run = find(layout);
        if (!run) {
            // A free slot, else the shortest of the other layouts' runs.
            run = std::min_element(_runs.begin(), _runs.end(),
                                   [](const Run& a, const Run& b) { return a.failures < b.failures; });
            *run = {.layout = layout};
        }
        return ++run->failures;
    }

    void noteSuccess(const ImportLayout& layout)
    {
        const QMutexLocker lock(&_mutex);
        if (Run* run = find(layout)) {
            run->failures = 0;
        }
    }

    void clear()
    {
        const QMutexLocker lock(&_mutex);
        _runs = {};
    }

private:
    struct Run
    {
        ImportLayout layout;
        int failures = 0;
    };

    Run* find(const ImportLayout& layout)
    {
        const auto run = std::find_if(_runs.begin(), _runs.end(), [&layout](const Run& candidate) {
            return (candidate.failures > 0) && (candidate.layout == layout);
        });
        return (run != _runs.end()) ? run : nullptr;
    }

    QMutex _mutex;
    std::array<Run, 4> _runs{};
};

ImportFailureRuns s_importFailures;

constexpr const char* kModifiersExt = "EGL_EXT_image_dma_buf_import_modifiers";
constexpr const char* kNativeFenceExt = "EGL_ANDROID_native_fence_sync";

void disableImport(GstHwPathTelemetry::HwFallbackReason reason, const char* cause)
{
    if (!s_importDisabled.exchange(true, std::memory_order_relaxed)) {
        GstHwPathTelemetry::recordFallbackReason(HwVideoBufferPath::DmaBuf, reason);
        qCWarning(GstDmaBufLog) << cause << "— DMABuf zero-copy disabled until the scene graph is recreated";
    }
}

/// Feeds @p outcome of importing a frame of @p layout into the disable latch: a refusal by the driver counts in the
/// layout's run of failures, as does a @p tiled frame that could not be imported, whatever kept it from being.
void noteLayoutImportOutcome(const ImportLayout& layout, bool tiled, GstDmaBufVideoBuffer::ImportOutcome outcome)
{
    using ImportOutcome = GstDmaBufVideoBuffer::ImportOutcome;
    switch (outcome) {
        case ImportOutcome::Imported:
            s_importFailures.noteSuccess(layout);
            return;
        case ImportOutcome::Unrecoverable:
            disableImport(GstHwPathTelemetry::HwFallbackReason::EglBadMatch,
                          "Qt's DMABuf importer hit EGL_BAD_MATCH (broken EGL/driver combination)");
            return;
        case ImportOutcome::Rejected:
            GstHwPathTelemetry::recordFallbackReason(HwVideoBufferPath::DmaBuf,
                                                     GstHwPathTelemetry::HwFallbackReason::MapFailed);
            break;
        case ImportOutcome::FdDupFailed:
            // The process is out of file descriptors, which no driver refused: counted under a reason of its own. As
            // below, only a tiled frame has no other way to the screen.
            GstHwPathTelemetry::recordFallbackReason(HwVideoBufferPath::DmaBuf,
                                                     GstHwPathTelemetry::HwFallbackReason::FdDupFailed);
            if (!tiled) {
                return;
            }
            break;
        case ImportOutcome::NotImportable:
            // Qt shows a LINEAR frame through map() instead, and another QRhi may import it. A tiled one is not shown
            // at all, so only renegotiating without DMABuf brings its stream back.
            if (!tiled) {
                return;
            }
            break;
    }
    if (s_importFailures.noteFailure(layout) < GstDmaBufVideoBuffer::kMaxConsecutiveImportFailures) {
        return;
    }
    if (outcome == ImportOutcome::FdDupFailed) {
        disableImport(GstHwPathTelemetry::HwFallbackReason::FdDupFailed,
                      "The dma-buf fds of a stream's recent frames could not be duplicated");
    } else {
        disableImport(GstHwPathTelemetry::HwFallbackReason::MapFailed, "Every recent DMABuf import of a stream failed");
    }
}

// RPi eglfs quirk: V3D EGL implicitly converts YUYV/UYVY DMABuf to RGBA, so declare RGBA8888 to pick Qt's RGB shader
// and avoid a double YUV->RGB (mirrors Qt's eglfs branch). The import then uses the packed-YUV fourcc for the whole
// image.
quint32 eglfsPackedYuvFourcc(QVideoFrameFormat::PixelFormat format)
{
    static const bool isEglfsQPA = QGuiApplication::platformName() == QLatin1String("eglfs");
    if (!isEglfsQPA) {
        return 0;
    }
    if (format == QVideoFrameFormat::Format_YUYV) {
        return DRM_FORMAT_YUYV;
    }
    if (format == QVideoFrameFormat::Format_UYVY) {
        return DRM_FORMAT_UYVY;
    }
    return 0;
}

// Best-effort GPU-side wait on the producer's fences: export a sync_file from each dma-buf, import it as an
// EGL_SYNC_NATIVE_FENCE_ANDROID sync and eglWaitSyncKHR in Qt's (current) context. True only when every plane's
// dma-buf got a confirmed server-side wait, which makes the CPU barrier redundant.
bool tryExplicitFenceWait(EGLDisplay eglDpy, GstBuffer* buffer)
{
#if defined(QGC_HAS_DMABUF_EXPORT_SYNC)
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

    int lastFd = -1;
    for (guint i = 0; i < gst_buffer_n_memory(buffer); ++i) {
        const int dmaFd = gst_dmabuf_memory_get_fd(gst_buffer_peek_memory(buffer, i));
        if (dmaFd < 0 || dmaFd == lastFd) {
            continue;
        }
        lastFd = dmaFd;
        dma_buf_export_sync_file req = {};
        req.flags = DMA_BUF_SYNC_READ;  // wait only on writers (the decoder), not on prior readers
        req.fd = -1;
        if (ioctl(dmaFd, DMA_BUF_IOCTL_EXPORT_SYNC_FILE, &req) != 0 || req.fd < 0) {
            return false;
        }
        // EGL owns req.fd once the sync is created (closed by eglDestroySync); close it ourselves only on failure.
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
    }
    if (QGC_HW_ONCE()) {
        qCDebug(GstDmaBufLog) << "waiting on the producer's dma-buf fences GPU-side (CPU barrier skipped)";
    }
    GstHwPathTelemetry::record(HwVideoBufferPath::DmaBuf, GstHwPathTelemetry::Counter::GpuWaits);
    return true;
#else
    Q_UNUSED(eglDpy);
    Q_UNUSED(buffer);
    return false;
#endif
}

// Bounded so a GPU hang cannot wedge Qt's render thread.
constexpr int kProducerWaitTimeoutMs = 100;

// CPU-side barrier on the producer's implicit write fences: poll(POLLIN) on a dma-buf fd returns once its write fences
// signal (the wait linux/dma-buf.h documents). Unlike mmap/DMA_BUF_IOCTL_SYNC it triggers no exporter cache maintenance
// or buffer migration. False when a wait timed out or failed, or the fd reported anything but POLLIN: poll() also
// returns for POLLERR, POLLHUP and POLLNVAL, which say nothing about the writes.
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
        const bool writesDone = (ret > 0) && (pfd.revents & POLLIN) && !(pfd.revents & (POLLERR | POLLHUP | POLLNVAL));
        if (!writesDone) {
            const int pollError = (ret < 0) ? errno : 0;
            GstHwPathTelemetry::record(HwVideoBufferPath::DmaBuf, GstHwPathTelemetry::Counter::ProducerFenceTimeouts);
            QGC_HW_WARN_ONCE(GstDmaBufLog, "dma-buf producer fence wait"
                                               << (ret == 0 ? "timed out" : "failed") << "errno=" << pollError
                                               << "revents=" << Qt::hex << pfd.revents << Qt::dec
                                               << "— no producer barrier for this frame");
            return false;
        }
    }
    GstHwPathTelemetry::record(HwVideoBufferPath::DmaBuf, GstHwPathTelemetry::Counter::CpuWaits);
    return true;
}

// Qt's importer has no producer barrier, and Intel iHD's LINEAR exports aren't implicitly fenced by the EGL import:
// prefer a GPU-side wait on the exported fences, else block on the implicit ones.
void waitForProducer(EGLDisplay eglDpy, GstBuffer* buffer)
{
    if (!tryExplicitFenceWait(eglDpy, buffer)) {
        waitForProducerWrites(buffer);
    }
}

// Bounded so a GPU that hangs can't keep a decoder's buffer for good.
constexpr int kGpuReadsTimeoutMs = 1000;

#ifdef QGC_UNITTEST_BUILD
std::atomic<int> s_gpuFenceForTest{-1};
#endif

/// A sync file that signals once the GPU has run everything the context current on this thread has issued so far; -1
/// when none is current or EGL can't export its fences (EGL_ANDROID_native_fence_sync).
int exportGpuFence(EGLDisplay eglDpy)
{
#ifdef QGC_UNITTEST_BUILD
    if (const int fd = s_gpuFenceForTest.load(std::memory_order_relaxed); fd >= 0) {
        return ::dup(fd);
    }
#endif
    QOpenGLContext* const context = QOpenGLContext::currentContext();
    if (!context || !GstEglHelpers::displaySupportsExtension(eglDpy, kNativeFenceExt)) {
        return -1;
    }
    using DupNativeFenceFd = EGLint(EGLAPIENTRY*)(EGLDisplay, EGLSyncKHR);
    static const auto createSyncKHR = reinterpret_cast<PFNEGLCREATESYNCKHRPROC>(eglGetProcAddress("eglCreateSyncKHR"));
    static const auto destroySyncKHR =
        reinterpret_cast<PFNEGLDESTROYSYNCKHRPROC>(eglGetProcAddress("eglDestroySyncKHR"));
    static const auto dupNativeFenceFd =
        reinterpret_cast<DupNativeFenceFd>(eglGetProcAddress("eglDupNativeFenceFDANDROID"));
    if (!createSyncKHR || !destroySyncKHR || !dupNativeFenceFd) {
        return -1;
    }
    const EGLint attribs[] = {EGL_SYNC_NATIVE_FENCE_FD_ANDROID, EGL_NO_NATIVE_FENCE_FD_ANDROID, EGL_NONE};
    const EGLSyncKHR sync = createSyncKHR(eglDpy, EGL_SYNC_NATIVE_FENCE_ANDROID, attribs);
    if (sync == EGL_NO_SYNC_KHR) {
        return -1;
    }
    // The sync gets its fence when the commands before it are flushed.
    context->functions()->glFlush();
    const int fence = dupNativeFenceFd(eglDpy, sync);
    destroySyncKHR(eglDpy, sync);
    return fence;
}

/// A decoder's buffer and the fence of the GPU work that sampled it.
struct PendingGpuReads
{
    int fence = -1;
    GstBuffer* buffer = nullptr;
};

/// GFunc of the pool below: waits for the fence of the PendingGpuReads @p data, then lets go of its buffer.
void waitForGpuReads(gpointer data, gpointer /*userData*/)
{
    const std::unique_ptr<PendingGpuReads> pending(static_cast<PendingGpuReads*>(data));
    pollfd pfd = {pending->fence, POLLIN, 0};
    int ret = 0;
    do {
        ret = ::poll(&pfd, 1, kGpuReadsTimeoutMs);
    } while (ret < 0 && (errno == EINTR || errno == EAGAIN));
    ::close(pending->fence);
    gst_buffer_unref(pending->buffer);
}

/// Lets go of @p buffer, whose textures Qt has just deleted, once the GPU has run the draws that sampled them: a
/// decoder that doesn't wait for a dma-buf's readers, as V4L2's don't, would otherwise write its next picture under
/// them. Qt deletes the textures on the render thread with their context current. Another thread waits, so neither
/// the render thread nor the decoder depends on a later frame for it; without a fence the buffer goes back at once.
void unrefAfterGpuReads(EGLDisplay eglDpy, GstBuffer* buffer)
{
    // GLib's threads, as many as there are buffers waiting: the decoder needs these buffers back, and QGC's long jobs
    // can keep every thread of Qt's global pool busy.
    static GThreadPool* const waiters = g_thread_pool_new(&waitForGpuReads, nullptr, -1, FALSE, nullptr);
    const int fence = waiters ? exportGpuFence(eglDpy) : -1;
    if (fence < 0) {
        gst_buffer_unref(buffer);
        return;
    }
    // A false return only says no thread could be started now: the wait stays queued.
    (void) g_thread_pool_push(waiters, new PendingGpuReads{.fence = fence, .buffer = buffer}, nullptr);
}

/// Each plane's fd and byte offset inside that dma-buf, located as GStreamer's own frame map and gst-gl's DMABuf upload
/// do: the meta offset counts across the buffer's memories. False when a plane isn't inside one dma-buf memory or its
/// layout doesn't fit EGL's attributes.
bool locatePlanes(GstBuffer* buffer, const GstVideoInfo& info, const GstQtDmaBufImport::PlaneFourccs& fourccs,
                  quint64 modifier, std::span<GstQtDmaBufImport::Plane> planes)
{
    const GstVideoMeta* vmeta = gst_buffer_get_video_meta(buffer);
    for (int i = 0; i < fourccs.count; ++i) {
        // The meta's layout when present: decoders pad rows and planes beyond the caps' layout.
        const gsize offset = vmeta ? vmeta->offset[i] : GST_VIDEO_INFO_PLANE_OFFSET(&info, i);
        const gint stride = vmeta ? vmeta->stride[i] : GST_VIDEO_INFO_PLANE_STRIDE(&info, i);
        guint memIdx = 0;
        guint length = 0;
        gsize skip = 0;
        if (!gst_buffer_find_memory(buffer, offset, 1, &memIdx, &length, &skip) || (length != 1)) {
            return false;
        }
        GstMemory* mem = gst_buffer_peek_memory(buffer, memIdx);
        const quint64 fdOffset = static_cast<quint64>(mem->offset) + skip;
        constexpr quint64 kMaxAttrib = (std::numeric_limits<EGLint>::max)();
        if (!gst_is_dmabuf_memory(mem) || (stride <= 0) || (fdOffset > kMaxAttrib)) {
            return false;
        }
        planes[std::size_t(i)] = {.fd = gst_dmabuf_memory_get_fd(mem),
                                  .offset = static_cast<quint32>(fdOffset),
                                  .pitch = static_cast<quint32>(stride),
                                  .drmFourcc = fourccs.fourcc[std::size_t(i)],
                                  .modifier = modifier};
        if (planes[std::size_t(i)].fd < 0) {
            return false;
        }
    }
    return true;
}

/// locatePlanes() for a frame sampled as @p fourccs; a layout that can't be imported is logged once and counted.
bool locateImportPlanes(GstBuffer* buffer, const GstVideoInfo& info, const GstQtDmaBufImport::PlaneFourccs& fourccs,
                        quint64 modifier, std::span<GstQtDmaBufImport::Plane> planes)
{
    if ((fourccs.count != 0) && (fourccs.count == int(GST_VIDEO_INFO_N_PLANES(&info))) &&
        locatePlanes(buffer, info, fourccs, modifier, planes)) {
        return true;
    }
    QGC_HW_WARN_ONCE(GstDmaBufLog, "DMABuf layout not importable (format"
                                       << gst_video_format_to_string(GST_VIDEO_INFO_FORMAT(&info)) << "memories"
                                       << gst_buffer_n_memory(buffer) << ") — CPU fallback");
    GstHwPathTelemetry::recordFallbackReason(HwVideoBufferPath::DmaBuf,
                                             GstHwPathTelemetry::HwFallbackReason::ImportUnsupported);
    return false;
}

}  // namespace

GstDmaBufVideoBuffer::GstDmaBufVideoBuffer(GstSample* sample, const GstVideoInfo& videoInfo,
                                           const QVideoFrameFormat& format, EGLDisplay eglDisplay)
    : GstHwVideoBuffer(sample, videoInfo, format)
    , _eglDisplay(eglDisplay)
    , _drmModifier(DRM_FORMAT_MOD_INVALID)
    , _packedYuvFourcc(eglfsPackedYuvFourcc(format.pixelFormat()))
{
    // Advertised for the GPU import only; the base class keeps the memory's format, so map() refuses the CPU upload Qt
    // would otherwise read as RGBA.
    if (_packedYuvFourcc != 0) {
        _format = GstHw::withPixelFormat(_format, QVideoFrameFormat::Format_RGBA8888);
    }
#if GST_CHECK_VERSION(1, 24, 0)
    if (GstCaps* caps = gst_sample_get_caps(sample); caps && gst_video_is_dma_drm_caps(caps)) {
        GstVideoInfoDmaDrm drmInfo = {};
        gst_video_info_dma_drm_init(&drmInfo);
        if (gst_video_info_dma_drm_from_caps(&drmInfo, caps)) {
            _drmModifier = drmInfo.drm_modifier;
        }
    }
#endif
}

EGLDisplay GstDmaBufVideoBuffer::eglDisplayFor(const HwVideoBufferContext& context)
{
    return (context.dmaBufImporter == QGCRhiCapture::qtEglDisplay()) ? context.dmaBufImporter : EGL_NO_DISPLAY;
}

bool GstDmaBufVideoBuffer::isTiled() const noexcept
{
    return (_drmModifier != DRM_FORMAT_MOD_INVALID) && (_drmModifier != DRM_FORMAT_MOD_LINEAR);
}

bool GstDmaBufVideoBuffer::importDisabled() noexcept
{
    return s_importDisabled.load(std::memory_order_relaxed);
}

void GstDmaBufVideoBuffer::noteImportOutcome(ImportOutcome outcome) const
{
    noteLayoutImportOutcome(
        {.modifier = _drmModifier, .pixelFormat = _format.pixelFormat(), .size = _format.frameSize()}, isTiled(),
        outcome);
}

void GstDmaBufVideoBuffer::resetCachedState() noexcept
{
    s_importDisabled.store(false, std::memory_order_relaxed);
    s_importFailures.clear();
    GstHw::rearmOnceLatches();
#if defined(QGC_HAS_GST_DMABUF_VULKAN_IMPORT)
    GstDmaBufVulkan::resetCachedState();
#endif
}

namespace {

// Only the EGLDisplay Qt itself renders with imports for it, and Qt has none under GLX; under Vulkan its device
// imports. Without either the path admits no frame, so DMABuf frames take the CPU path up front instead of failing
// every import. The importer comes and goes within a caps epoch (Qt's EGLDisplay or Vulkan device becomes known, an
// import failure disables the path until the scene graph is recreated), so it is asked for every frame rather than
// deciding whether the path matches.
bool admitDmaBuf(GstMemory*, const HwVideoBufferContext& context)
{
    if (context.dmaBufImporter) {
        return true;
    }
    GstHwPathTelemetry::recordFallbackReason(HwVideoBufferPath::DmaBuf, GstHwPathTelemetry::HwFallbackReason::NoExt);
    return false;
}

}  // namespace

constinit const HwPathDescriptor GstHwPaths::kDmaBuf = {
    .path = HwVideoBufferPath::DmaBuf,
    .matches = [](GstMemory* mem) -> bool { return gst_is_dmabuf_memory(mem); },
    .admits = &admitDmaBuf,
    .construct = [](GstSample* sample, const GstVideoInfo& info, QVideoFrameFormat format,
                    const HwVideoBufferContext& context) -> std::unique_ptr<GstHwVideoBuffer> {
        return std::make_unique<GstDmaBufVideoBuffer>(sample, info, format,
                                                      GstDmaBufVideoBuffer::eglDisplayFor(context));
    },
#if defined(QGC_HAS_GST_DMABUF_VULKAN_IMPORT)
    .probeDevice = &GstDmaBufVulkan::probeDevice,
#endif
    .resetCaches =
        [] {
            GstDmaBufVideoBuffer::resetCachedState();
            GstEglHelpers::resetExtensionCache();
        },
};

#ifdef QGC_UNITTEST_BUILD
void GstDmaBufVideoBuffer::recordImportFailureForTest(bool unrecoverable)
{
    noteLayoutImportOutcome({}, /*tiled=*/false,
                            unrecoverable ? ImportOutcome::Unrecoverable : ImportOutcome::Rejected);
}

void GstDmaBufVideoBuffer::noteImportOutcomeForTest(ImportOutcome outcome) const
{
    noteImportOutcome(outcome);
}

bool GstDmaBufVideoBuffer::waitForProducerWritesForTest(GstBuffer* buffer)
{
    return waitForProducerWrites(buffer);
}

void GstDmaBufVideoBuffer::setGpuFenceForTest(int fd)
{
    s_gpuFenceForTest.store(fd, std::memory_order_relaxed);
}
#endif

bool GstDmaBufVideoBuffer::validatePlaneHandles() const
{
    if (importDisabled()) {
        return false;
    }
    const bool planesOk = validatePlanes(
        [](GstMemory* mem) { return mem && gst_is_dmabuf_memory(mem) && (gst_dmabuf_memory_get_fd(mem) >= 0); });
    if (!planesOk) {
        return false;
    }
#if defined(QGC_HAS_GST_DMABUF_VULKAN_IMPORT)
    // Qt's Vulkan device takes the layouts it reported when it was probed, and no others.
    if (QGCRhiCapture::deviceSnapshot().backend.load(std::memory_order_acquire) == static_cast<int>(QRhi::Vulkan)) {
        return GstDmaBufVulkan::canImport(GstQtDmaBufImport::planeFourccs(_format.pixelFormat()), _drmModifier);
    }
#endif
    // A tiled layout imports only through the modifier attributes.
    return !isTiled() ||
           ((_eglDisplay != EGL_NO_DISPLAY) && GstEglHelpers::displaySupportsExtension(_eglDisplay, kModifiersExt));
}

QAbstractVideoBuffer::MapData GstDmaBufVideoBuffer::map(QVideoFrame::MapMode mode)
{
    if (isTiled()) {
        QGC_HW_WARN_ONCE(GstDmaBufLog, "tiled DMABuf frame has no CPU layout (modifier" << Qt::hex << _drmModifier
                                                                                        << ") — frame not shown");
        return {};
    }
    return GstHwVideoBuffer::map(mode);
}

QVideoFrameTexturesUPtr GstDmaBufVideoBuffer::importTextures(QRhi& rhi, QVideoFrameTexturesUPtr& old)
{
    const GstHwPathTelemetry::ScopedMapTimer mapTimer(HwVideoBufferPath::DmaBuf);
    if (importDisabled()) {
        return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
    }
    ImportOutcome failure = ImportOutcome::NotImportable;
    QVideoFrameTexturesUPtr textures = importFrame(rhi, old, failure);
    noteImportOutcome(textures ? ImportOutcome::Imported : failure);
    return textures;
}

QVideoFrameTexturesUPtr GstDmaBufVideoBuffer::importFrame(QRhi& rhi, QVideoFrameTexturesUPtr& old,
                                                          ImportOutcome& failure)
{
#if defined(QGC_HAS_GST_DMABUF_VULKAN_IMPORT)
    const bool vulkan = (rhi.backend() == QRhi::Vulkan);
#else
    constexpr bool vulkan = false;
    Q_UNUSED(old);
#endif
    GstBuffer* buffer = nullptr;
    if (!checkMapPreconditions(rhi, static_cast<int>(vulkan ? QRhi::Vulkan : QRhi::OpenGLES2), GstDmaBufLog(),
                               buffer)) {
        return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
    }

#if defined(QGC_HAS_GST_DMABUF_VULKAN_IMPORT)
    if (vulkan) {
        return importVulkan(rhi, buffer, old, failure);
    }
#endif
    return importGl(rhi, buffer, failure);
}

#if defined(QGC_HAS_GST_DMABUF_VULKAN_IMPORT)
QVideoFrameTexturesUPtr GstDmaBufVideoBuffer::importVulkan(QRhi& rhi, GstBuffer* buffer, QVideoFrameTexturesUPtr& old,
                                                           ImportOutcome& failure)
{
    const QVideoFrameFormat::PixelFormat pixelFormat = _format.pixelFormat();
    const GstQtDmaBufImport::PlaneFourccs fourccs = GstQtDmaBufImport::planeFourccs(pixelFormat);
    std::array<GstQtDmaBufImport::Plane, kMaxPlanes> planes{};
    if (!locateImportPlanes(buffer, _videoInfo, fourccs, _drmModifier, planes)) {
        return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
    }
    if (!GstHwImportPreflight::preflightOrRecord(rhi, HwVideoBufferPath::DmaBuf, pixelFormat, _format.frameSize())) {
        return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
    }

    // Vulkan does not wait on the implicit fences of imported dma-buf memory, and QRhi's submission can't be given the
    // producer's fence: import only once the poll() barrier confirms the producer's writes completed.
    if (!waitForProducerWrites(buffer)) {
        GstHwPathTelemetry::recordFallbackReason(HwVideoBufferPath::DmaBuf,
                                                 GstHwPathTelemetry::HwFallbackReason::VulkanNoSync);
        QGC_HW_WARN_ONCE(GstDmaBufLog, "Vulkan import refused: producer writes not known complete — CPU fallback");
        return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
    }

    GstDmaBufVulkan::ImportFailure vulkanFailure = GstDmaBufVulkan::ImportFailure::NotImportable;
    QVideoFrameTexturesUPtr textures =
        GstDmaBufVulkan::importTextures(*this, rhi, old, std::span(planes.data(), std::size_t(fourccs.count)),
                                        pixelFormat, _format.frameSize(), buffer, vulkanFailure);
    switch (vulkanFailure) {
        case GstDmaBufVulkan::ImportFailure::NotImportable:
            break;
        case GstDmaBufVulkan::ImportFailure::Rejected:
            failure = ImportOutcome::Rejected;
            break;
        case GstDmaBufVulkan::ImportFailure::FdDupFailed:
            failure = ImportOutcome::FdDupFailed;
            break;
    }
    return textures;
}
#endif

QVideoFrameTexturesUPtr GstDmaBufVideoBuffer::importGl(QRhi& rhi, GstBuffer* buffer, ImportOutcome& failure)
{
    const EGLDisplay eglDpy = GstQtDmaBufImport::importDisplay(rhi);
    if (eglDpy == EGL_NO_DISPLAY) {
        QGC_HW_WARN_ONCE(GstDmaBufLog,
                         "Qt's GL context is not EGL or can't bind EGLImages — DMABuf frames take the CPU path");
        GstHwPathTelemetry::recordFallbackReason(HwVideoBufferPath::DmaBuf,
                                                 GstHwPathTelemetry::HwFallbackReason::NoExt);
        return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
    }

    const QVideoFrameFormat::PixelFormat pixelFormat = _format.pixelFormat();
    GstQtDmaBufImport::PlaneFourccs fourccs = GstQtDmaBufImport::planeFourccs(pixelFormat);
    if (_packedYuvFourcc != 0) {
        fourccs = {.fourcc = {_packedYuvFourcc}, .count = 1};
    }
    const quint64 modifier =
        ((_drmModifier != DRM_FORMAT_MOD_INVALID) && GstEglHelpers::displaySupportsExtension(eglDpy, kModifiersExt))
            ? _drmModifier
            : GstQtDmaBufImport::kImplicitModifier;
    std::array<GstQtDmaBufImport::Plane, kMaxPlanes> planes{};
    if (!locateImportPlanes(buffer, _videoInfo, fourccs, modifier, planes)) {
        return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
    }

    if (!GstHwImportPreflight::preflightOrRecord(rhi, HwVideoBufferPath::DmaBuf, pixelFormat, _format.frameSize())) {
        return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
    }

    waitForProducer(eglDpy, buffer);

    // The textures sample the dma-bufs directly: keep the decoder from recycling them until Qt has dropped the
    // textures and the GPU has drawn with them.
    std::shared_ptr<void> keepAlive(gst_buffer_ref(buffer),
                                    [eglDpy](void* ref) { unrefAfterGpuReads(eglDpy, static_cast<GstBuffer*>(ref)); });
    GstQtDmaBufImport::ImportResult imported =
        GstQtDmaBufImport::importTextures(rhi, std::span(planes.data(), std::size_t(fourccs.count)), pixelFormat,
                                          _format.frameSize(), std::move(keepAlive));
    if (!imported.textures) {
        failure = imported.unrecoverable ? ImportOutcome::Unrecoverable : ImportOutcome::Rejected;
        return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
    }
    if (QGC_HW_ONCE()) {
        qCDebug(GstDmaBufLog) << "First DMABuf import: size=" << _format.frameSize() << "format=" << pixelFormat
                              << "modifier=" << Qt::hex << modifier;
    }
    return std::move(imported.textures);
}

#endif  // QGC_HAS_GST_DMABUF_GPU_PATH
