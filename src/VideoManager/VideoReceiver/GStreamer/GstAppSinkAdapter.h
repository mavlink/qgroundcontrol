#pragma once

#include <QtCore/QObject>
#include <QtCore/QElapsedTimer>
#include <QtCore/QList>
#include <QtCore/QMutex>
#include <QtCore/QPointer>
#include <QtCore/QTimer>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoFrameFormat>

#include <atomic>
#include <cstdint>

#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/video/video-info.h>

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH) || defined(QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH)
#include <EGL/egl.h>
#endif

class QVideoSink;

// File-scope helpers (kept out of the anonymous namespace so unit tests can call them directly).
QVideoFrameFormat::ColorSpace    toQtColorSpace   (GstVideoColorMatrix matrix);
QVideoFrameFormat::ColorTransfer toQtColorTransfer(GstVideoTransferFunction transfer);
QVideoFrameFormat::PixelFormat   toQtPixelFormat  (GstVideoFormat fmt);
QVideoFrameFormat                applyCropMeta    (QVideoFrameFormat format, GstBuffer *buffer);
// Umbrella header — pulls in GstVideoOrientationMethod (always present) plus the
// per-buffer meta accessor (gated below by QGC_HAS_GST_VIDEO_ORIENTATION_META).
// The standalone <gst/video/video-orientation.h> isn't shipped in every gst-video
// install; the umbrella is the portable spelling.
#include <gst/video/video.h>
void                             applyOrientationToFrame(QVideoFrame &frame, GstVideoOrientationMethod method);

/// \brief Bridges a GStreamer appsink to a Qt QVideoSink.
///
/// Each decoded frame arriving at the appsink is copied into a QVideoFrame
/// and pushed to the QVideoSink, which renders through Qt's native RHI
/// backend (Metal on macOS, Vulkan/D3D elsewhere).
///
class GstAppSinkAdapter : public QObject
{
    Q_OBJECT

    Q_PROPERTY(quint64 gpuFrameCount READ gpuFrameCount NOTIFY frameCountsChanged)
    Q_PROPERTY(quint64 cpuFrameCount READ cpuFrameCount NOTIFY frameCountsChanged)
    Q_PROPERTY(quint64 gpuFallbackCount READ gpuFallbackCount NOTIFY frameCountsChanged)
    /// Frames that reached the appsink sink pad (counted via pad probe).
    Q_PROPERTY(quint64 appsinkInputFrames READ appsinkInputFrames NOTIFY frameCountsChanged)
    /// Frames the appsink dropped because of `max-buffers=1, drop=true` queue overflow.
    /// Distinct from decoder-level QoS drops surfaced by GstVideoReceiver.
    Q_PROPERTY(quint64 appsinkDroppedFrames READ appsinkDroppedFrames NOTIFY frameCountsChanged)

public:
    explicit GstAppSinkAdapter(QObject *parent = nullptr);
    ~GstAppSinkAdapter() override;

    /// Connect to the named appsink inside @p sinkBin and push frames to @p videoSink.
    /// Returns true on success.
    bool setup(GstElement *sinkBin, QVideoSink *videoSink);

    /// Disconnect the callback (safe to call multiple times).
    void teardown();

    /// Signal the adapter to refresh pipeline latency on the next streaming-thread tick.
    void requestLatencyRefresh() noexcept { _latencyRefreshPending.store(true, std::memory_order_relaxed); }

    /// Gate frame delivery without tearing the pipeline down. When set to false the next
    /// frame seen is a single empty QVideoFrame (clears the sink), and subsequent samples
    /// are dropped at the appsink callback until re-enabled. Mirrors Qt's
    /// QVideoSink::isActive gate (qgstvideorenderersink.cpp:345-355). Use when switching
    /// streams to avoid the previous stream's last frame ghosting on the QML output.
    void setActive(bool active);

    /// Display refresh-rate baseline for appsink max-time (max(1/refreshHz, 1/streamFps)
    /// applied at each caps change). Call once from the GUI thread after setup(); pass 0
    /// to leave the bin's 33 ms default.
    void setRefreshRate(qreal hz);

    /// Opt-in OBS-style smoothing: 3-deep ring, display-rate timer picks the frame
    /// nearest the PTS-anchored expected presentation time. Adds up to one frame of
    /// latency. refreshHz <= 0 falls back to 60 Hz tick.
    void setSmoothingEnabled(bool enabled, qreal refreshHz);

