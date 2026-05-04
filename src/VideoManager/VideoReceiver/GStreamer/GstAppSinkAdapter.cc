#include "GstAppSinkAdapter.h"
#include "HwBuffers/GstHwVideoBufferFactory.h"
#include "QGCLoggingCategory.h"
#include "gstqgc/gstqgcvideosinkbin.h"

#include <QtCore/QMetaObject>
#include <QtCore/QMutexLocker>

#include <chrono>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoFrameFormat>
#include <QtMultimedia/QVideoSink>

#include <gst/app/gstappsink.h>
#include <gst/video/gstvideometa.h>
#include <gst/video/video-hdr.h>
#include <gst/video/video-info.h>
// Umbrella — provides GstVideoOrientationMethod, gst_video_orientation_from_tag, and (when
// the build's gst-video supports it) the per-buffer GstVideoOrientationMeta accessor. Both
// pieces work without QGC_HAS_GST_VIDEO_ORIENTATION_META except the buffer-meta lookup.
#include <gst/video/video.h>
#if GST_CHECK_VERSION(1, 24, 0)
#  include <gst/video/video-info-dma.h>
#endif

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
#include "HwBuffers/GstDmaBufVideoBuffer.h"
#include <QtGui/QGuiApplication>
#include <qpa/qplatformnativeinterface.h>
#include <EGL/egl.h>
#endif
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
#include "HwBuffers/GstGlVideoBuffer.h"
#endif
#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
#include "HwBuffers/GstD3D11VideoBuffer.h"
#endif
#if defined(QGC_HAS_GST_D3D12_GPU_PATH)
#include "HwBuffers/GstD3D12VideoBuffer.h"
#endif
#if defined(QGC_HAS_GST_IOSURFACE_GPU_PATH)
#include "HwBuffers/GstIOSurfaceVideoBuffer.h"
#endif
#if defined(QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH)
#include "HwBuffers/GstAHardwareBufferVideoBuffer.h"
#include <EGL/egl.h>
#endif

QGC_LOGGING_CATEGORY(GstAppSinkAdapterLog, "Video.GStreamer.GstAppSinkAdapter")

QVideoFrameFormat::ColorSpace toQtColorSpace(GstVideoColorMatrix matrix)
{
    switch (matrix) {
    case GST_VIDEO_COLOR_MATRIX_BT601:    return QVideoFrameFormat::ColorSpace_BT601;
    case GST_VIDEO_COLOR_MATRIX_BT709:    return QVideoFrameFormat::ColorSpace_BT709;
    case GST_VIDEO_COLOR_MATRIX_BT2020:   return QVideoFrameFormat::ColorSpace_BT2020;
    case GST_VIDEO_COLOR_MATRIX_SMPTE240M: return QVideoFrameFormat::ColorSpace_BT709; // closest Qt equivalent
    case GST_VIDEO_COLOR_MATRIX_FCC:      return QVideoFrameFormat::ColorSpace_BT601;
    default:                              return QVideoFrameFormat::ColorSpace_Undefined;
    }
}

QVideoFrameFormat::ColorTransfer toQtColorTransfer(GstVideoTransferFunction transfer)
{
    // Mapping mirrors qt6/qtmultimedia/.../qgst.cpp QGstCaps::formatAndVideoInfo() — keep
    // in sync if Qt changes its mapping (last cross-check: Qt 6.10.3).
    switch (transfer) {
    case GST_VIDEO_TRANSFER_BT601:        return QVideoFrameFormat::ColorTransfer_BT601;
    case GST_VIDEO_TRANSFER_BT2020_10:
    case GST_VIDEO_TRANSFER_BT2020_12:
    case GST_VIDEO_TRANSFER_BT709:        return QVideoFrameFormat::ColorTransfer_BT709;
    case GST_VIDEO_TRANSFER_GAMMA20:      return QVideoFrameFormat::ColorTransfer_BT709; // best fit per Qt
    case GST_VIDEO_TRANSFER_SMPTE240M:    return QVideoFrameFormat::ColorTransfer_BT709; // near-identical to BT.709 per Qt qgst.cpp:424
    case GST_VIDEO_TRANSFER_GAMMA22:
    case GST_VIDEO_TRANSFER_SRGB:
    case GST_VIDEO_TRANSFER_ADOBERGB:     return QVideoFrameFormat::ColorTransfer_Gamma22;
    case GST_VIDEO_TRANSFER_GAMMA18:      return QVideoFrameFormat::ColorTransfer_Gamma22; // closest Qt equivalent
    case GST_VIDEO_TRANSFER_GAMMA28:      return QVideoFrameFormat::ColorTransfer_Gamma28;
    case GST_VIDEO_TRANSFER_GAMMA10:      return QVideoFrameFormat::ColorTransfer_Linear;
    case GST_VIDEO_TRANSFER_SMPTE2084:    return QVideoFrameFormat::ColorTransfer_ST2084;
    case GST_VIDEO_TRANSFER_ARIB_STD_B67: return QVideoFrameFormat::ColorTransfer_STD_B67;
    // GST_VIDEO_TRANSFER_LOG100 / LOG316 have no Qt equivalent — leave as Unknown
    default:                              return QVideoFrameFormat::ColorTransfer_Unknown;
    }
}

QVideoFrameFormat::PixelFormat toQtPixelFormat(GstVideoFormat fmt)
{
    switch (fmt) {
    case GST_VIDEO_FORMAT_BGRA:      return QVideoFrameFormat::Format_BGRA8888;
    case GST_VIDEO_FORMAT_RGBA:      return QVideoFrameFormat::Format_RGBA8888;
    case GST_VIDEO_FORMAT_BGRx:      return QVideoFrameFormat::Format_BGRX8888;
    case GST_VIDEO_FORMAT_RGBx:      return QVideoFrameFormat::Format_RGBX8888;
    // Qt6 has no 24-bit packed format; drop rather than corrupt stride arithmetic.
    case GST_VIDEO_FORMAT_BGR:
    case GST_VIDEO_FORMAT_RGB:       return QVideoFrameFormat::Format_Invalid;
    case GST_VIDEO_FORMAT_ARGB:      return QVideoFrameFormat::Format_ARGB8888;
    case GST_VIDEO_FORMAT_xRGB:      return QVideoFrameFormat::Format_XRGB8888;
    case GST_VIDEO_FORMAT_NV12:      return QVideoFrameFormat::Format_NV12;
    case GST_VIDEO_FORMAT_NV21:      return QVideoFrameFormat::Format_NV21;
    case GST_VIDEO_FORMAT_I420:      return QVideoFrameFormat::Format_YUV420P;
    case GST_VIDEO_FORMAT_Y42B:      return QVideoFrameFormat::Format_YUV422P;
    case GST_VIDEO_FORMAT_YV12:      return QVideoFrameFormat::Format_YV12;
    case GST_VIDEO_FORMAT_I420_10LE: return QVideoFrameFormat::Format_YUV420P10;
    case GST_VIDEO_FORMAT_P010_10LE: return QVideoFrameFormat::Format_P010;
    case GST_VIDEO_FORMAT_P016_LE:   return QVideoFrameFormat::Format_P016;
    case GST_VIDEO_FORMAT_AYUV:      return QVideoFrameFormat::Format_AYUV;
    case GST_VIDEO_FORMAT_YUY2:      return QVideoFrameFormat::Format_YUYV;
    case GST_VIDEO_FORMAT_UYVY:      return QVideoFrameFormat::Format_UYVY;
    case GST_VIDEO_FORMAT_GRAY8:     return QVideoFrameFormat::Format_Y8;
    case GST_VIDEO_FORMAT_GRAY16_LE: return QVideoFrameFormat::Format_Y16;
    default:                         return QVideoFrameFormat::Format_Invalid;
    }
}

namespace {

QVideoFrameFormat::ColorRange toQtColorRange(GstVideoColorRange range)
{
    switch (range) {
    case GST_VIDEO_COLOR_RANGE_0_255:   return QVideoFrameFormat::ColorRange_Full;
    case GST_VIDEO_COLOR_RANGE_16_235:  return QVideoFrameFormat::ColorRange_Video;
    default:                            return QVideoFrameFormat::ColorRange_Unknown;
    }
}

} // namespace

