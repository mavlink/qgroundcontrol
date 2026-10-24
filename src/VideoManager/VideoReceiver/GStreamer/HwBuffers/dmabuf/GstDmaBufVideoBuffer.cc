#include "GstDmaBufVideoBuffer.h"

#include "GstDmaBufVulkanImport.h"
#include "GstEglHelpers.h"
#include "GstHwImportPreflight.h"
#include "GstHwPathRegistry.h"
#include "GstHwPathTelemetry.h"
#include "GstHwVideoBuffer.h"
#include "GstHwVideoInfo.h"
#include "GstQtDmaBufImport.h"
#include "QGCRhiCapture.h"

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)

#include <gst/allocators/gstdmabuf.h>
#include <gst/video/video.h>
#include <rhi/qrhi.h>

#include <QtCore/QLoggingCategory>
#include <QtGui/QGuiApplication>

#include "QGCLoggingCategory.h"
#if GST_CHECK_VERSION(1, 24, 0)
#include <gst/video/video-info-dma.h>
#endif

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

#ifndef EGL_ANDROID_native_fence_sync
#define EGL_SYNC_NATIVE_FENCE_ANDROID 0x3144
#define EGL_SYNC_NATIVE_FENCE_FD_ANDROID 0x3145
#define EGL_NO_NATIVE_FENCE_FD_ANDROID -1
#endif

QGC_LOGGING_CATEGORY(GstDmaBufLog, "Video.GStreamer.HwBuffers.GstDmaBuf")