    quint64 gpuFrameCount() const noexcept;
    quint64 cpuFrameCount() const noexcept;
    quint64 gpuFallbackCount() const noexcept;
    quint64 appsinkInputFrames() const noexcept;
    quint64 appsinkDroppedFrames() const noexcept;

signals:
    void frameCountsChanged();

private:
    static GstFlowReturn onNewSample(GstAppSink *appsink, gpointer userData);
    /// Sink-pad probe on the appsink that increments _appsinkInputFrames per buffer.
    /// Combined with delivered counters this exposes appsink drop pressure separately
    /// from decoder QoS drops.
    static GstPadProbeReturn appsinkBufferProbe(GstPad *pad, GstPadProbeInfo *info, gpointer userData);

    void _logFrameStats() const;
    /// Sum of every "delivered" counter (cpu + every enabled GPU path).
    quint64 _deliveredFrames() const noexcept;

    /// Push a GST_EVENT_QOS upstream from the streaming thread to throttle the decoder.
    void _pushQosUpstream(GstAppSink *appsink, GstBuffer *buffer);
    /// Re-query pipeline latency via the appsink element; called periodically on the streaming thread.
    void _refreshLatency();

    /// Push immediately (smoothing off) or enqueue into the ring (smoothing on).
    /// ptsNs = -1 when buffer PTS is unset.
    void _deliverFrame(QPointer<QVideoSink> sink, QVideoFrame &&frame, int64_t ptsNs);

    /// GUI-thread tick — picks the ring entry nearest the anchored target PTS.
    void _onSmoothingTick();

    QTimer _telemetryEmitTimer;

    // Never held while pushing a frame to QVideoSink (avoids priority inversion with the render thread).
    mutable QMutex _stateMutex;

    QPointer<QVideoSink> _videoSink;
    GstElement *_appsink = nullptr;
    // Ref-held: the probe is installed on the appsink's sink pad; we keep the pad alive so
    // teardown can target it for removal even if _appsink ownership changes.
    GstPad *_appsinkProbePad = nullptr;
    gulong _appsinkProbeId = 0;
    std::atomic<quint64> _appsinkInputFrames{0};
    // Set by GST_EVENT_FLUSH_START on the appsink sink pad, cleared by FLUSH_STOP.
    // Drops new_sample callbacks while flushing so a buffer queued before a pipeline reset
    // can't surface as a stale frame after FLUSH_STOP. Read on the streaming thread, written
    // on the upstream serializer thread — both ordered by GStreamer's event semantics.
    std::atomic<bool> _flushing{false};
    // setActive(false) drops samples at onNewSample so a torn-down stream's last frame
    // can't ghost when the user switches sources. Default true for backwards compatibility.
    std::atomic<bool> _active{true};
    // Stream-level orientation from upstream GST_TAG_IMAGE_ORIENTATION. Per-buffer
    // GstVideoOrientationMeta still wins when present (gated by QGC_HAS_GST_VIDEO_ORIENTATION_META);
    // this fallback works on any gst-video install since gst_video_orientation_from_tag is
    // in the umbrella header.
    std::atomic<int> _streamOrientation{static_cast<int>(GST_VIDEO_ORIENTATION_IDENTITY)};

    // 0 = use bin's 33 ms default. Streaming thread reads on caps change.
    std::atomic<quint64> _refreshPeriodNs{0};

    // Smoothing ring: producer (streaming) writes under _smoothingMutex, timer (owner thread) reads.
    // _smoothingClock is started before _smoothingEnabled flips so producer always sees started clock.
    static constexpr int kSmoothingRingCapacity = 3;
    static constexpr int64_t kSmoothingThresholdNs = 70 * 1000000;
    struct SmoothingEntry {
        QVideoFrame frame;
        int64_t ptsNs;       // stream PTS (or fallback wall-time when PTS missing)
        qint64  enqueuedNs;  // QElapsedTimer::nsecsElapsed at enqueue
    };
    std::atomic<bool> _smoothingEnabled{false};
    mutable QMutex _smoothingMutex;
    QList<SmoothingEntry> _smoothingRing;
    QTimer _smoothingTickTimer;
    QElapsedTimer _smoothingClock;
    int64_t _smoothingFirstPtsNs = -1;
    qint64  _smoothingFirstClockNs = 0;
    std::atomic<quint64> _smoothingDroppedFrames{0};
    // -1 = no prior; reset by DISCONT and FLUSH_STOP. Without this a seek wedges QVideoOutput.
    std::atomic<int64_t> _lastDeliveredPtsNs{-1};