// Operates on the GstVideoOrientationMethod enum, which is in <gst/video/video.h>'s
// always-present subset — independent of QGC_HAS_GST_VIDEO_ORIENTATION_META.
void applyOrientationToFrame(QVideoFrame &frame, GstVideoOrientationMethod method)
{
    switch (method) {
    case GST_VIDEO_ORIENTATION_IDENTITY:
        frame.setRotation(QtVideo::Rotation::None);
        frame.setMirrored(false);
        break;
    case GST_VIDEO_ORIENTATION_90R:
        frame.setRotation(QtVideo::Rotation::Clockwise90);
        frame.setMirrored(false);
        break;
    case GST_VIDEO_ORIENTATION_180:
        frame.setRotation(QtVideo::Rotation::Clockwise180);
        frame.setMirrored(false);
        break;
    case GST_VIDEO_ORIENTATION_90L:
        frame.setRotation(QtVideo::Rotation::Clockwise270);
        frame.setMirrored(false);
        break;
    case GST_VIDEO_ORIENTATION_HORIZ:
        frame.setRotation(QtVideo::Rotation::None);
        frame.setMirrored(true);
        break;
    case GST_VIDEO_ORIENTATION_VERT:
        frame.setRotation(QtVideo::Rotation::Clockwise180);
        frame.setMirrored(true);
        break;
    case GST_VIDEO_ORIENTATION_UL_LR:
        frame.setRotation(QtVideo::Rotation::Clockwise90);
        frame.setMirrored(true);
        break;
    case GST_VIDEO_ORIENTATION_UR_LL:
        frame.setRotation(QtVideo::Rotation::Clockwise270);
        frame.setMirrored(true);
        break;
    default:
        static bool s_warnedUnhandled = false;
        if (!s_warnedUnhandled) {
            s_warnedUnhandled = true;
            qCWarning(GstAppSinkAdapterLog) << "Unhandled GstVideoOrientationMethod" << method << "— treating as identity";
        }
        frame.setRotation(QtVideo::Rotation::None);
        frame.setMirrored(false);
        break;
    }
}

namespace {

void applyColorimetry(QVideoFrameFormat &format, const GstVideoInfo &info, GstCaps *caps)
{
    const GstVideoColorimetry &colorimetry = GST_VIDEO_INFO_COLORIMETRY(&info);
    QVideoFrameFormat::ColorSpace colorSpace = toQtColorSpace(colorimetry.matrix);
    // Live RTSP sources often omit colorimetry caps; resolution heuristic matches mpv.
    if (colorSpace == QVideoFrameFormat::ColorSpace_Undefined) {
        const int height = GST_VIDEO_INFO_HEIGHT(&info);
        if (height > 0) {
            colorSpace = (height <= 720) ? QVideoFrameFormat::ColorSpace_BT601
                                         : QVideoFrameFormat::ColorSpace_BT709;
        }
    }
    format.setColorSpace(colorSpace);
    format.setColorTransfer(toQtColorTransfer(colorimetry.transfer));
    QVideoFrameFormat::ColorRange range = toQtColorRange(colorimetry.range);
    // H.264/H.265 omit VUI range but encode limited per spec — Unknown skips Qt's limited→full offset.
    if (range == QVideoFrameFormat::ColorRange_Unknown
            && colorimetry.matrix != GST_VIDEO_COLOR_MATRIX_RGB) {
        range = QVideoFrameFormat::ColorRange_Video;
    }
    format.setColorRange(range);

    // Prefer MaxCLL (tighter tone-mapping target) over mastering-display max-luminance.
    GstVideoContentLightLevel cll;
    bool clipApplied = false;
    if (caps && gst_video_content_light_level_from_caps(&cll, caps)
            && cll.max_content_light_level > 0) {
        format.setMaxLuminance(static_cast<float>(cll.max_content_light_level));
        clipApplied = true;
    }
    if (!clipApplied) {
        GstVideoMasteringDisplayInfo masteringInfo;
        if (caps && gst_video_mastering_display_info_from_caps(&masteringInfo, caps)) {
            // GstVideoMasteringDisplayColorVolume max_luma is in 0.0001 cd/m².
            const double maxLuminance = static_cast<double>(masteringInfo.max_display_mastering_luminance) / 10000.0;
            if (maxLuminance > 0.0) {
                format.setMaxLuminance(static_cast<float>(maxLuminance));
            }
        }
    }
}

// Definition lives outside the anonymous namespace; declaration in GstAppSinkAdapter.h.

void applyOrientationAndTiming(QVideoFrame &frame, [[maybe_unused]] GstBuffer *buffer,
                               int streamOrientation)
{
    // Per-buffer meta wins (per-frame override) — only available when the build's gst-video
    // exports gst_buffer_get_video_orientation_meta. The stream-level fallback works on every
    // gst-video install (gst_video_orientation_from_tag is in the umbrella header).
#ifdef QGC_HAS_GST_VIDEO_ORIENTATION_META
    if (GstVideoOrientationMeta *meta = gst_buffer_get_video_orientation_meta(buffer)) {
        applyOrientationToFrame(frame, meta->orientation);
    } else
#endif
    if (streamOrientation != static_cast<int>(GST_VIDEO_ORIENTATION_IDENTITY)) {
        applyOrientationToFrame(frame, static_cast<GstVideoOrientationMethod>(streamOrientation));
    }
    if (GST_BUFFER_PTS_IS_VALID(buffer)) {
        // GstClockTime is ns; QVideoFrame timestamps are µs.
        frame.setStartTime(GST_BUFFER_PTS(buffer) / GST_USECOND);
        if (GST_BUFFER_DURATION_IS_VALID(buffer)) {
            frame.setEndTime((GST_BUFFER_PTS(buffer) + GST_BUFFER_DURATION(buffer)) / GST_USECOND);
        }
    }
}

void pushFrameQueued(QPointer<QVideoSink> sink, QVideoFrame &&frame)
{
    // Take QPointer by value: callers extract under _stateMutex and pass through, so we never construct a QPointer from a possibly-dangling raw pointer (UB) — the QPointer's own atomic guard tracks destruction across the snapshot→deliver window.
    if (!sink) return;
    // AutoConnection: direct call when already on the sink's thread, queued otherwise — mirrors Qt's qgstreamervideosink.cpp pattern.
    QMetaObject::invokeMethod(sink.data(), [sink, f = std::move(frame)]() {
        if (sink) sink->setVideoFrame(f);
    }, Qt::AutoConnection);
}

} // namespace

// QQuickVideoOutput computes its sample rect as viewport/frameSize (qquickvideooutput.cpp:498);
// matches Qt's own gstreamer renderer (qgstvideorenderersink.cpp:230). externalTextureMatrix
// is only consulted for Format_SamplerExternalOES, so it can't be used for crop on standard formats.
QVideoFrameFormat applyCropMeta(QVideoFrameFormat format, GstBuffer *buffer)
{
    if (GstVideoCropMeta *crop = gst_buffer_get_video_crop_meta(buffer)) {
        format.setViewport(QRect(crop->x, crop->y, crop->width, crop->height));
    }
    return format;
}

