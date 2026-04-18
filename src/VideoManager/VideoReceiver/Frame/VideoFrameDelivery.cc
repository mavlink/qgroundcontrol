#include "VideoFrameDelivery.h"

#include <QtCore/QFuture>
#include <QtCore/QPointer>
#include <QtCore/QPromise>
#include <QtCore/QThread>
#include <QtMultimedia/QVideoSink>
#include <memory>
#include <utility>

// The delivery endpoint lives on its owner's thread (typically VideoStream on the main
// thread). `_onDeliveryThread()` returns true iff the caller is on that thread
// so mutator funnels can take a direct-call fast path.

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(VideoFrameDeliveryLog, "Video.VideoFrameDelivery")

VideoFrameDelivery::VideoFrameDelivery(QObject* parent) : QObject(parent)
{
    _watchdogClock.start();
    // Periodic tick cadence: 500 ms. Coarse timer type - fine for a multi-
    // second stall watchdog and keeps wake-ups cheap. Timer is a child QObject,
    // so it shares thread affinity with the delivery endpoint and gets reparented with it
    // on any future moveToThread.
    _watchdogTick.setParent(this);
    _watchdogTick.setInterval(500);
    _watchdogTick.setTimerType(Qt::CoarseTimer);
    connect(&_watchdogTick, &QTimer::timeout, this, &VideoFrameDelivery::_onWatchdogTick);
}

VideoFrameDelivery::~VideoFrameDelivery()
{
    // Detach the sink on the delivery thread so the final `setVideoFrame({})`
    // is queued before the endpoint stops receiving frames. When the dtor runs
    // on the delivery thread (normal parent-child destruction), the call is
    // direct; cross-thread deletion uses BlockingQueuedConnection to retain
    // the synchronous-detach ordering the old worker-thread design relied on.
    if (_sink.loadRelaxed()) {
        if (_onDeliveryThread()) {
            _applyVideoSink(nullptr);
        } else {
            QMetaObject::invokeMethod(
                this, [this]() { _applyVideoSink(nullptr); }, Qt::BlockingQueuedConnection);
        }
    }
}

bool VideoFrameDelivery::_onDeliveryThread() const
{
    return QThread::currentThread() == thread();
}

void VideoFrameDelivery::setVideoSink(QVideoSink* sink)
{
    if (_onDeliveryThread()) {
        _applyVideoSink(sink);
        return;
    }
    // BlockingQueuedConnection so `videoSink()` is observable on return -
    // callers (VideoStream::registerVideoSink, test code, deferred-sink flush
    // in _ensureReceiver) rely on synchronous visibility. In practice this is
    // rarely hit: the endpoint lives on the owner's thread (main), and mutators
    // are driven from the same thread.
    QMetaObject::invokeMethod(
        this, [this, sink]() { _applyVideoSink(sink); }, Qt::BlockingQueuedConnection);
}

void VideoFrameDelivery::_applyVideoSink(QVideoSink* sink)
{
    QVideoSink* oldSink = _sink.loadRelaxed();
    if (oldSink == sink)
        return;

    // Flush an empty frame so Qt's render thread drops stale GPU textures.
    if (oldSink) {
        if (QThread::currentThread() == oldSink->thread()) {
            oldSink->setVideoFrame(QVideoFrame());
        } else {
            QMetaObject::invokeMethod(
                oldSink, [oldSink]() { oldSink->setVideoFrame(QVideoFrame()); }, Qt::QueuedConnection);
        }
    }

    disconnect(_sizeConn);
    disconnect(_destroyedConn);
    _sink.storeRelaxed(nullptr);

    if (sink) {
        _sink.storeRelaxed(sink);
        _destroyedConn = connect(sink, &QObject::destroyed, this, [this](QObject*) { _sink.storeRelaxed(nullptr); });
        _sizeConn = connect(sink, &QVideoSink::videoSizeChanged, this, [this]() {
            QVideoSink* s = _sink.loadRelaxed();
            const QSize size = s ? s->videoSize() : QSize();
            {
                QMutexLocker lock(&_sizeMutex);
                if (size == _videoSize)
                    return;
                _videoSize = size;
            }
            emit videoSizeChanged(size);
        });

        // Replay cached format so a late-bound sink gets AR info without
        // waiting for the next frame to land. Snapshot under the size mutex
        // since announceFormat writes _announcedFormat under that lock.
        QVideoFrameFormat replay;
        {
            QMutexLocker lock(&_sizeMutex);
            replay = _announcedFormat;
        }
        if (replay.isValid()) {
            QVideoFrame hint(replay);
            if (QThread::currentThread() == sink->thread()) {
                sink->setVideoFrame(hint);
            } else {
                QMetaObject::invokeMethod(
                    sink, [sink, hint]() { sink->setVideoFrame(hint); }, Qt::QueuedConnection);
            }
        }
    }

    qCDebug(VideoFrameDeliveryLog) << "Video sink set:" << sink;
}

