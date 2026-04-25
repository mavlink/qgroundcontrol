#pragma once

#include <QtCore/QAtomicInteger>
#include <QtCore/QAtomicPointer>
#include <QtCore/QElapsedTimer>
#include <QtCore/QFuture>
#include <QtCore/QLoggingCategory>
#include <QtCore/QMetaObject>
#include <QtCore/QMutex>
#include <QtCore/QObject>
#include <QtCore/QSize>
#include <QtCore/QTimer>
#include <QtGui/QImage>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoFrameFormat>
#include <atomic>
#include <chrono>

Q_DECLARE_LOGGING_CATEGORY(VideoFrameDeliveryLog)

class QVideoSink;

/// Thread-safe frame delivery endpoint between video receivers and QVideoSink.
///
/// Display receivers use forwardFrameToSink() when this endpoint should push
/// frames to the registered QVideoSink, and observeSinkFrame() when Qt has
/// already pushed frames directly to that sink. Both paths update recording
/// snapshots, stats, watchdog state, and video size.
///
/// Threading model:
///   * The delivery endpoint is a plain QObject that lives on its parent's thread -
///     typically the main thread, since it is parented to the stream session. No
///     dedicated worker thread: mutator funnels dispatch via invokeMethod
///     onto the delivery thread, so callers on that thread execute
///     synchronously and off-thread callers post a queued event.
///   * Mutators (setVideoSink, announceFormat, resetStats, armWatchdog)
///     dispatch onto the delivery thread via QMetaObject::invokeMethod when
///     called from another thread, so per-mutator ad-hoc thread checks
///     collapse into a single funnel.
///   * forwardFrameToSink() remains callable from any thread and keeps its
///     mutex+atomic fast path - it must not add a queue hop on the hot path.
///   * Readers (videoSize, lastRawFrame, frameCount, droppedFrames, latencyMs,
///     videoSink, grabFrame) are thread-safe via atomics/seqlock/mutex.
///
/// Stats monitoring (FPS, latency, health) is handled by VideoStreamStats,
/// which observes this endpoint's signals.
class VideoFrameDelivery : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(VideoFrameDelivery)

    Q_PROPERTY(QString lastFrameHandleTypeName READ lastFrameHandleTypeName NOTIFY frameDiagnosticsChanged)
    Q_PROPERTY(QString lastFramePixelFormatName READ lastFramePixelFormatName NOTIFY frameDiagnosticsChanged)
    Q_PROPERTY(bool lastFrameUsesHardwareHandle READ lastFrameUsesHardwareHandle NOTIFY frameDiagnosticsChanged)

public:
    /// Parent the delivery endpoint to its owner. The endpoint shares the
    /// owner's thread affinity.
    explicit VideoFrameDelivery(QObject* parent = nullptr);
    ~VideoFrameDelivery() override;

    /// Set or replace the output sink. Pass nullptr to disconnect.
    /// Dispatches to the delivery thread if called from another thread.
    void setVideoSink(QVideoSink* sink);

    [[nodiscard]] QVideoSink* videoSink() const { return _sink.loadRelaxed(); }

    /// Thread-safe forwarding path - direct execution on the caller's thread
    /// (typically the streaming thread). Updates counters + seqlock + posts
    /// the frame to the sink's thread via invokeMethod.
    void forwardFrameToSink(QVideoFrame frame);

    /// Observe a frame that was already delivered directly to a QVideoSink by
    /// another producer, such as QMediaPlayer. Updates counters, cached frame,
    /// size, and recording/stat signals without forwarding the frame again.
    void observeSinkFrame(QVideoFrame frame);

    /// Announce the negotiated video format *before* the first buffer arrives.
    /// Dispatches to the delivery thread if called from another thread.
    ///
    /// Eliminates the first-frame aspect-ratio flash: without this, QVideoSink
    /// starts at videoSize=(0,0) and QML VideoOutput paints at its item aspect
    /// until the first real frame lands. Calling this on caps negotiation
    /// updates `_videoSize` and emits `videoSizeChanged` so VideoOutput can
    /// size its contentRect correctly from the start. The format is cached so
    /// a replacement sink attached mid-stream receives the same announcement.
    void announceFormat(const QVideoFrameFormat& format);

    /// Returns the last frame seen by either forwarding or observation.
    /// Protected by a seqlock: producer-side writers, multiple readers.
    /// Returns a default-constructed (invalid) frame if no frame has been seen.
    [[nodiscard]] QVideoFrame lastRawFrame() const;

    struct FrameSnapshot
    {
        QVideoFrame frame;
        quint64 sequence = 0;
    };

    /// Returns the last raw frame and its monotonically increasing sequence.
    /// Consumers that pull from the cached frame, such as FrameDeliveryRecorder, use
    /// this to avoid resubmitting the same frame on repeated ready signals.
    [[nodiscard]] FrameSnapshot lastRawFrameSnapshot() const;

    [[nodiscard]] QSize videoSize() const;

    [[nodiscard]] QString lastFrameHandleTypeName() const;
    [[nodiscard]] QString lastFramePixelFormatName() const;
    [[nodiscard]] bool lastFrameUsesHardwareHandle() const;

    [[nodiscard]] quint64 frameCount() const { return _frameCount.loadRelaxed(); }

    [[nodiscard]] bool firstFrameReady() const { return _firstFrameReady.load(std::memory_order_acquire); }

    [[nodiscard]] quint64 droppedFrames() const { return _droppedFrames.loadRelaxed(); }

    void noteDroppedFrame() { _droppedFrames.fetchAndAddRelaxed(1); }

    /// Record a latency sample supplied by a receiver.
    void noteLatencySample(float ms);
    [[nodiscard]] float latencyMs() const;

    /// Dispatches to the delivery thread if called from another thread.
    void resetStats();

    /// Grab the last delivered frame as a QImage (for snapshots).
    [[nodiscard]] QFuture<QImage> grabFrame() const;

    /// Arm the frame watchdog: if no frame is delivered for `timeout`, the
    /// endpoint emits `watchdogTimeout` once and disarms itself until a new
    /// frame re-primes it. Pass a zero duration (or call `disarmWatchdog`)
    /// to stop watching. Dispatches to the delivery thread if needed.
    ///
    /// The watchdog uses an atomic timestamp updated per frame plus a
    /// coarse periodic tick - no queue hop on the hot path.
    void armWatchdog(std::chrono::milliseconds timeout);
    void disarmWatchdog() { armWatchdog(std::chrono::milliseconds::zero()); }

