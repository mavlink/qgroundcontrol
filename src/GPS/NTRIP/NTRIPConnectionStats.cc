#include "NTRIPConnectionStats.h"

#include <algorithm>
#include <utility>

#include "MonotonicClock.h"
#include "QGCLoggingCategory.h"
#include "RuntimeScheduler.h"

QGC_LOGGING_CATEGORY(NTRIPConnectionStatsLog, "GPS.NTRIPConnectionStats")

NTRIPConnectionStats::NTRIPConnectionStats(std::chrono::milliseconds staleAfter, QObject* parent,
                                           RuntimeScheduler* scheduler)
    : QObject(parent)
    , _staleAfter(staleAfter)
    , _scheduler(RuntimeScheduler::orDefault(scheduler, this))
    , _publishTask(_scheduler, this)
    , _rateTracker([this]() { return _scheduler->nowUs(); })
{}

std::chrono::milliseconds NTRIPConnectionStats::_now() const
{
    return std::chrono::milliseconds(_scheduler->nowMs());
}

void NTRIPConnectionStats::start()
{
    _startedAt = _now();
    _publishTask.scheduleRepeating(std::chrono::seconds{1}, [this]() {
        _rateTracker.refresh();
        (void) _updateDataStale(_now());
        emit statsChanged();
    });
}

void NTRIPConnectionStats::stop()
{
    _publishTask.cancel();
    _startedAt = std::chrono::milliseconds::zero();
    _rateTracker.resetRate();
    emit statsChanged();
}

double NTRIPConnectionStats::correctionAgeSec() const
{
    const auto age = MonotonicClock::age(_lastReceivedAt, std::chrono::milliseconds(_scheduler->nowMs()));
    return age ? std::chrono::duration<double>(*age).count() : -1.0;
}

bool NTRIPConnectionStats::_updateDataStale(std::chrono::milliseconds now)
{
    const auto since = _lastReceivedAt > std::chrono::milliseconds::zero() ? _lastReceivedAt : _startedAt;
    const auto age = MonotonicClock::age(since, now);
    const bool stale = age && *age >= _staleAfter;
    return std::exchange(_dataStale, stale) != stale;
}

void NTRIPConnectionStats::recordMessage(int bytes, int messageId, qint64 receivedAtMs)
{
    if (bytes <= 0) {
        return;
    }
    _rateTracker.recordBytes(bytes);
    _messagesReceived++;
    ++_messageCountsById[messageId];
    const auto now = _now();
    const std::chrono::milliseconds receivedAt(receivedAtMs);
    if (receivedAt <= std::chrono::milliseconds::zero() || receivedAt > now) {
        qCWarning(NTRIPConnectionStatsLog) << "Invalid RTCM receipt timestamp:" << receivedAtMs;
    } else {
        _lastReceivedAt = (std::max) (_lastReceivedAt, receivedAt);
    }
    // A stream that recovers shows it at once; the counters follow at the next second.
    if (_updateDataStale(now)) {
        emit statsChanged();
    }
}

void NTRIPConnectionStats::reset()
{
    _rateTracker.reset();
    _messagesReceived = 0;
    _lastReceivedAt = std::chrono::milliseconds::zero();
    _startedAt = std::chrono::milliseconds::zero();
    _messageCountsById.clear();
    _dataStale = false;
    emit statsChanged();
}

QList<RTCMMessageCount> NTRIPConnectionStats::messageCountsById() const
{
    QList<RTCMMessageCount> counts;
    counts.reserve(_messageCountsById.size());
    for (auto it = _messageCountsById.cbegin(); it != _messageCountsById.cend(); ++it) {
        counts.append({it.key(), it.value()});
    }
    std::ranges::sort(counts, {}, &RTCMMessageCount::messageId);
    return counts;
}