void VideoFrameDelivery::announceFormat(const QVideoFrameFormat& format)
{
    if (!format.isValid())
        return;
    if (_onDeliveryThread()) {
        _applyAnnounceFormat(format);
        return;
    }
    QMetaObject::invokeMethod(
        this, [this, format]() { _applyAnnounceFormat(format); }, Qt::QueuedConnection);
}

void VideoFrameDelivery::_applyAnnounceFormat(QVideoFrameFormat format)
{
    const QSize size = format.frameSize();
    bool sizeChanged = false;
    {
        QMutexLocker lock(&_sizeMutex);
        if (_announcedFormat == format && _videoSize == size)
            return;  // already announced this exact format
        _announcedFormat = format;
        if (_videoSize != size) {
            _videoSize = size;
            sizeChanged = true;
        }
    }

    if (sizeChanged)
        emit videoSizeChanged(size);

    // Push a format-only frame to the sink so QVideoSink::videoSize picks up
    // the real AR before the first decoded buffer arrives. The frame has no
    // pixel data; it exists purely to prime the sink's size reporting.
    QVideoSink* sink = _sink.loadAcquire();
    if (!sink)
        return;

    QVideoFrame hint(format);
    if (QThread::currentThread() == sink->thread()) {
        sink->setVideoFrame(hint);
    } else {
        QMetaObject::invokeMethod(
            sink, [sink, hint]() { sink->setVideoFrame(hint); }, Qt::QueuedConnection);
    }
}

void VideoFrameDelivery::deliverFrame(QVideoFrame frame)
{
    if (!frame.isValid())
        return;

    // Watchdog: stamp monotonic time of last valid frame (any thread - relaxed
    // store is sufficient since the tick reader only needs eventual visibility
    // and is rate-limited to 500 ms).
    _lastFrameMs.store(_watchdogClock.elapsed(), std::memory_order_relaxed);
    if (!_firstFrameReady.exchange(true, std::memory_order_acq_rel))
        emit firstFrameReadyChanged(true);

    // Seqlock write: mark odd (writing), store frame, mark even (done).
    // Single writer - no compare-exchange needed.
    unsigned seq = _lastFrameSeq.load(std::memory_order_relaxed);
    _lastFrameSeq.store(seq | 1u, std::memory_order_release);
    _lastRawFrame = frame;
    _lastFrameSeq.store(seq + 2u, std::memory_order_release);

    // Decide size change under the lock, emit outside - avoid re-entrancy deadlock.
    const QSize size = frame.size();
    bool sizeChanged = false;
    {
        QMutexLocker lock(&_sizeMutex);
        if (size != _videoSize) {
            _videoSize = size;
            sizeChanged = true;
        }
    }
    if (sizeChanged)
        emit videoSizeChanged(size);

    QVideoSink* sink = _sink.loadAcquire();
    if (!sink) {
        _frameCount.fetchAndAddRelaxed(1);
        emit frameArrived();
        return;
    }

    constexpr int kMaxInFlight = 3;
    const int prev = _pendingDelivery.fetchAndAddAcquire(1);
    if (prev >= kMaxInFlight) {
        _pendingDelivery.fetchAndSubRelease(1);
        // Only count drops - don't tick frameCount or emit frameArrived, or
        // FPS/drop-rate stats double-count.
        noteDroppedFrame();
        emit frameDropped();
        return;
    }

    _frameCount.fetchAndAddRelaxed(1);
    emit frameArrived();

    QPointer<VideoFrameDelivery> self(this);
    auto deliveryGuard = std::shared_ptr<QPointer<VideoFrameDelivery>>(
        new QPointer<VideoFrameDelivery>(self),
        [](QPointer<VideoFrameDelivery>* delivery) {
            if (*delivery)
                (*delivery)->_pendingDelivery.fetchAndSubRelease(1);
            delete delivery;
        });
    QMetaObject::invokeMethod(
        sink,
        [sink, guard = std::move(deliveryGuard), f = std::move(frame)]() {
            Q_UNUSED(guard)
            sink->setVideoFrame(f);
        },
        Qt::QueuedConnection);
}

QVideoFrame VideoFrameDelivery::lastRawFrame() const
{
    // Seqlock read: retry while seq is odd (writer active) or changes mid-copy.
    QVideoFrame result;
    unsigned seq;
    do {
        seq = _lastFrameSeq.load(std::memory_order_acquire);
        if (seq & 1u)
            continue;  // writer active - spin
        result = _lastRawFrame;  // QVideoFrame copy = refcount increment (cheap)
    } while (_lastFrameSeq.load(std::memory_order_acquire) != seq);
    return result;
}