void GstAppSinkAdapter::_logFrameStats() const
{
    const quint64 cpu = _cpuFrames.load(std::memory_order_relaxed);
    quint64 totalThisCall = cpu;
    QString s = QStringLiteral("CPU:%1").arg(cpu);
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    const quint64 dma = _gpuFrames.load(std::memory_order_relaxed);
    s += QStringLiteral(" DMABuf:%1/%2").arg(dma).arg(GstDmaBufVideoBuffer::peekMapFailureCount());
    totalThisCall += dma;
#endif
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
    const quint64 gl = _glFrames.load(std::memory_order_relaxed);
    s += QStringLiteral(" GL:%1/%2").arg(gl).arg(GstGlVideoBuffer::peekMapFailureCount());
    totalThisCall += gl;
#endif
#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
    const quint64 d3d = _d3d11Frames.load(std::memory_order_relaxed);
    s += QStringLiteral(" D3D11:%1/%2").arg(d3d).arg(GstD3D11VideoBuffer::peekMapFailureCount());
    totalThisCall += d3d;
#endif
#if defined(QGC_HAS_GST_D3D12_GPU_PATH)
    const quint64 d3d12 = _d3d12Frames.load(std::memory_order_relaxed);
    s += QStringLiteral(" D3D12:%1/%2").arg(d3d12).arg(GstD3D12VideoBuffer::peekMapFailureCount());
    totalThisCall += d3d12;
#endif
#if defined(QGC_HAS_GST_IOSURFACE_GPU_PATH)
    const quint64 ios = _iosurfaceFrames.load(std::memory_order_relaxed);
    s += QStringLiteral(" IOSurface:%1/%2").arg(ios).arg(GstIOSurfaceVideoBuffer::peekMapFailureCount());
    totalThisCall += ios;
#endif
#if defined(QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH)
    const quint64 ahwb = _ahwbFrames.load(std::memory_order_relaxed);
    s += QStringLiteral(" AHWBuf:%1/%2").arg(ahwb).arg(GstAHardwareBufferVideoBuffer::peekMapFailureCount());
    totalThisCall += ahwb;
#endif

    QString tail;
    {
        QMutexLocker locker(&_stateMutex);
        if (!_cachedAllocatorName.isEmpty()) {
            const int w = GST_VIDEO_INFO_WIDTH(&_cachedInfo);
            const int h = GST_VIDEO_INFO_HEIGHT(&_cachedInfo);
            const char *fmt = gst_video_format_to_string(GST_VIDEO_INFO_FORMAT(&_cachedInfo));
            tail = QStringLiteral(" (alloc=%1 %2x%3 %4")
                       .arg(_cachedAllocatorName).arg(w).arg(h).arg(QLatin1String(fmt));
        }
    }

    const qint64 nowNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
                             std::chrono::steady_clock::now().time_since_epoch()).count();
    const qint64 prevNs = _lastStatsAtNs.exchange(nowNs, std::memory_order_relaxed);
    if (prevNs != 0 && nowNs > prevNs && !tail.isEmpty()) {
        // Approximate: total counters are cumulative, so derive delta against the previous _logFrameStats call.
        // Skip the first call (prevNs == 0) since no window exists yet.
        const double seconds = static_cast<double>(nowNs - prevNs) / 1e9;
        const quint64 prevTotal = _lastStatsTotal.exchange(totalThisCall, std::memory_order_relaxed);
        if (totalThisCall > prevTotal && seconds > 0.0) {
            const double fps = static_cast<double>(totalThisCall - prevTotal) / seconds;
            tail += QStringLiteral(" ~%1fps").arg(fps, 0, 'f', 1);
        }
        tail += QLatin1Char(')');
    } else if (!tail.isEmpty()) {
        _lastStatsTotal.store(totalThisCall, std::memory_order_relaxed);
        tail += QLatin1Char(')');
    }

    qCDebug(GstAppSinkAdapterLog).noquote() << "Frame stats —" << s << tail;
}

GstAppSinkAdapter::GstAppSinkAdapter(QObject *parent)
    : QObject(parent)
{
}

// Pipeline must be NULL before destroying the adapter; teardown() does not block in-flight callbacks.
GstAppSinkAdapter::~GstAppSinkAdapter()
{
    teardown();
}

bool GstAppSinkAdapter::setup(GstElement *sinkBin, QVideoSink *videoSink)
{
    if (!sinkBin || !videoSink) {
        qCWarning(GstAppSinkAdapterLog) << "setup() called with null arguments";
        return false;
    }

    teardown();

    if (!GST_IS_QGC_VIDEO_SINK_BIN(sinkBin)) {
        qCWarning(GstAppSinkAdapterLog) << "sinkBin is not a GstQgcVideoSinkBin";
        return false;
    }

    _appsink = gst_qgc_video_sink_bin_get_appsink(GST_QGC_VIDEO_SINK_BIN(sinkBin));
    if (!_appsink) {
        qCWarning(GstAppSinkAdapterLog) << "qgcvideosinkbin has no appsink (not constructed?)";
        return false;
    }

    {
        QMutexLocker locker(&_stateMutex);
        _videoSink = videoSink;
    }

#if defined(QGC_HAS_ANY_GPU_PATH)
    // Bin owns the path: it set its `gpu-zerocopy` GObject prop at construct time. Reading it back here keeps adapter telemetry in lockstep with whichever pipeline the bin actually built — no fact-vs-bin desync.
    {
        gboolean binZeroCopy = FALSE;
        g_object_get(sinkBin, "gpu-zerocopy", &binZeroCopy, NULL);
        _gpuPathEnabled = binZeroCopy ? true : false;
    }
#endif
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    if (_gpuPathEnabled) {
        // Construction-time hint only — render-time the buffer's mapTextures() prefers eglGetCurrentDisplay() over this so xcb_egl mismatches between Qt's actual EGLDisplay and eglGetDisplay(EGL_DEFAULT_DISPLAY) don't cause silent black-frame imports.
        _eglDisplay = EGL_NO_DISPLAY;
        const QString platform = QGuiApplication::platformName();
        if (platform == QLatin1String("wayland") || platform == QLatin1String("wayland-egl")) {
            if (auto *ni = QGuiApplication::platformNativeInterface()) {
                _eglDisplay = static_cast<EGLDisplay>(ni->nativeResourceForIntegration("egldisplay"));
            }
        }
        if (_eglDisplay == EGL_NO_DISPLAY) {
            _eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        }
        if (_eglDisplay == EGL_NO_DISPLAY) {
            qCWarning(GstAppSinkAdapterLog) << "GPU zero-copy requested but EGLDisplay unavailable on platform"
                                            << platform << "— DMABuf path disabled";
        } else {
            qCInfo(GstAppSinkAdapterLog) << "DMABuf zero-copy path available on" << platform
                                         << "— actual path chosen at caps negotiation";
        }
    }
#endif
#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
    if (_gpuPathEnabled) {
        qCInfo(GstAppSinkAdapterLog) << "D3D11 zero-copy path available — actual path chosen at caps negotiation";
    }
#endif
#if defined(QGC_HAS_GST_D3D12_GPU_PATH)
    if (_gpuPathEnabled) {
        qCInfo(GstAppSinkAdapterLog) << "D3D12 zero-copy path available — actual path chosen at caps negotiation";
    }
#endif
#if defined(QGC_HAS_GST_IOSURFACE_GPU_PATH)
    if (_gpuPathEnabled) {
        qCInfo(GstAppSinkAdapterLog) << "IOSurface zero-copy path available — actual path chosen at caps negotiation";
    }
#endif
#if defined(QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH)
    if (_gpuPathEnabled) {
        _ahwbEglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (_ahwbEglDisplay == EGL_NO_DISPLAY) {
            qCWarning(GstAppSinkAdapterLog) << "AHardwareBuffer path: EGLDisplay unavailable";
        } else {
            qCInfo(GstAppSinkAdapterLog) << "AHardwareBuffer zero-copy path available"
                                         << "— actual path chosen at caps negotiation";
        }
    }
#endif

    GstAppSinkCallbacks callbacks{};
    callbacks.new_sample = onNewSample;
    // teardown() clears callbacks before unref'ing _appsink — destroy_notify=NULL is safe.
    gst_app_sink_set_callbacks(GST_APP_SINK(_appsink), &callbacks, this, nullptr);

    // Install a sink-pad probe so we can observe every buffer that *reaches* the appsink,
    // even ones the appsink later drops via max-buffers=1/drop=TRUE. Difference vs delivered
    // counters = appsink-level drop pressure (separate from decoder QoS drops).
    _appsinkInputFrames.store(0, std::memory_order_relaxed);
    _flushing.store(false, std::memory_order_relaxed);
    if (GstPad *sinkPad = gst_element_get_static_pad(_appsink, "sink")) {
        // BUFFER + EVENT_DOWNSTREAM + EVENT_FLUSH in one probe. BUFFER feeds the input counter.
        // EVENT_DOWNSTREAM catches serialized events (FLUSH_STOP arrives serialized after data).
        // EVENT_FLUSH is REQUIRED to catch FLUSH_START — that event is non-serialized and bypasses
        // the streaming thread, so EVENT_DOWNSTREAM alone would miss it (per gstpad.h:484-487).
        _appsinkProbeId = gst_pad_add_probe(sinkPad,
                                            GstPadProbeType(GST_PAD_PROBE_TYPE_BUFFER
                                                            | GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM
                                                            | GST_PAD_PROBE_TYPE_EVENT_FLUSH),
                                            &GstAppSinkAdapter::appsinkBufferProbe, this, nullptr);
        if (_appsinkProbeId == 0) {
            qCWarning(GstAppSinkAdapterLog) << "gst_pad_add_probe(BUFFER) returned 0 — appsink drop counter disabled";
            gst_object_unref(sinkPad);
        } else {
            // Hold the ref: removal in teardown() targets the pad regardless of _appsink lifetime.
            _appsinkProbePad = sinkPad;
        }
    } else {
        qCWarning(GstAppSinkAdapterLog) << "Could not obtain appsink sink pad — drop counter disabled";
    }

    _telemetryEmitTimer.setInterval(1000);
    _telemetryEmitTimer.setSingleShot(false);
    // setup() is idempotent — disconnect first so re-setup doesn't stack lambda emitters.
    QObject::disconnect(&_telemetryEmitTimer, &QTimer::timeout, this, nullptr);
    QObject::connect(&_telemetryEmitTimer, &QTimer::timeout, this, [this]() {
        quint64 total = _cpuFrames.load(std::memory_order_relaxed);
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
        total += _gpuFrames.load(std::memory_order_relaxed);
#endif
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
        total += _glFrames.load(std::memory_order_relaxed);
#endif
#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
        total += _d3d11Frames.load(std::memory_order_relaxed);
#endif
#if defined(QGC_HAS_GST_D3D12_GPU_PATH)
        total += _d3d12Frames.load(std::memory_order_relaxed);
#endif
#if defined(QGC_HAS_GST_IOSURFACE_GPU_PATH)
        total += _iosurfaceFrames.load(std::memory_order_relaxed);
#endif
#if defined(QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH)
        total += _ahwbFrames.load(std::memory_order_relaxed);
#endif
        if (total != _lastEmittedFrameTotal) {
            _lastEmittedFrameTotal = total;
            emit frameCountsChanged();
        }
    });
    // QTimer::start must run on its owning thread (GUI); setup() may be on the streaming thread.
    QMetaObject::invokeMethod(&_telemetryEmitTimer, qOverload<>(&QTimer::start), Qt::QueuedConnection);

    qCDebug(GstAppSinkAdapterLog) << "Installed appsink callbacks";
    return true;
}

