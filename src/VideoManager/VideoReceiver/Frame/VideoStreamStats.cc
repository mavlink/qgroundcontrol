#include "VideoStreamStats.h"

#include <algorithm>

#include "QGCLoggingCategory.h"
#include "VideoFrameDelivery.h"

QGC_LOGGING_CATEGORY(VideoStreamStatsLog, "Video.VideoStreamStats")

namespace {
// FPS emit thresholds
static constexpr float kFpsChangeDelta = 0.1F;         ///< Minimum FPS change before emitting
static constexpr qint64 kFpsEmitIntervalMs = 500;       ///< Max ms between FPS emissions
// Latency emit threshold
static constexpr float kLatencyChangeDelta = 0.5F;     ///< Minimum latency change (ms) before emitting
// Stream health drop-rate thresholds
static constexpr float kDropRateCritical = 0.2F;        ///< Drop rate above which → Critical
static constexpr float kDropRateDegraded = 0.05F;       ///< Drop rate above which → Degraded
// Stream health FPS thresholds
static constexpr float kFpsCritical = 5.0F;             ///< FPS below which → Critical
static constexpr float kFpsDegraded = 15.0F;            ///< FPS below which → Degraded
}  // namespace

VideoStreamStats::VideoStreamStats(QObject* parent) : QObject(parent)
{
    _fpsTimer.start();

    _updateTimer.setInterval(500);
    _updateTimer.setTimerType(Qt::CoarseTimer);
    connect(&_updateTimer, &QTimer::timeout, this, &VideoStreamStats::_update);
}

void VideoStreamStats::setFrameDelivery(VideoFrameDelivery* delivery)
{
    if (_delivery == delivery)
        return;

    disconnect(_frameConn);
    _updateTimer.stop();
    _delivery = delivery;

    if (_delivery) {
        _frameConn = connect(_delivery, &VideoFrameDelivery::frameArrived, this, &VideoStreamStats::_onFrameArrived);
        _updateTimer.start();
    }
}

float VideoStreamStats::fps() const
{
    QMutexLocker lock(&_fpsMutex);

    if (_frameTimeCount < 2)
        return 0.0F;

    const int oldest = (_frameTimeHead - _frameTimeCount + kFpsWindowSize) % kFpsWindowSize;
    const int newest = (_frameTimeHead - 1 + kFpsWindowSize) % kFpsWindowSize;
    const qint64 span = _frameTimes[newest] - _frameTimes[oldest];

    if (span <= 0)
        return 0.0F;

    return static_cast<float>(_frameTimeCount - 1) * 1000.0F / static_cast<float>(span);
}

float VideoStreamStats::latencyMs() const
{
    return _delivery ? _delivery->latencyMs() : -1.0F;
}

quint64 VideoStreamStats::droppedFrames() const
{
    return _delivery ? _delivery->droppedFrames() : 0;
}

void VideoStreamStats::reset()
{
    {
        QMutexLocker lock(&_fpsMutex);
        _frameTimeHead = 0;
        _frameTimeCount = 0;
        _fpsTimer.restart();
    }
    _lastEmittedFps = 0.0F;
    _lastEmittedLatency = -1.0F;
    _lastEmittedDroppedFrames = 0;
    _lastFpsEmitMs = 0;
    _lastLatencyEmitMs = 0;
    _streamHealth = Health::Good;
    _lastUpdateFrameCount = 0;
    // Clear rolling drop-rate buffer
    _dropHead = 0;
    _dropCount = 0;
    std::fill(std::begin(_dropSnapshots), std::end(_dropSnapshots), DropSnapshot{});
    emit fpsChanged(0.0F);
    emit latencyChanged(-1.0F);
    emit streamHealthChanged(Health::Good);
    emit droppedFramesChanged(0);
}

void VideoStreamStats::_onFrameArrived()
{
    QMutexLocker lock(&_fpsMutex);
    const qint64 now = _fpsTimer.elapsed();
    _frameTimes[_frameTimeHead] = now;
    _frameTimeHead = (_frameTimeHead + 1) % kFpsWindowSize;
    if (_frameTimeCount < kFpsWindowSize)
        ++_frameTimeCount;
}