    // Held ref prevents GstCaps address reuse that would produce a false identity-cache hit with stale colorimetry.
    GstCaps *_cachedCapsKey = nullptr;
    GstVideoInfo _cachedInfo{};
    QVideoFrameFormat _cachedFormat;
    int _cachedPixelFormat = 0; // QVideoFrameFormat::Format_Invalid sentinel
    QString _cachedAllocatorName; // negotiated allocator from first sample of each caps change

    // Steady-clock ns checkpoint + matching total-frames snapshot for fps-over-window in _logFrameStats.
    // Both mutated from the streaming thread; readers (currently none) get a relaxed view.
    mutable std::atomic<qint64> _lastStatsAtNs{0};
    mutable std::atomic<quint64> _lastStatsTotal{0};

    // Tracks the last total-frames count observed by the emit timer so we can suppress
    // no-change frameCountsChanged() emissions (idle adapters waste ~60 emits/min otherwise).
    quint64 _lastEmittedFrameTotal = 0;

    // Format warning throttle: only warn on first occurrence of each unsupported format.
    std::atomic<GstVideoFormat> _lastWarnedFormat{GST_VIDEO_FORMAT_UNKNOWN};

    // Counters are written from the GStreamer streaming thread and read from the GUI thread
    // (Q_PROPERTY getters + telemetry timer). Atomics keep the read/write race TSan-clean.
    std::atomic<quint64> _cpuFrames{0};

#if defined(QGC_HAS_ANY_GPU_PATH)
    bool _gpuPathEnabled = false;
#endif
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    // Set at setup() to a Wayland/X11 EGL display when zero-copy is enabled and
    // resolvable. EGL_NO_DISPLAY disables the GPU path for this adapter instance.
    EGLDisplay _eglDisplay = EGL_NO_DISPLAY;
    std::atomic<quint64> _gpuFrames{0};       // DMABuf zero-copy frames delivered
#endif
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
    std::atomic<quint64> _glFrames{0};        // GLMemory zero-copy frames delivered
#endif
#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
    std::atomic<quint64> _d3d11Frames{0};     // D3D11Memory zero-copy frames delivered
#endif
#if defined(QGC_HAS_GST_D3D12_GPU_PATH)
    std::atomic<quint64> _d3d12Frames{0};     // D3D12Memory zero-copy frames delivered
#endif
#if defined(QGC_HAS_GST_IOSURFACE_GPU_PATH)
    std::atomic<quint64> _iosurfaceFrames{0}; // IOSurface/CVPixelBuffer zero-copy frames delivered
#endif
#if defined(QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH)
    EGLDisplay _ahwbEglDisplay = EGL_NO_DISPLAY;
    std::atomic<quint64> _ahwbFrames{0};      // AHardwareBuffer zero-copy frames delivered
#endif

    // QoS upstream feedback — EWMA mirrors gstbasesink.c DO_RUNNING_AVG window constants.
    bool _qosUpstreamEnabled = true;
    // Number of samples delivered so far; skip first kQosWarmup before sending events.
    quint64 _qosSampleCount = 0;
    // EWMA of actual interval / expected interval; >1.0 means we're consuming slower than source.
    double _qosAvgRate = 1.0;
    GstClockTime _qosLastPts = GST_CLOCK_TIME_NONE;
    GstClockTime _qosLastArrivalNs = 0;
    static constexpr quint64 kQosWarmup = 10;
    static constexpr int kQosInterval = 8; // emit every N-th frame to avoid event spam
    // Latency fields — all written and read exclusively on the streaming thread;
    // teardown() resets them only after the pipeline is NULL (no concurrent streaming).
    GstClockTime _pipelineMinLatencyNs = 0;
    bool _latencyValid = false;           // false until first successful GST_QUERY_LATENCY
    std::atomic<bool> _latencyRefreshPending{false}; // set by requestLatencyRefresh() from any thread
    static constexpr quint64 kLatencyRefreshInterval = 256; // re-query every N frames
};