void GstAppSinkAdapter::setRefreshRate(qreal hz)
{
    if (hz < 1.0) {
        return;
    }
    const quint64 periodNs = static_cast<quint64>(GST_SECOND / hz);
    _refreshPeriodNs.store(periodNs, std::memory_order_release);

    // Apply baseline now so first-frame budget isn't capped by the bin's 33 ms default.
#if defined(QGC_GST_BUILD_VERSION_MAJOR) && \
    (QGC_GST_BUILD_VERSION_MAJOR > 1 || \
     (QGC_GST_BUILD_VERSION_MAJOR == 1 && QGC_GST_BUILD_VERSION_MINOR >= 24))
    QMutexLocker locker(&_stateMutex);
    if (_appsink) {
        gst_app_sink_set_max_time(GST_APP_SINK(_appsink), static_cast<GstClockTime>(periodNs));
    }
#endif
}

void GstAppSinkAdapter::setSmoothingEnabled(bool enabled, qreal refreshHz)
{
    if (enabled == _smoothingEnabled.load(std::memory_order_acquire)) {
        return;
    }
    if (enabled) {
        // refreshHz may be 0 (headless) or NaN (broken QScreen) — clamp.
        const int hz = (refreshHz >= 1.0 && refreshHz <= 240.0) ? int(qRound(refreshHz)) : 60;
        const int periodMs = qMax(1, int(qRound(1000.0 / hz)));
        // Start clock before publishing enabled=true so streaming thread sees a started clock.
        _smoothingClock.start();
        connect(&_smoothingTickTimer, &QTimer::timeout,
                this, &GstAppSinkAdapter::_onSmoothingTick, Qt::UniqueConnection);
        _smoothingTickTimer.setInterval(periodMs);
        _smoothingTickTimer.setTimerType(Qt::PreciseTimer);
        _smoothingEnabled.store(true, std::memory_order_release);
        QMetaObject::invokeMethod(&_smoothingTickTimer, qOverload<>(&QTimer::start),
                                  Qt::QueuedConnection);
        qCInfo(GstAppSinkAdapterLog) << "Smoothing ring on: tick=" << periodMs
            << "ms threshold=" << (kSmoothingThresholdNs / 1000000) << "ms capacity="
            << kSmoothingRingCapacity;
    } else {
        _smoothingEnabled.store(false, std::memory_order_release);
        QMetaObject::invokeMethod(&_smoothingTickTimer, &QTimer::stop, Qt::QueuedConnection);
        QMutexLocker lock(&_smoothingMutex);
        _smoothingRing.clear();
        _smoothingFirstPtsNs = -1;
    }
}

void GstAppSinkAdapter::_deliverFrame(QPointer<QVideoSink> sink, QVideoFrame &&frame, int64_t ptsNs)
{
    if (!_smoothingEnabled.load(std::memory_order_acquire)) {
        pushFrameQueued(sink, std::move(frame));
        return;
    }
    const qint64 nowNs = _smoothingClock.nsecsElapsed();
    QMutexLocker lock(&_smoothingMutex);
    if (_smoothingRing.size() >= kSmoothingRingCapacity) {
        _smoothingRing.removeFirst();
        const quint64 c = _smoothingDroppedFrames.fetch_add(1, std::memory_order_relaxed) + 1;
        if ((c & 0x3F) == 1) {
            qCDebug(GstAppSinkAdapterLog) << "Smoothing ring overflow; dropped oldest (total=" << c << ")";
        }
    }
    _smoothingRing.append({std::move(frame),
                           ptsNs >= 0 ? ptsNs : static_cast<int64_t>(nowNs),
                           nowNs});
}

void GstAppSinkAdapter::_onSmoothingTick()
{
    QPointer<QVideoSink> sinkSnapshot;
    {
        QMutexLocker locker(&_stateMutex);
        sinkSnapshot = _videoSink;
    }
    if (!sinkSnapshot) {
        return;
    }

    QVideoFrame chosen;
    {
        QMutexLocker lock(&_smoothingMutex);
        if (_smoothingRing.isEmpty()) {
            return; // hold last good frame on freeze
        }
        if (_smoothingFirstPtsNs < 0) {
            _smoothingFirstPtsNs = _smoothingRing.first().ptsNs;
            _smoothingFirstClockNs = _smoothingRing.first().enqueuedNs;
        }
        const qint64 nowNs = _smoothingClock.nsecsElapsed();
        const int64_t targetPts = _smoothingFirstPtsNs + (nowNs - _smoothingFirstClockNs);
        int bestIdx = 0;
        int64_t bestDelta = std::llabs(_smoothingRing[0].ptsNs - targetPts);
        for (int i = 1; i < _smoothingRing.size(); ++i) {
            const int64_t d = std::llabs(_smoothingRing[i].ptsNs - targetPts);
            if (d < bestDelta) {
                bestDelta = d;
                bestIdx = i;
            }
        }
        if (bestDelta > kSmoothingThresholdNs) {
            // Re-anchor on next tick; ring contents become candidates for the new anchor.
            _smoothingFirstPtsNs = -1;
            return;
        }
        chosen = _smoothingRing[bestIdx].frame;
        // Drop chosen + older so the same frame can't be picked again.
        for (int i = bestIdx; i >= 0; --i) {
            _smoothingRing.removeAt(i);
        }
    }
    pushFrameQueued(sinkSnapshot, std::move(chosen));
}