namespace {

using GstHw::kMaxPlanes;

GstHw::MapDiagnostics s_diag;

// Set when an import fails in a way no later frame avoids: EGL_BAD_MATCH from a broken EGL/VA-API combination
// (QTBUG-112312), or a driver rejecting the stream's layout frame after frame. Qt's importer warns on every failed
// frame, so the latch also bounds that log. Short-circuits mapTextures() and validatePlaneHandles().
std::atomic<bool> s_importDisabled{false};
std::atomic<int> s_consecutiveImportFailures{0};

std::atomic<bool> s_loggedNoEglContext{false};
std::atomic<bool> s_loggedLayoutRejected{false};
std::atomic<bool> s_loggedTiledCpuMap{false};
std::atomic<bool> s_loggedProducerWaitTimeout{false};
std::atomic<bool> s_loggedExplicitFence{false};
std::atomic<bool> s_loggedVulkanSyncRefused{false};

constexpr const char* kModifiersExt = "EGL_EXT_image_dma_buf_import_modifiers";
constexpr const char* kNativeFenceExt = "EGL_ANDROID_native_fence_sync";

void disableImport(GstHwPathTelemetry::HwFallbackReason reason, const char* cause)
{
    if (!s_importDisabled.exchange(true, std::memory_order_relaxed)) {
        GstHwPathTelemetry::recordFallbackReason(HwVideoBufferPath::DmaBuf, reason);
        qCWarning(GstDmaBufLog) << cause << "— DMABuf zero-copy disabled until the scene graph is recreated";
    }
}

void recordImportFailure(bool unrecoverable)
{
    if (unrecoverable) {
        disableImport(GstHwPathTelemetry::HwFallbackReason::EglBadMatch,
                      "Qt's DMABuf importer hit EGL_BAD_MATCH (broken EGL/driver combination)");
        return;
    }
    GstHwPathTelemetry::recordFallbackReason(HwVideoBufferPath::DmaBuf,
                                             GstHwPathTelemetry::HwFallbackReason::MapFailed);
    if (s_consecutiveImportFailures.fetch_add(1, std::memory_order_relaxed) + 1 >=
        GstDmaBufVideoBuffer::kMaxConsecutiveImportFailures) {
        disableImport(GstHwPathTelemetry::HwFallbackReason::MapFailed,
                      "The driver rejected every recent DMABuf import");
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

QVideoFrameFormat withPixelFormat(const QVideoFrameFormat& format, QVideoFrameFormat::PixelFormat pixelFormat)
{
    QVideoFrameFormat out(format.frameSize(), pixelFormat);
    out.setStreamFrameRate(format.streamFrameRate());
    out.setColorRange(format.colorRange());
    out.setColorSpace(format.colorSpace());
    out.setColorTransfer(format.colorTransfer());
    out.setViewport(format.viewport());
    return out;
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
    if (!s_loggedExplicitFence.exchange(true, std::memory_order_relaxed)) {
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

// Qt's importer has no producer barrier, and Intel iHD's LINEAR exports aren't implicitly fenced by the EGL import:
// prefer a GPU-side wait on the exported fences, else block on the implicit ones.
void waitForProducer(EGLDisplay eglDpy, GstBuffer* buffer)
{
    if (!tryExplicitFenceWait(eglDpy, buffer)) {
        waitForProducerWrites(buffer);
    }
}

/// Each plane's fd and byte offset inside that dma-buf, located as GStreamer's own frame map and gst-gl's DMABuf upload
/// do: the meta offset counts across the buffer's memories. False when a plane isn't inside one dma-buf memory or its
/// layout doesn't fit EGL's attributes.
bool locatePlanes(GstBuffer* buffer, const GstVideoInfo& info, const GstQtDmaBufImport::PlaneFourccs& fourccs,
                  quint64 modifier, std::span<GstQtDmaBufImport::Plane> planes)
{
    const GstVideoMeta* vmeta = gst_buffer_get_video_meta(buffer);
    for (int i = 0; i < fourccs.count; ++i) {
        const auto [offset, stride] = GstHw::planeLayout(info, vmeta, i);
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

}  // namespace

GstDmaBufVideoBuffer::GstDmaBufVideoBuffer(GstSample* sample, const GstVideoInfo& videoInfo,
                                           const QVideoFrameFormat& format, EGLDisplay eglDisplay)
    : GstHwVideoBuffer(QVideoFrame::RhiTextureHandle, sample, videoInfo, format)
    , _eglDisplay(eglDisplay)
    , _drmModifier(DRM_FORMAT_MOD_INVALID)
    , _packedYuvFourcc(eglfsPackedYuvFourcc(format.pixelFormat()))
{
    // Advertised for the GPU import only; the base class keeps the memory's format, so map() refuses the CPU upload Qt
    // would otherwise read as RGBA.
    if (_packedYuvFourcc != 0) {
        _format = withPixelFormat(_format, QVideoFrameFormat::Format_RGBA8888);
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

bool GstDmaBufVideoBuffer::isTiled() const noexcept
{
    return (_drmModifier != DRM_FORMAT_MOD_INVALID) && (_drmModifier != DRM_FORMAT_MOD_LINEAR);
}

bool GstDmaBufVideoBuffer::importDisabled() noexcept
{
    return s_importDisabled.load(std::memory_order_relaxed);
}

void GstDmaBufVideoBuffer::resetCachedState() noexcept
{
    s_importDisabled.store(false, std::memory_order_relaxed);
    s_consecutiveImportFailures.store(0, std::memory_order_relaxed);
    s_loggedNoEglContext.store(false, std::memory_order_relaxed);
    s_loggedLayoutRejected.store(false, std::memory_order_relaxed);
    s_loggedTiledCpuMap.store(false, std::memory_order_relaxed);
    s_loggedProducerWaitTimeout.store(false, std::memory_order_relaxed);
#if defined(QGC_HAS_GST_VULKAN_GPU_PATH)
    GstDmaBufVulkan::resetCachedState();
#endif
}

namespace {

// Streaming thread (sink start): only the EGLDisplay Qt itself renders with can import for it, and Qt has none
// under GLX. Without one the path never matches, so DMABuf frames take the CPU path up front instead of failing
// every import.
void prepareDmaBufContext(HwVideoBufferContext& context)
{
    context.dmaBufEglDisplay = GstEglHelpers::qtPlatformDisplay();
    static std::atomic<bool> s_logged{false};
    if (!s_logged.exchange(true, std::memory_order_relaxed)) {
        if (context.dmaBufEglDisplay == EGL_NO_DISPLAY) {
            qCDebug(GstDmaBufLog) << "Qt renders without EGL on" << QGuiApplication::platformName()
                                  << "— DMABuf frames take the CPU path (xcb: QT_XCB_GL_INTEGRATION=xcb_egl)";
        } else {
            qCDebug(GstDmaBufLog) << "DMABuf zero-copy available on" << QGuiApplication::platformName();
        }
    }
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
void GstDmaBufVideoBuffer::recordImportFailureForTest(bool unrecoverable) noexcept
{
    recordImportFailure(unrecoverable);
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
#if defined(QGC_HAS_GST_VULKAN_GPU_PATH)
    if (QGCRhiCapture::deviceSnapshot().backend.load(std::memory_order_acquire) == static_cast<int>(QRhi::Vulkan) &&
        !GstDmaBufVulkan::canImport(_videoInfo)) {
        return false;
    }
#endif
    // A tiled layout imports only through the modifier attributes.
    return !isTiled() ||
           ((_eglDisplay != EGL_NO_DISPLAY) && GstEglHelpers::displaySupportsExtension(_eglDisplay, kModifiersExt));
}

QAbstractVideoBuffer::MapData GstDmaBufVideoBuffer::map(QVideoFrame::MapMode mode)
{
    if (isTiled()) {
        QGC_HW_WARN_ONCE(
            GstDmaBufLog, s_loggedTiledCpuMap,
            "tiled DMABuf frame has no CPU layout (modifier" << Qt::hex << _drmModifier << ") — frame not shown");
        return {};
    }
    return GstHwVideoBuffer::map(mode);
}

QVideoFrameTexturesUPtr GstDmaBufVideoBuffer::mapTextures(QRhi& rhi, QVideoFrameTexturesUPtr& old)
{
    const GstHwPathTelemetry::ScopedMapTimer mapTimer(HwVideoBufferPath::DmaBuf);
#if defined(QGC_HAS_GST_VULKAN_GPU_PATH)
    const bool vulkan = (rhi.backend() == QRhi::Vulkan);
#else
    constexpr bool vulkan = false;
    Q_UNUSED(old);
#endif
    GstBuffer* buffer = nullptr;
    if (!checkMapPreconditions(rhi, static_cast<int>(vulkan ? QRhi::Vulkan : QRhi::OpenGLES2), GstDmaBufLog(), s_diag,
                               buffer)) {
        return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
    }
    if (importDisabled()) {
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
    return importGl(rhi, buffer);
}

QVideoFrameTexturesUPtr GstDmaBufVideoBuffer::importGl(QRhi& rhi, GstBuffer* buffer)
{
    const EGLDisplay eglDpy = GstQtDmaBufImport::importDisplay(rhi);
    if (eglDpy == EGL_NO_DISPLAY) {
        QGC_HW_WARN_ONCE(GstDmaBufLog, s_loggedNoEglContext,
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
    if ((fourccs.count == 0) || (fourccs.count != int(GST_VIDEO_INFO_N_PLANES(&_videoInfo))) ||
        !locatePlanes(buffer, _videoInfo, fourccs, modifier, planes)) {
        QGC_HW_WARN_ONCE(GstDmaBufLog, s_loggedLayoutRejected,
                         "DMABuf layout not importable (format"
                             << gst_video_format_to_string(GST_VIDEO_INFO_FORMAT(&_videoInfo)) << "memories"
                             << gst_buffer_n_memory(buffer) << ") — CPU fallback");
        GstHwPathTelemetry::recordFallbackReason(HwVideoBufferPath::DmaBuf,
                                                 GstHwPathTelemetry::HwFallbackReason::ImportUnsupported);
        return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
    }

    if (!GstHwImportPreflight::preflightOrRecord(&rhi, HwVideoBufferPath::DmaBuf, pixelFormat, _format.frameSize())) {
        return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
    }

    waitForProducer(eglDpy, buffer);

    // The textures sample the dma-bufs directly: keep the decoder from recycling them until Qt drops the textures.
    std::shared_ptr<void> keepAlive(gst_buffer_ref(buffer),
                                    [](void* ref) { gst_buffer_unref(static_cast<GstBuffer*>(ref)); });
    GstQtDmaBufImport::ImportResult imported =
        GstQtDmaBufImport::importTextures(rhi, std::span(planes.data(), std::size_t(fourccs.count)), pixelFormat,
                                          _format.frameSize(), std::move(keepAlive));
    if (!imported.textures) {
        recordImportFailure(imported.unrecoverable);
        return GstHwPathTelemetry::fail(HwVideoBufferPath::DmaBuf);
    }
    s_consecutiveImportFailures.store(0, std::memory_order_relaxed);
    if (!s_diag.loggedFirstSuccess.exchange(true, std::memory_order_relaxed)) {
        qCDebug(GstDmaBufLog) << "First DMABuf import: size=" << _format.frameSize() << "format=" << pixelFormat
                              << "modifier=" << Qt::hex << modifier;
    }
    return std::move(imported.textures);
}

#endif  // QGC_HAS_GST_DMABUF_GPU_PATH