QSize VideoFrameDelivery::videoSize() const
{
    QMutexLocker lock(&_sizeMutex);
    return _videoSize;
}

float VideoFrameDelivery::latencyMs() const
{
    return _latencyMs.load(std::memory_order_relaxed);
}

void VideoFrameDelivery::noteLatencySample(float ms)
{
    constexpr float kAlpha = 0.0625F;
    float prev = _latencyMs.load(std::memory_order_relaxed);
    float next = (prev < 0.0F) ? ms : (prev * (1.0F - kAlpha) + ms * kAlpha);
    _latencyMs.store(next, std::memory_order_relaxed);
}

void VideoFrameDelivery::resetStats()
{
    if (_onDeliveryThread()) {
        _applyResetStats();
        return;
    }
    QMetaObject::invokeMethod(
        this, [this]() { _applyResetStats(); }, Qt::QueuedConnection);
}

void VideoFrameDelivery::_applyResetStats()
{
    _frameCount.storeRelaxed(0);
    _droppedFrames.storeRelaxed(0);
    _pendingDelivery.storeRelaxed(0);
    _latencyMs.store(-1.0F, std::memory_order_relaxed);
    const bool wasReady = _firstFrameReady.exchange(false, std::memory_order_acq_rel);
    // Seqlock write to clear the last frame.
    unsigned seq = _lastFrameSeq.load(std::memory_order_relaxed);
    _lastFrameSeq.store(seq | 1u, std::memory_order_release);
    _lastRawFrame = QVideoFrame{};
    _lastFrameSeq.store(seq + 2u, std::memory_order_release);
    _lastFrameMs.store(-1, std::memory_order_relaxed);
    _watchdogFired = false;
    if (wasReady)
        emit firstFrameReadyChanged(false);
}

void VideoFrameDelivery::armWatchdog(std::chrono::milliseconds timeout)
{
    if (_onDeliveryThread()) {
        _applyArmWatchdog(timeout);
        return;
    }
    QMetaObject::invokeMethod(
        this, [this, timeout]() { _applyArmWatchdog(timeout); }, Qt::QueuedConnection);
}

void VideoFrameDelivery::_applyArmWatchdog(std::chrono::milliseconds timeout)
{
    _watchdogTimeoutMs = timeout.count();
    _watchdogFired = false;
    if (_watchdogTimeoutMs > 0) {
        // Leave `_lastFrameMs` alone. Production code calls resetStats()
        // (via validateBridgeForDecoding) before arming, so it's already -1 -
        // and `_onWatchdogTick` treats -1 as "no frame yet" and returns.
        // Previously this function primed `_lastFrameMs = now`, which fired
        // spurious timeouts on HW decoders (vah264dec/vah265dec/v4l2h264dec)
        // that spend 1-3 s on caps/DMA negotiation before the first sample.
        // Tests that exercise stall-after-activity (deliver frame -> arm ->
        // expect silence to fire) rely on us preserving the pre-arm stamp.
        if (!_watchdogTick.isActive())
            _watchdogTick.start();
    } else {
        _watchdogTick.stop();
    }
}

void VideoFrameDelivery::_onWatchdogTick()
{
    if (_watchdogTimeoutMs <= 0 || _watchdogFired)
        return;

    const qint64 last = _lastFrameMs.load(std::memory_order_relaxed);
    if (last < 0)
        return;  // never stamped - nothing to watch

    const qint64 elapsed = _watchdogClock.elapsed() - last;
    if (elapsed >= _watchdogTimeoutMs) {
        _watchdogFired = true;
        qCDebug(VideoFrameDeliveryLog) << "Watchdog: no frames for" << elapsed << "ms - emitting watchdogTimeout";
        emit watchdogTimeout();
    }
}

QFuture<QImage> VideoFrameDelivery::grabFrame() const
{
    // Try the last raw frame first via seqlock read - avoids a cross-thread round-trip.
    const QVideoFrame raw = lastRawFrame();
    if (raw.isValid())
        return QtFuture::makeReadyValueFuture(raw.toImage());

    QVideoSink* sink = _sink.loadRelaxed();
    if (!sink)
        return QtFuture::makeReadyValueFuture(QImage{});

    auto grab = [sink]() -> QImage {
        QVideoFrame frame = sink->videoFrame();
        return frame.isValid() ? frame.toImage() : QImage{};
    };

    if (QThread::currentThread() == sink->thread())
        return QtFuture::makeReadyValueFuture(grab());

    QPromise<QImage> promise;
    QFuture<QImage> future = promise.future();
    promise.start();

    QMetaObject::invokeMethod(
        sink,
        [grab, p = std::move(promise)]() mutable {
            p.addResult(grab());
            p.finish();
        },
        Qt::QueuedConnection);

    return future;
}
