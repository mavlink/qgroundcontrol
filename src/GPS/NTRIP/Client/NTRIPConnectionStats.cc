#include "NTRIPConnectionStats.h"

#include <algorithm>
#include <optional>

#include <QtCore/QPointer>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(NTRIPConnectionStatsLog, "GPS.NTRIP.NTRIPConnectionStats")

namespace {
std::chrono::milliseconds monotonicNow()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(MonotonicClock::now());
}

/// Empty for a missing or future receipt.
std::optional<std::chrono::milliseconds> ageAt(std::chrono::milliseconds receivedAt, std::chrono::milliseconds now)
{
    return receivedAt > std::chrono::milliseconds::zero() && receivedAt <= now ? std::optional(now - receivedAt)
                                                                               : std::nullopt;
}
}  // namespace

NTRIPConnectionStats::NTRIPConnectionStats(QObject* parent)
    : QObject(parent)
    , _rateTimer(this)
{
    _rateTimer.setInterval(std::chrono::seconds{1});
    _rateTimer.callOnTimeout(this, [this]() {
        const double previousRate = _rateTracker.bytesPerSec();
        _rateTracker.refresh();
        const quint64 totalBytes = _rateTracker.totalBytes();
        if (totalBytes != _prevBytesReceived) {
            _prevBytesReceived = totalBytes;
            emit bytesReceivedChanged();
            emit dataRateChanged();
        } else if (_rateTracker.bytesPerSec() != previousRate) {
            emit dataRateChanged();
        }
        if (_prevMessagesReceived != _messagesReceived) {
            _prevMessagesReceived = _messagesReceived;
            emit messagesReceivedChanged();
        }
        if (_lastReceivedAt > std::chrono::milliseconds::zero()) {
            emit correctionAgeChanged();
        }

        _updateDataStale(monotonicNow());
        if (_messageCountsDirty) {
            _messageCountsDirty = false;
            emit messageCountsByIdChanged();
        }
    });
}

void NTRIPConnectionStats::start()
{
    _startedAt = monotonicNow();
    _rateTimer.start();
}

void NTRIPConnectionStats::stop()
{
    _rateTimer.stop();
    _startedAt = std::chrono::milliseconds::zero();
    const bool rateChanged = _rateTracker.bytesPerSec() != 0.0;
    const bool bytesChanged = _prevBytesReceived != _rateTracker.totalBytes();
    const bool messagesChanged = _prevMessagesReceived != _messagesReceived;
    const bool countsChanged = _messageCountsDirty;
    _prevBytesReceived = _rateTracker.totalBytes();
    _prevMessagesReceived = _messagesReceived;
    _messageCountsDirty = false;
    _rateTracker.resetRate();
    const QPointer<NTRIPConnectionStats> guard(this);
    if (rateChanged) {
        emit dataRateChanged();
    }
    if (guard && bytesChanged) {
        emit bytesReceivedChanged();
    }
    if (guard && messagesChanged) {
        emit messagesReceivedChanged();
    }
    if (guard && countsChanged) {
        emit messageCountsByIdChanged();
    }
}

double NTRIPConnectionStats::correctionAgeSec() const
{
    const auto age = ageAt(_lastReceivedAt, monotonicNow());
    return age ? std::chrono::duration<double>(*age).count() : -1.0;
}

void NTRIPConnectionStats::_updateDataStale(std::chrono::milliseconds now)
{
    const auto age = ageAt(_lastReceivedAt > std::chrono::milliseconds::zero() ? _lastReceivedAt : _startedAt, now);
    const bool stale = age && *age >= kStaleThreshold;
    if (stale != _dataStale) {
        _dataStale = stale;
        emit dataStaleChanged();
    }
}

void NTRIPConnectionStats::recordMessage(int bytes, int messageId, qint64 receivedAtMs)
{
    if (bytes <= 0) {
        return;
    }
    _rateTracker.recordBytes(bytes);
    _messagesReceived++;
    const auto now = monotonicNow();
    const std::chrono::milliseconds receivedAt(receivedAtMs);
    if (receivedAt <= std::chrono::milliseconds::zero() || receivedAt > now) {
        qCWarning(NTRIPConnectionStatsLog) << "Invalid RTCM receipt timestamp:" << receivedAtMs;
    } else {
        _lastReceivedAt = (std::max) (_lastReceivedAt, receivedAt);
    }
    _updateDataStale(now);
    if (_rateTracker.rateUpdated()) {
        _prevBytesReceived = _rateTracker.totalBytes();
        emit dataRateChanged();
        emit bytesReceivedChanged();
    }
    ++_messageCountsById[messageId];
    _messageCountsDirty = true;
}

void NTRIPConnectionStats::reset()
{
    _rateTracker.reset();
    _prevBytesReceived = 0;
    _messagesReceived = 0;
    _prevMessagesReceived = 0;
    _lastReceivedAt = std::chrono::milliseconds::zero();
    _startedAt = std::chrono::milliseconds::zero();
    _messageCountsById.clear();
    _messageCountsDirty = false;
    if (_dataStale) {
        _dataStale = false;
        emit dataStaleChanged();
    }
    emit bytesReceivedChanged();
    emit messagesReceivedChanged();
    emit dataRateChanged();
    emit correctionAgeChanged();
    emit messageCountsByIdChanged();
}

QList<RTCMMessageCount> NTRIPConnectionStats::messageCountsById() const
{
    return rtcmMessageCounts(_messageCountsById);
}
