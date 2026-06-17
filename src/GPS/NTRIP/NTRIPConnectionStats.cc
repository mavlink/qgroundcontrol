#include "NTRIPConnectionStats.h"

#include <algorithm>

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
        if (_lastMessageTime.isValid()) {
            emit correctionAgeChanged();
        }

        const bool stale = _lastMessageTime.isValid() && _lastMessageTime.elapsed() >= kStaleThreshold.count() &&
                           _messagesReceived > 0;
        if (stale != _dataStale) {
            _dataStale = stale;
            emit dataStaleChanged();
        }
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

void NTRIPConnectionStats::recordMessage(int bytes, int messageId)
{
    if (bytes <= 0) {
        return;
    }
    _rateTracker.recordBytes(bytes);
    _messagesReceived++;
    _lastMessageTime.restart();
    if (_dataStale) {
        _dataStale = false;
        emit dataStaleChanged();
    }
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
    _lastMessageTime.invalidate();
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