void VideoStreamStats::_update()
{
    // Early-exit gate: if no new frames arrived since the last tick AND the
    // stream is already healthy, skip all the expensive computation.
    // This prevents redundant mutex acquisitions and ring-buffer writes when
    // the stream is idle (e.g. no video source connected, or stopped).
    const quint64 currentFrameCount = _delivery ? _delivery->frameCount() : 0;
    const bool anyFrames = (currentFrameCount != _lastUpdateFrameCount);
    _lastUpdateFrameCount = currentFrameCount;
    if (!anyFrames && _streamHealth == Health::Good)
        return;

    const float currentFps = fps();

    // Emit fpsChanged when the delta is >= 0.1 FPS (meaningful change) OR
    // at least 500 ms have elapsed since the last emission (periodic refresh).
    // Internal sample cadence (_fpsTimer / kFpsWindowSize) is unchanged.
    const qint64 nowMs = _fpsTimer.elapsed();
    const bool deltaSignificant = qAbs(currentFps - _lastEmittedFps) >= kFpsChangeDelta;
    const bool timedOut = (nowMs - _lastFpsEmitMs) >= kFpsEmitIntervalMs;
    if (deltaSignificant || timedOut) {
        _lastEmittedFps = currentFps;
        _lastFpsEmitMs = nowMs;
        emit fpsChanged(currentFps);
    }

    // Compute drop rate over the last ~10 s (up to kDropSnapshotCount ticks).
    // Cumulative counters would skew toward "healthy" on long sessions even
    // when drops are currently severe, so we use a rolling window instead.
    Health health = Health::Good;
    if (_delivery) {
        const quint64 nowFrames = _delivery->frameCount();
        const quint64 nowDrops = _delivery->droppedFrames();

        _dropSnapshots[_dropHead] = {nowFrames, nowDrops};
        _dropHead = (_dropHead + 1) % kDropSnapshotCount;
        if (_dropCount < kDropSnapshotCount)
            ++_dropCount;

        float dropRate = 0.0F;
        if (_dropCount >= 2) {
            const int oldestIdx = (_dropHead - _dropCount + kDropSnapshotCount) % kDropSnapshotCount;
            const int newestIdx = (_dropHead - 1 + kDropSnapshotCount) % kDropSnapshotCount;
            const quint64 frameSpan = _dropSnapshots[newestIdx].frameCount - _dropSnapshots[oldestIdx].frameCount;
            const quint64 dropSpan = _dropSnapshots[newestIdx].droppedFrames - _dropSnapshots[oldestIdx].droppedFrames;
            dropRate = (frameSpan > 0) ? static_cast<float>(dropSpan) / static_cast<float>(frameSpan) : 0.0F;
        }

        if (currentFps > 0.0F) {
            if (currentFps < kFpsCritical || dropRate > kDropRateCritical)
                health = Health::Critical;
            else if (currentFps < kFpsDegraded || dropRate > kDropRateDegraded)
                health = Health::Degraded;
        } else if (nowFrames > 0) {
            health = Health::Critical;
        }
    }

    if (health != _streamHealth) {
        _streamHealth = health;
        emit streamHealthChanged(_streamHealth);
    }

    const quint64 currentDroppedFrames = droppedFrames();
    if (currentDroppedFrames != _lastEmittedDroppedFrames) {
        _lastEmittedDroppedFrames = currentDroppedFrames;
        emit droppedFramesChanged(currentDroppedFrames);
    }

    // Latency — emit on delta OR when 500 ms elapsed (mirrors FPS debounce #21).
    if (_delivery) {
        const float lat = _delivery->latencyMs();
        const bool latDelta = qAbs(lat - _lastEmittedLatency) > kLatencyChangeDelta;
        const bool latTimedOut = (nowMs - _lastLatencyEmitMs) >= kFpsEmitIntervalMs;
        if (latDelta || latTimedOut) {
            _lastEmittedLatency = lat;
            _lastLatencyEmitMs = nowMs;
            emit latencyChanged(lat);
        }
    }
}
