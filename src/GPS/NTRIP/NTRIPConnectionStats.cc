#include "NTRIPConnectionStats.h"

#include <algorithm>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(NTRIPConnectionStatsLog, "GPS.NTRIPConnectionStats")

NTRIPConnectionStats::NTRIPConnectionStats(QObject* parent) : QObject(parent), _rateTimer(this)
{
    _rateTimer.setInterval(std::chrono::seconds{1});
    _rateTimer.callOnTimeout(this, [this]() {
        const quint64 totalBytes = _rateTracker.totalBytes();
        if (totalBytes != _prevBytesReceived) {
            _prevBytesReceived = totalBytes;
            emit bytesReceivedChanged();
            emit dataRateChanged();
        } else if (_rateTracker.bytesPerSec() > 0) {
            emit dataRateChanged();
        }
        if (_prevMessagesReceived != _messagesReceived) {
            _prevMessagesReceived = _messagesReceived;
            emit messagesReceivedChanged();
        }
        if (_lastReceivedAtMs > 0) {
            emit correctionAgeChanged();
        }

        _updateDataStale(static_cast<qint64>(MonotonicClock::nowUs() / 1000));
        if (_messageCountsDirty) {
            _messageCountsDirty = false;
            emit messageCountsByIdChanged();
        }
    });
}

void NTRIPConnectionStats::start()
{
    _rateTimer.start();
}

void NTRIPConnectionStats::stop()
{
    _rateTimer.stop();
    if (_rateTracker.bytesPerSec() != 0.0) {
        _rateTracker.reset();
        _prevBytesReceived = 0;
        emit dataRateChanged();
        emit bytesReceivedChanged();
    }
}

double NTRIPConnectionStats::correctionAgeSec() const
{
    if (_lastReceivedAtMs <= 0) {
        return -1.0;
    }
    return (static_cast<qint64>(MonotonicClock::nowUs() / 1000) - _lastReceivedAtMs) / 1000.0;
}

void NTRIPConnectionStats::_updateDataStale(qint64 nowMs)
{
    const bool stale = _lastReceivedAtMs > 0 && nowMs - _lastReceivedAtMs >= kStaleThreshold.count();
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
    const qint64 nowMs = static_cast<qint64>(MonotonicClock::nowUs() / 1000);
    if (receivedAtMs <= 0 || receivedAtMs > nowMs) {
        qCWarning(NTRIPConnectionStatsLog) << "Invalid RTCM receipt timestamp:" << receivedAtMs;
    } else {
        _lastReceivedAtMs = (std::max) (_lastReceivedAtMs, receivedAtMs);
    }
    _updateDataStale(nowMs);
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
    _lastReceivedAtMs = 0;
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

QVariantList NTRIPConnectionStats::messageCountsById() const
{
    QList<int> ids = _messageCountsById.keys();
    std::sort(ids.begin(), ids.end());

    QVariantList out;
    out.reserve(ids.size());
    for (int id : ids) {
        out.append(QVariant(QVariantList{id, _messageCountsById.value(id)}));
    }
    return out;
}
