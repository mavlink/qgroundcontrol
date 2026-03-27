#include "NTRIPConnectionStats.h"

NTRIPConnectionStats::NTRIPConnectionStats(QObject* parent)
    : QObject(parent)
    , _rateTimer(this)
{
    _rateTimer.setInterval(std::chrono::seconds{1});
    _rateTimer.callOnTimeout(this, [this]() {
        if (_rateTracker.bytesPerSec() > 0) {
            emit bytesReceivedChanged();
            emit dataRateChanged();
        }
        if (_prevMessagesReceived != _messagesReceived) {
            _prevMessagesReceived = _messagesReceived;
            emit messagesReceivedChanged();
        }
        if (_lastMessageTime.isValid()) {
            emit correctionAgeChanged();
        }

        const bool stale = _lastMessageTime.isValid() &&
                           _lastMessageTime.elapsed() >= kStaleThreshold.count() &&
                           _messagesReceived > 0;
        if (stale != _dataStale) {
            _dataStale = stale;
            emit dataStaleChanged();
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
        emit dataRateChanged();
        emit bytesReceivedChanged();
    }
}

void NTRIPConnectionStats::recordMessage(int bytes)
{
    if (bytes > 0) {
        _rateTracker.recordBytes(bytes);
        _messagesReceived++;
        _lastMessageTime.restart();
        if (_dataStale) {
            _dataStale = false;
            emit dataStaleChanged();
        }
        if (_rateTracker.rateUpdated()) {
            emit dataRateChanged();
            emit bytesReceivedChanged();
        }
    }
}

void NTRIPConnectionStats::reset()
{
    _rateTracker.reset();
    _messagesReceived = 0;
    _lastMessageTime.invalidate();
    if (_dataStale) {
        _dataStale = false;
        emit dataStaleChanged();
    }
    emit bytesReceivedChanged();
    emit messagesReceivedChanged();
    emit dataRateChanged();
    emit correctionAgeChanged();
}