void GstAppSinkAdapter::teardown()
{
    // teardown() may fire from the streaming thread; queue stop to the timer's owner.
    QMetaObject::invokeMethod(&_telemetryEmitTimer, &QTimer::stop, Qt::QueuedConnection);
    // acq_rel exchange flips enabled then drains the ring so late _deliverFrame() can't refill.
    if (_smoothingEnabled.exchange(false, std::memory_order_acq_rel)) {
        QMetaObject::invokeMethod(&_smoothingTickTimer, &QTimer::stop, Qt::QueuedConnection);
        QMutexLocker lock(&_smoothingMutex);
        _smoothingRing.clear();
        _smoothingFirstPtsNs = -1;
    }

    {
        QString stats = QStringLiteral("CPU:%1").arg(_cpuFrames.load(std::memory_order_relaxed));
        quint64 totalFrames = _cpuFrames.load(std::memory_order_relaxed);
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
        const quint64 dmaFailures = GstDmaBufVideoBuffer::takeMapFailureCount();
        stats += QStringLiteral(" DMABuf:%1 DMABuf-failures:%2").arg(_gpuFrames.load(std::memory_order_relaxed)).arg(dmaFailures);
        totalFrames += _gpuFrames.load(std::memory_order_relaxed) + dmaFailures;
        _gpuFrames.store(0, std::memory_order_relaxed);
#endif
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
        const quint64 glFailures = GstGlVideoBuffer::takeMapFailureCount();
        const quint64 glReuse = GstGlVideoBuffer::takeTextureReuseHits();
        quint64 glGpuWaits = 0;
        const quint64 glCpuWaits = GstGlVideoBuffer::takeSyncWaitCounts(glGpuWaits);
        stats += QStringLiteral(" GL:%1 GL-failures:%2 GL-reuse:%3 GL-wait[gpu/cpu]:%4/%5")
                     .arg(_glFrames.load(std::memory_order_relaxed))
                     .arg(glFailures).arg(glReuse).arg(glGpuWaits).arg(glCpuWaits);
        totalFrames += _glFrames.load(std::memory_order_relaxed) + glFailures;
        _glFrames.store(0, std::memory_order_relaxed);
#endif
#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
        const quint64 d3dFailures = GstD3D11VideoBuffer::takeMapFailureCount();
        stats += QStringLiteral(" D3D11:%1 D3D11-failures:%2").arg(_d3d11Frames.load(std::memory_order_relaxed)).arg(d3dFailures);
        totalFrames += _d3d11Frames.load(std::memory_order_relaxed) + d3dFailures;
        _d3d11Frames.store(0, std::memory_order_relaxed);
#endif
#if defined(QGC_HAS_GST_D3D12_GPU_PATH)
        const quint64 d3d12Failures = GstD3D12VideoBuffer::takeMapFailureCount();
        stats += QStringLiteral(" D3D12:%1 D3D12-failures:%2").arg(_d3d12Frames.load(std::memory_order_relaxed)).arg(d3d12Failures);
        totalFrames += _d3d12Frames.load(std::memory_order_relaxed) + d3d12Failures;
        _d3d12Frames.store(0, std::memory_order_relaxed);
#endif
#if defined(QGC_HAS_GST_IOSURFACE_GPU_PATH)
        const quint64 iosFailures = GstIOSurfaceVideoBuffer::takeMapFailureCount();
        stats += QStringLiteral(" IOSurface:%1 IOSurface-failures:%2").arg(_iosurfaceFrames.load(std::memory_order_relaxed)).arg(iosFailures);
        totalFrames += _iosurfaceFrames.load(std::memory_order_relaxed) + iosFailures;
        _iosurfaceFrames.store(0, std::memory_order_relaxed);
#endif
#if defined(QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH)
        const quint64 ahwbFailures = GstAHardwareBufferVideoBuffer::takeMapFailureCount();
        stats += QStringLiteral(" AHWBuf:%1 AHWBuf-failures:%2").arg(_ahwbFrames.load(std::memory_order_relaxed)).arg(ahwbFailures);
        totalFrames += _ahwbFrames.load(std::memory_order_relaxed) + ahwbFailures;
        _ahwbFrames.store(0, std::memory_order_relaxed);
#endif
        if (totalFrames > 0) {
            qCInfo(GstAppSinkAdapterLog).noquote() << "Adapter teardown —" << stats;
        }
    }
    _cpuFrames.store(0, std::memory_order_relaxed);
    _lastEmittedFrameTotal = 0;

    if (_appsink) {
        // userData=nullptr ensures any racing new_sample callback receives nullptr and bails early;
        // set_callbacks takes appsink's internal mutex before swapping the callbacks struct.
        GstAppSinkCallbacks empty{};
        gst_app_sink_set_callbacks(GST_APP_SINK(_appsink), &empty, nullptr, nullptr);
        // Drain any buffer queued before the callbacks swap took effect.
        while (GstSample *s = gst_app_sink_try_pull_sample(GST_APP_SINK(_appsink), 0)) {
            gst_sample_unref(s);
        }
    }
    // Drop the probe before releasing the pad ref. gst_pad_remove_probe is a no-op on id=0.
    if (_appsinkProbeId != 0 && _appsinkProbePad) {
        gst_pad_remove_probe(_appsinkProbePad, _appsinkProbeId);
    }
    _appsinkProbeId = 0;
    gst_clear_object(&_appsinkProbePad);
    _appsinkInputFrames.store(0, std::memory_order_relaxed);
    gst_clear_object(&_appsink);

    {
        QMutexLocker locker(&_stateMutex);
        _videoSink = nullptr;
        // Drop the held caps ref; next setup() call repopulates on first sample.
        if (_cachedCapsKey) {
            gst_caps_unref(_cachedCapsKey);
            _cachedCapsKey = nullptr;
        }
        _cachedFormat = QVideoFrameFormat();
        _cachedPixelFormat = QVideoFrameFormat::Format_Invalid;
    }

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    _eglDisplay = EGL_NO_DISPLAY;
#endif
#if defined(QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH)
    _ahwbEglDisplay = EGL_NO_DISPLAY;
#endif
#if defined(QGC_HAS_ANY_GPU_PATH)
    _gpuPathEnabled = false;
#endif
    _active.store(true, std::memory_order_release);
    _qosSampleCount = 0;
    _qosAvgRate = 1.0;
    _qosLastPts = GST_CLOCK_TIME_NONE;
    _qosLastArrivalNs = 0;
    _pipelineMinLatencyNs = 0;
    _latencyValid = false;
    _latencyRefreshPending.store(false, std::memory_order_relaxed);
}

void GstAppSinkAdapter::setActive(bool active)
{
    const bool was = _active.exchange(active, std::memory_order_acq_rel);
    if (was == active) return;
    if (active) return;
    // false transition: clear the sink with one empty frame so the previous stream's last
    // image doesn't ghost. snapshot the QVideoSink under lock — same dangling-pointer
    // contract as onNewSample (sink may be destroyed on its owner thread).
    QPointer<QVideoSink> sinkSnapshot;
    {
        QMutexLocker locker(&_stateMutex);
        sinkSnapshot = _videoSink;
    }
    if (sinkSnapshot) {
        pushFrameQueued(sinkSnapshot, QVideoFrame{});
    }
}