signals:
    void videoSizeChanged(QSize size);
    /// Fires for each frame enqueued for the sink (after the backpressure gate).
    void frameArrived();
    /// Fires when the first decoded frame arrives, and when resetStats() clears
    /// the stream for a fresh decode session.
    void firstFrameReadyChanged(bool ready);
    void frameDiagnosticsChanged();
    /// Fires when the backpressure gate drops a frame.
    void frameDropped();
    /// Fires once when no frames have arrived for the armed timeout window.
    /// Disarms after firing - deliver a frame to re-prime.
    void watchdogTimeout();

private:
    /// True iff the caller is on the delivery endpoint's own thread - mutator funnels
    /// use this to pick direct-call vs queued-dispatch.
    [[nodiscard]] bool _onDeliveryThread() const;

    /// Runs on the delivery thread - all sink/connection mutation lives here.
    void _applyVideoSink(QVideoSink* sink);
    void _applyAnnounceFormat(QVideoFrameFormat format);
    void _applyResetStats();
    void _applyArmWatchdog(std::chrono::milliseconds timeout);
    void _onWatchdogTick();
    void _storeFrameSnapshot(QVideoFrame frame);
    void _updateFrameDiagnostics(const QVideoFrame& frame);

    QAtomicPointer<QVideoSink> _sink = nullptr;
    QSize _videoSize;
    /// Last format announced via announceFormat(). Replayed on sink attach so
    /// a late-bound QVideoSink gets AR info without waiting for the next frame.
    QVideoFrameFormat _announcedFormat;
    mutable QMutex _sizeMutex;
    mutable QMutex _frameDiagnosticsMutex;
    QString _lastFrameHandleTypeName;
    QString _lastFramePixelFormatName;
    bool _lastFrameUsesHardwareHandle = false;
    QAtomicInteger<quint64> _frameCount{0};
    QAtomicInteger<quint64> _droppedFrames{0};
    QAtomicInteger<int> _pendingDelivery{0};
    std::atomic<bool> _firstFrameReady{false};
    QMetaObject::Connection _sizeConn;
    QMetaObject::Connection _destroyedConn;

    // Frame watchdog.
    // _lastFrameMs is written by frame producers via relaxed store;
    // the tick handler on the delivery thread reads it and compares against
    // the configured timeout. Using a monotonic QElapsedTimer-derived stamp
    // avoids wall-clock jumps and is cheaper than restarting a QTimer per
    // frame at 60 fps.
    QElapsedTimer _watchdogClock;
    QTimer _watchdogTick;
    std::atomic<qint64> _lastFrameMs{-1};
    qint64 _watchdogTimeoutMs = 0;  ///< 0 = disarmed
    bool _watchdogFired = false;    ///< edge-trigger: cleared when a new frame arrives

    // Last raw frame - protected by a seqlock.
    // Writers: store odd seq, write frame, store even seq.
    // Readers (lastRawFrameSnapshot, grabFrame): load seq, copy frame, load seq again - retry if odd or changed.
    // QVideoFrame is implicitly shared (one pointer internally); copy is a refcount increment.
    mutable std::atomic<unsigned> _lastFrameSeq{0};
    QVideoFrame _lastRawFrame;

    // Latency measurement (EWMA of capture-to-display delta)
    std::atomic<float> _latencyMs{-1.0f};
};