GstFlowReturn GstAppSinkAdapter::onNewSample(GstAppSink *appsink, gpointer userData)
{
    auto *self = static_cast<GstAppSinkAdapter *>(userData);
    // nullptr after teardown() swaps in an empty callbacks struct with userData=nullptr.
    if (!self) return GST_FLOW_OK;

    // Drop the sample if a flush is in progress: a new_sample callback can race ahead of
    // FLUSH_START between the upstream serializer and the appsink's queue. Returning
    // GST_FLOW_FLUSHING tells appsink to discard without state-change side effects.
    if (self->_flushing.load(std::memory_order_acquire)) {
        if (GstSample *drop = gst_app_sink_try_pull_sample(appsink, 0)) {
            gst_sample_unref(drop);
        }
        return GST_FLOW_FLUSHING;
    }

    // Inactive sink: pull-and-discard so appsink's queue doesn't back-pressure upstream.
    if (!self->_active.load(std::memory_order_acquire)) {
        if (GstSample *drop = gst_app_sink_try_pull_sample(appsink, 0)) {
            gst_sample_unref(drop);
        }
        return GST_FLOW_OK;
    }

    GstSample *sample = gst_app_sink_pull_sample(appsink);
    if (!sample) {
        return GST_FLOW_ERROR;
    }

    GstBuffer *buffer = gst_sample_get_buffer(sample);
    GstCaps *caps = gst_sample_get_caps(sample);
    if (!buffer || !caps) {
        gst_sample_unref(sample);
        return GST_FLOW_ERROR;
    }

    // Copy the QPointer member directly so the snapshot stays sin-aware across the queued-delivery window — extracting a raw pointer here would dangle if the QVideoSink is destroyed on its owner thread before pushFrameQueued constructs its QPointer.
    QPointer<QVideoSink> sinkSnapshot;
    {
        QMutexLocker locker(&self->_stateMutex);
        sinkSnapshot = self->_videoSink;
    }
    if (!sinkSnapshot) {
        gst_sample_unref(sample);
        return GST_FLOW_OK;
    }

    // Lock pattern: check key → on miss parse outside lock → relock to store.
    GstVideoInfo localInfo{};
    QVideoFrameFormat localFormat;
    int localPixelFormat = 0;

    {
        QMutexLocker locker(&self->_stateMutex);
        if (caps == self->_cachedCapsKey) {
            localInfo = self->_cachedInfo;
            localFormat = self->_cachedFormat;
            localPixelFormat = self->_cachedPixelFormat;
        } else {
            localPixelFormat = -1; // sentinel: cache miss, parse below
        }
    }

    if (localPixelFormat == -1) {
        GstVideoInfo parsedInfo{};
        // GStreamer 1.24+ va plugin advertises DMABuf as format=DMA_DRM with drm-format=<fourcc:modifier>; gst_video_info_from_caps doesn't understand DMA_DRM and would fail here, so go through the DMA-DRM-aware variant first.
#if GST_CHECK_VERSION(1, 24, 0)
        if (gst_video_is_dma_drm_caps(caps)) {
            GstVideoInfoDmaDrm drmInfo;
            gst_video_info_dma_drm_init(&drmInfo);
            if (!gst_video_info_dma_drm_from_caps(&drmInfo, caps)
                || !gst_video_info_dma_drm_to_video_info(&drmInfo, &parsedInfo)) {
                qCWarning(GstAppSinkAdapterLog) << "Failed to parse DMA-DRM video info from caps";
                gst_sample_unref(sample);
                return GST_FLOW_ERROR;
            }
        } else
#endif
        if (!gst_video_info_from_caps(&parsedInfo, caps)) {
            qCWarning(GstAppSinkAdapterLog) << "Failed to parse video info from caps";
            gst_sample_unref(sample);
            return GST_FLOW_ERROR;
        }
        const QVideoFrameFormat::PixelFormat pixelFormat = toQtPixelFormat(
            GST_VIDEO_INFO_FORMAT(&parsedInfo));
        if (pixelFormat == QVideoFrameFormat::Format_Invalid) {
            const GstVideoFormat fmt = GST_VIDEO_INFO_FORMAT(&parsedInfo);
            if (self->_lastWarnedFormat.exchange(fmt, std::memory_order_relaxed) != fmt) {
                qCWarning(GstAppSinkAdapterLog) << "Unsupported video format"
                    << gst_video_format_to_string(fmt);
            }
            gst_sample_unref(sample);
            return GST_FLOW_ERROR;
        }
        const int w = GST_VIDEO_INFO_WIDTH(&parsedInfo);
        const int h = GST_VIDEO_INFO_HEIGHT(&parsedInfo);
        if (w <= 0 || h <= 0) {
            gst_sample_unref(sample);
            return GST_FLOW_ERROR;
        }
        QVideoFrameFormat parsedFormat(QSize(w, h), pixelFormat);
        applyColorimetry(parsedFormat, parsedInfo, caps);
        const int fpsN = GST_VIDEO_INFO_FPS_N(&parsedInfo);
        const int fpsD = GST_VIDEO_INFO_FPS_D(&parsedInfo);
        if (fpsN > 0 && fpsD > 0) {
            parsedFormat.setStreamFrameRate(static_cast<qreal>(fpsN) / static_cast<qreal>(fpsD));
            // max(refreshPeriod, framePeriod): refresh is floor (don't drop early), frame is ceiling.
#if defined(QGC_GST_BUILD_VERSION_MAJOR) && \
    (QGC_GST_BUILD_VERSION_MAJOR > 1 || \
     (QGC_GST_BUILD_VERSION_MAJOR == 1 && QGC_GST_BUILD_VERSION_MINOR >= 24))
            const quint64 framePeriodNs = static_cast<quint64>(GST_SECOND) * static_cast<quint64>(fpsD)
                                          / static_cast<quint64>(fpsN);
            const quint64 refreshNs = self->_refreshPeriodNs.load(std::memory_order_acquire);
            const GstClockTime maxTime = static_cast<GstClockTime>(qMax(framePeriodNs, refreshNs));
            gst_app_sink_set_max_time(appsink, maxTime);
#endif
        }

        QString allocName;
        {
            GstMemory *mem0 = gst_buffer_peek_memory(buffer, 0);
            allocName = QString::fromUtf8((mem0 && mem0->allocator) ? mem0->allocator->mem_type : "(none)");
            qCInfo(GstAppSinkAdapterLog).noquote()
                << "Caps changed — allocator:" << allocName
                << "format:" << gst_video_format_to_string(GST_VIDEO_INFO_FORMAT(&parsedInfo))
                << GST_VIDEO_INFO_WIDTH(&parsedInfo) << "x" << GST_VIDEO_INFO_HEIGHT(&parsedInfo);
        }

        // QVideoFrameFormat has no setPixelAspectRatio; GPU branch can't normalize non-1/1 PAR.
        const int parN = GST_VIDEO_INFO_PAR_N(&parsedInfo);
        const int parD = GST_VIDEO_INFO_PAR_D(&parsedInfo);
        if (parN > 0 && parD > 0 && parN != parD) {
#if defined(QGC_HAS_ANY_GPU_PATH)
            if (self->_gpuPathEnabled) {
                qCWarning(GstAppSinkAdapterLog).noquote()
                    << "Source has non-square PAR" << parN << "/" << parD
                    << "— GPU zero-copy renders distorted (no Qt API to compensate)."
                    << "Disable GPU zero-copy on this source for correct geometry.";
            }
#endif
        }

        gst_caps_ref(caps);
        {
            QMutexLocker locker(&self->_stateMutex);
            if (self->_cachedCapsKey) {
                gst_caps_unref(self->_cachedCapsKey);
            }
            self->_cachedCapsKey = caps;
            self->_cachedInfo = parsedInfo;
            self->_cachedFormat = parsedFormat;
            self->_cachedPixelFormat = pixelFormat;
            self->_cachedAllocatorName = allocName;
        }

        localInfo = parsedInfo;
        localFormat = parsedFormat;
        localPixelFormat = pixelFormat;
    } else {
        // v4l2h264dec can silently upgrade allocator (sysmem→DMABuf) without re-negotiation.
        if (GstMemory *mem0 = gst_buffer_peek_memory(buffer, 0)) {
            const QString memType = QString::fromUtf8(
                (mem0->allocator) ? mem0->allocator->mem_type : "(none)");
            QMutexLocker locker(&self->_stateMutex);
            if (memType != self->_cachedAllocatorName) {
                qCInfo(GstAppSinkAdapterLog).noquote()
                    << "Allocator changed mid-stream:" << self->_cachedAllocatorName
                    << "→" << memType;
                self->_cachedAllocatorName = memType;
            }
        }
    }

    const GstVideoInfo &videoInfo = localInfo;

    // PTS monotonicity guard: a regressed timestamp wedges QVideoOutput's internal advance.
    if (GST_BUFFER_PTS_IS_VALID(buffer)) {
        if (GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DISCONT)) {
            self->_lastDeliveredPtsNs.store(-1, std::memory_order_release);
        }
        const int64_t pts = static_cast<int64_t>(GST_BUFFER_PTS(buffer));
        const int64_t lastPts = self->_lastDeliveredPtsNs.load(std::memory_order_acquire);
        if (lastPts >= 0 && pts < lastPts) {
            const int fpsN = GST_VIDEO_INFO_FPS_N(&videoInfo);
            const int fpsD = GST_VIDEO_INFO_FPS_D(&videoInfo);
            const int64_t framePeriodNs = (fpsN > 0 && fpsD > 0)
                ? static_cast<int64_t>(GST_SECOND) * fpsD / fpsN
                : 33000000;
            if (lastPts - pts > framePeriodNs) {
                static std::atomic<quint64> s_ptsRegressionDrops{0};
                const quint64 c = s_ptsRegressionDrops.fetch_add(1, std::memory_order_relaxed) + 1;
                if ((c & 0x3F) == 1) {
                    qCWarning(GstAppSinkAdapterLog)
                        << "PTS regression — dropping buffer (pts=" << pts
                        << "last=" << lastPts << "delta=" << (lastPts - pts) << "ns; total drops=" << c << ")";
                }
                gst_sample_unref(sample);
                return GST_FLOW_OK;
            }
        }
        self->_lastDeliveredPtsNs.store(pts, std::memory_order_release);
    }

#if defined(QGC_HAS_ANY_GPU_PATH)
    {
        HwVideoBufferPath matchedPath = HwVideoBufferPath::None;
        auto hwBuf = makeHwVideoBuffer(
            sample, videoInfo, applyCropMeta(localFormat, buffer),
            self->_gpuPathEnabled,
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
            self->_eglDisplay,
#else
            nullptr,
#endif
#if defined(QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH)
            self->_ahwbEglDisplay,
#else
            nullptr,
#endif
            matchedPath);
        if (hwBuf) {
            QVideoFrame gpuFrame(std::move(hwBuf));
            applyOrientationAndTiming(gpuFrame, buffer,
                self->_streamOrientation.load(std::memory_order_acquire));
            gst_sample_unref(sample);
            switch (matchedPath) {
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
            case HwVideoBufferPath::DmaBuf: {
                const quint64 c = self->_gpuFrames.fetch_add(1, std::memory_order_relaxed) + 1;
                if ((c & 0xFF) == 0) self->_logFrameStats();
                break;
            }
#endif
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
            case HwVideoBufferPath::GlMemory: {
                const quint64 c = self->_glFrames.fetch_add(1, std::memory_order_relaxed) + 1;
                if ((c & 0xFF) == 0) self->_logFrameStats();
                break;
            }
#endif
#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
            case HwVideoBufferPath::D3D11: {
                const quint64 c = self->_d3d11Frames.fetch_add(1, std::memory_order_relaxed) + 1;
                if ((c & 0xFF) == 0) self->_logFrameStats();
                break;
            }
#endif
#if defined(QGC_HAS_GST_D3D12_GPU_PATH)
            case HwVideoBufferPath::D3D12: {
                const quint64 c = self->_d3d12Frames.fetch_add(1, std::memory_order_relaxed) + 1;
                if ((c & 0xFF) == 0) self->_logFrameStats();
                break;
            }
#endif
#if defined(QGC_HAS_GST_IOSURFACE_GPU_PATH)
            case HwVideoBufferPath::IOSurface: {
                const quint64 c = self->_iosurfaceFrames.fetch_add(1, std::memory_order_relaxed) + 1;
                if ((c & 0xFF) == 0) self->_logFrameStats();
                break;
            }
#endif
#if defined(QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH)
            case HwVideoBufferPath::AHardwareBuffer: {
                const quint64 c = self->_ahwbFrames.fetch_add(1, std::memory_order_relaxed) + 1;
                if ((c & 0xFF) == 0) self->_logFrameStats();
                break;
            }
#endif
            default: break;
            }
            const int64_t ptsNs = GST_BUFFER_PTS_IS_VALID(buffer)
                ? static_cast<int64_t>(GST_BUFFER_PTS(buffer)) : -1;
            self->_deliverFrame(sinkSnapshot, std::move(gpuFrame), ptsNs);
            self->_pushQosUpstream(appsink, buffer);
            return GST_FLOW_OK;
        }
    }
#endif

    GstVideoFrame gstFrame;
    // gst_video_frame_map honors GstVideoMeta strides; bypass would require manual offset handling.
    if (!gst_video_frame_map(&gstFrame, const_cast<GstVideoInfo *>(&videoInfo), buffer, GST_MAP_READ)) {
        // PTS is the most useful clue for diagnosing hardware-fence / timeout failures.
        static std::atomic<quint64> s_failCount{0};
        const quint64 count = s_failCount.fetch_add(1, std::memory_order_relaxed) + 1;
        if ((count & 0x3F) == 1) {
            qCWarning(GstAppSinkAdapterLog) << "gst_video_frame_map failed; pts="
                << (GST_BUFFER_PTS_IS_VALID(buffer) ? GST_BUFFER_PTS(buffer) : 0)
                << "consecutive=" << count;
        }
        gst_sample_unref(sample);
        return GST_FLOW_ERROR;
    }

    QVideoFrame videoFrame(applyCropMeta(localFormat, buffer));

    if (!videoFrame.map(QVideoFrame::WriteOnly)) {
        qCWarning(GstAppSinkAdapterLog) << "Failed to map QVideoFrame for writing";
        gst_video_frame_unmap(&gstFrame);
        gst_sample_unref(sample);
        return GST_FLOW_ERROR;
    }

    const int planes = GST_VIDEO_INFO_N_PLANES(&videoInfo);
    for (int p = 0; p < planes; ++p) {
        const int dstStride = videoFrame.bytesPerLine(p);
        const int compWidth = GST_VIDEO_FRAME_COMP_WIDTH(&gstFrame, p);
        const int compPstride = GST_VIDEO_FRAME_COMP_PSTRIDE(&gstFrame, p);
        const int srcStride = GST_VIDEO_FRAME_PLANE_STRIDE(&gstFrame, p);
        const int planeHeight = GST_VIDEO_FRAME_COMP_HEIGHT(&gstFrame, p);
        const int activeRowBytes = compWidth * compPstride;
        const uchar *src = static_cast<const uchar *>(GST_VIDEO_FRAME_PLANE_DATA(&gstFrame, p));
        uchar *dst = videoFrame.bits(p);
        if (!dst) {
            continue;
        }
        if (activeRowBytes > dstStride) {
            // Qt allocated less than the active pixel width — should be impossible.
            static bool s_warnedStrideOverflow = false;
            if (!s_warnedStrideOverflow) {
                s_warnedStrideOverflow = true;
                qCWarning(GstAppSinkAdapterLog)
                    << "Plane" << p << ": activeRowBytes" << activeRowBytes
                    << "> dstStride" << dstStride << "— skipping frame";
            }
            videoFrame.unmap();
            gst_video_frame_unmap(&gstFrame);
            gst_sample_unref(sample);
            return GST_FLOW_ERROR;
        }
        if (srcStride == dstStride && activeRowBytes == srcStride) {
            memcpy(dst, src, static_cast<size_t>(planeHeight) * srcStride);
        } else {
            for (int y = 0; y < planeHeight; ++y) {
                memcpy(dst + y * dstStride, src + y * srcStride, activeRowBytes);
            }
        }
    }

    videoFrame.unmap();
    gst_video_frame_unmap(&gstFrame);

    applyOrientationAndTiming(videoFrame, buffer,
        self->_streamOrientation.load(std::memory_order_acquire));
    const quint64 c = self->_cpuFrames.fetch_add(1, std::memory_order_relaxed) + 1;
    if ((c & 0xFF) == 0) self->_logFrameStats();
    const int64_t ptsNs = GST_BUFFER_PTS_IS_VALID(buffer)
        ? static_cast<int64_t>(GST_BUFFER_PTS(buffer)) : -1;
    gst_sample_unref(sample);
    self->_deliverFrame(sinkSnapshot, std::move(videoFrame), ptsNs);
    self->_pushQosUpstream(appsink, buffer);
    return GST_FLOW_OK;
}

void GstAppSinkAdapter::_refreshLatency()
{
    if (!_appsink) return;
    GstQuery *q = gst_query_new_latency();
    if (!q) return;
    // Query on the appsink element propagates upstream through the live pipeline graph.
    if (gst_element_query(_appsink, q)) {
        gboolean live = FALSE;
        GstClockTime minLat = 0, maxLat = 0;
        gst_query_parse_latency(q, &live, &minLat, &maxLat);
        _pipelineMinLatencyNs = (minLat != GST_CLOCK_TIME_NONE) ? minLat : 0;
        _latencyValid = true;
        qCDebug(GstAppSinkAdapterLog) << "Pipeline latency refreshed: live=" << live
            << "min=" << _pipelineMinLatencyNs << "ns max=" << maxLat << "ns";
    }
    // On failure, _latencyValid stays false; _pushQosUpstream retries every frame.
    gst_query_unref(q);
}

void GstAppSinkAdapter::_pushQosUpstream(GstAppSink * /*appsink*/, GstBuffer *buffer)
{
    if (!_qosUpstreamEnabled) return;
    if (!GST_BUFFER_PTS_IS_VALID(buffer)) return;

    // Retry every frame until valid; then every N frames or when bus thread pokes the flag.
    if (!_latencyValid || _latencyRefreshPending.exchange(false, std::memory_order_relaxed)
            || (_qosSampleCount > 0 && (_qosSampleCount % kLatencyRefreshInterval) == 0)) {
        _refreshLatency();
    }

    const GstClockTime pts = GST_BUFFER_PTS(buffer);

    // Measure wall-clock arrival interval using steady_clock; PTS spacing gives expected interval.
    const GstClockTime nowNs = static_cast<GstClockTime>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());

    ++_qosSampleCount;

    if (_qosLastPts == GST_CLOCK_TIME_NONE || _qosLastArrivalNs == 0
            || pts <= _qosLastPts) {
        _qosLastPts = pts;
        _qosLastArrivalNs = nowNs;
        return;
    }

    const GstClockTime ptsDelta      = pts - _qosLastPts;
    const GstClockTime arrivalDelta  = nowNs - _qosLastArrivalNs;

    _qosLastPts        = pts;
    _qosLastArrivalNs  = nowNs;

    if (ptsDelta == 0) return;

    // rate = arrivalDelta / ptsDelta; >1 means we're consuming slower than the stream.
    const double rate = static_cast<double>(arrivalDelta) / static_cast<double>(ptsDelta);

    // EWMA: positive deviations averaged over window=16, negative over window=4 —
    // mirrors gstbasesink.c UPDATE_RUNNING_AVG_P/N so slow-path feedback stabilises.
    if (rate > _qosAvgRate) {
        _qosAvgRate = (rate + 15.0 * _qosAvgRate) / 16.0; // UPDATE_RUNNING_AVG_P
    } else {
        _qosAvgRate = (rate +  3.0 * _qosAvgRate) /  4.0; // UPDATE_RUNNING_AVG_N
    }

    // Skip warmup samples and only emit every kQosInterval frames to avoid event spam.
    if (_qosSampleCount < kQosWarmup) return;
    if ((_qosSampleCount % kQosInterval) != 0) return;

    // High-latency pipelines (e.g. RTSP jitter buffer) look "slow" by _pipelineMinLatencyNs;
    // if absolute lateness is within the pipeline's own minimum latency, treat as steady-state.
    if (_pipelineMinLatencyNs > 0) {
        // GST_CLOCK_DIFF avoids UB on guint64 subtraction; returns signed GstClockTimeDiff.
        const GstClockTimeDiff absLateness = GST_CLOCK_DIFF(ptsDelta, arrivalDelta);
        if (absLateness < static_cast<GstClockTimeDiff>(_pipelineMinLatencyNs)) {
            return;
        }
    }

    // proportion is clamped to [0.0, 16.0] per gstbasesink convention.
    const gdouble proportion = qBound(0.0, _qosAvgRate, 16.0);

    // diff=0: we don't sync to the pipeline clock so pipeline-clock lateness is unavailable.
    const GstClockTimeDiff diff = 0;

    GstQOSType type;
    if (proportion >= 16.0) {
        type = GST_QOS_TYPE_THROTTLE;   // catastrophically behind
    } else if (proportion > 1.0) {
        type = GST_QOS_TYPE_UNDERFLOW;  // we're behind; upstream should slow its production rate
    } else {
        type = GST_QOS_TYPE_OVERFLOW;   // we have headroom; upstream is producing too fast
    }

    GstEvent *event = gst_event_new_qos(type, proportion, diff, pts);
    if (event) {
        if (!_appsinkProbePad) {
            gst_event_unref(event);
            return;
        }
        // _appsinkProbePad is ref-held at setup(); reuse avoids a per-frame get/unref pair.
        const bool pushed = gst_pad_push_event(_appsinkProbePad, event);
        if (!pushed) {
            static bool s_qosPushFailed = false;
            if (!s_qosPushFailed) {
                s_qosPushFailed = true;
                qCDebug(GstAppSinkAdapterLog) << "QoS upstream push failed (silenced after first)";
            }
        }
    }
}

quint64 GstAppSinkAdapter::gpuFrameCount() const noexcept
{
    QMutexLocker locker(&_stateMutex); // snapshot read; torn-write-safe on supported arches
    quint64 count = 0;
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    count += _gpuFrames.load(std::memory_order_relaxed);
#endif
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
    count += _glFrames.load(std::memory_order_relaxed);
#endif
#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
    count += _d3d11Frames.load(std::memory_order_relaxed);
#endif
#if defined(QGC_HAS_GST_D3D12_GPU_PATH)
    count += _d3d12Frames.load(std::memory_order_relaxed);
#endif
#if defined(QGC_HAS_GST_IOSURFACE_GPU_PATH)
    count += _iosurfaceFrames.load(std::memory_order_relaxed);
#endif
#if defined(QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH)
    count += _ahwbFrames.load(std::memory_order_relaxed);
#endif
    return count;
}

quint64 GstAppSinkAdapter::cpuFrameCount() const noexcept
{
    QMutexLocker locker(&_stateMutex); // snapshot read; torn-write-safe on supported arches
    return _cpuFrames.load(std::memory_order_relaxed);
}

quint64 GstAppSinkAdapter::gpuFallbackCount() const noexcept
{
    QMutexLocker locker(&_stateMutex); // snapshot read; torn-write-safe on supported arches
#if defined(QGC_HAS_ANY_GPU_PATH)
    return _gpuPathEnabled ? _cpuFrames.load(std::memory_order_relaxed) : quint64(0);
#else
    return 0;
#endif
}

quint64 GstAppSinkAdapter::_deliveredFrames() const noexcept
{
    quint64 count = _cpuFrames.load(std::memory_order_relaxed);
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    count += _gpuFrames.load(std::memory_order_relaxed);
#endif
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
    count += _glFrames.load(std::memory_order_relaxed);
#endif
#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
    count += _d3d11Frames.load(std::memory_order_relaxed);
#endif
#if defined(QGC_HAS_GST_D3D12_GPU_PATH)
    count += _d3d12Frames.load(std::memory_order_relaxed);
#endif
#if defined(QGC_HAS_GST_IOSURFACE_GPU_PATH)
    count += _iosurfaceFrames.load(std::memory_order_relaxed);
#endif
#if defined(QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH)
    count += _ahwbFrames.load(std::memory_order_relaxed);
#endif
    return count;
}

quint64 GstAppSinkAdapter::appsinkInputFrames() const noexcept
{
    return _appsinkInputFrames.load(std::memory_order_relaxed);
}

quint64 GstAppSinkAdapter::appsinkDroppedFrames() const noexcept
{
    const quint64 in = _appsinkInputFrames.load(std::memory_order_relaxed);
    const quint64 out = _deliveredFrames();
    // The probe runs synchronously on the streaming thread before the buffer is enqueued;
    // delivered counters lag by at most the in-flight callback. Underflow is therefore possible
    // for a single-buffer race window — clamp to zero so QML never sees a wraparound value.
    return in > out ? in - out : 0;
}

GstPadProbeReturn GstAppSinkAdapter::appsinkBufferProbe(GstPad *, GstPadProbeInfo *info, gpointer userData)
{
    auto *self = static_cast<GstAppSinkAdapter *>(userData);
    if (!self) return GST_PAD_PROBE_OK;
    const auto type = GST_PAD_PROBE_INFO_TYPE(info);
    // FLUSH_START arrives with only EVENT_FLUSH set (non-serialized); FLUSH_STOP is serialized
    // and arrives with EVENT_DOWNSTREAM set. Accept either to catch both halves of the flush.
    if (type & (GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM | GST_PAD_PROBE_TYPE_EVENT_FLUSH)) {
        // FLUSH_START is non-serialized: arrives even when the streaming thread is mid-callback.
        // Set the flag before letting the event through so a parallel new_sample sees it.
        if (GstEvent *event = GST_PAD_PROBE_INFO_EVENT(info)) {
            switch (GST_EVENT_TYPE(event)) {
            case GST_EVENT_FLUSH_START:
                self->_flushing.store(true, std::memory_order_release);
                break;
            case GST_EVENT_FLUSH_STOP:
                self->_flushing.store(false, std::memory_order_release);
                // New source may not re-emit orientation tag; reset to identity.
                self->_streamOrientation.store(static_cast<int>(GST_VIDEO_ORIENTATION_IDENTITY),
                                               std::memory_order_release);
                self->_lastDeliveredPtsNs.store(-1, std::memory_order_release);
                // Smoothing-ring PTS belongs to the old timeline.
                if (self->_smoothingEnabled.load(std::memory_order_acquire)) {
                    QMutexLocker lock(&self->_smoothingMutex);
                    self->_smoothingRing.clear();
                    self->_smoothingFirstPtsNs = -1;
                }
                break;
            case GST_EVENT_TAG: {
                GstTagList *taglist = nullptr;
                gst_event_parse_tag(event, &taglist);
                GstVideoOrientationMethod method = GST_VIDEO_ORIENTATION_IDENTITY;
                if (taglist && gst_video_orientation_from_tag(taglist, &method)) {
                    self->_streamOrientation.store(static_cast<int>(method),
                                                   std::memory_order_release);
                }
                break;
            }
            default:
                break;
            }
        }
        return GST_PAD_PROBE_OK;
    }
    if (type & GST_PAD_PROBE_TYPE_BUFFER) {
        self->_appsinkInputFrames.fetch_add(1, std::memory_order_relaxed);
    }
    return GST_PAD_PROBE_OK;
}
