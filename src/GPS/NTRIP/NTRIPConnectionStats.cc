#include "NTRIPConnectionStats.h"

#include <QtCore/QPointer>

#include <algorithm>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(NTRIPConnectionStatsLog, "GPS.NTRIP.NTRIPConnectionStats")

NTRIPConnectionStats::NTRIPConnectionStats(QObject* parent)
    : QObject(parent)
    , _rateTimer(this)
{
    qCDebug(NTRIPConnectionStatsLog) << this;
    _rateTimer.setInterval(std::chrono::seconds{1});
    _rateTimer.callOnTimeout(this, [this]() {
        const QPointer<NTRIPConnectionStats> guard(this);
        const auto revision = ++_revision;
        const quint64 totalBytes = _rateTracker.totalBytes();
        if (totalBytes != _prevBytesReceived) {
            _prevBytesReceived = totalBytes;
            emit bytesReceivedChanged();
            if (!guard || revision != _revision) {
                return;
            }
            emit dataRateChanged();
            if (!guard || revision != _revision) {
                return;
            }
        } else if (_rateTracker.bytesPerSec() > 0) {
            emit dataRateChanged();
            if (!guard || revision != _revision) {
                return;
            }
        }
        if (_prevMessagesReceived != _messagesReceived) {
            _prevMessagesReceived = _messagesReceived;
            emit messagesReceivedChanged();
            if (!guard || revision != _revision) {
                return;
            }
        }
        if (_lastMessageTime.isValid()) {
            emit correctionAgeChanged();
            if (!guard || revision != _revision) {
                return;
            }
        }

        const qint64 age = _lastMessageTime.isValid() ? _lastMessageTime.elapsed()
                           : _streamStarted.isValid() ? _streamStarted.elapsed()
                                                      : 0;
        const bool stale = age >= kStaleThreshold.count();
        if (stale != _dataStale) {
            _dataStale = stale;
            emit dataStaleChanged();
            if (!guard || revision != _revision) {
                return;
            }
        }
        if (_messageCountsDirty) {
            _messageCountsDirty = false;
            emit messageCountsByIdChanged();
            if (!guard || revision != _revision) {
                return;
            }
        }
    });
}

NTRIPConnectionStats::~NTRIPConnectionStats()
{
    qCDebug(NTRIPConnectionStatsLog) << this;
}

void NTRIPConnectionStats::start()
{
    ++_revision;
    _streamStarted.start();
    _rateTimer.start();
}

void NTRIPConnectionStats::stop()
{
    const QPointer<NTRIPConnectionStats> guard(this);
    const auto revision = ++_revision;
    _rateTimer.stop();
    if (_rateTracker.bytesPerSec() != 0.0) {
        _rateTracker.reset();
        _prevBytesReceived = 0;
        emit dataRateChanged();
        if (!guard || revision != _revision) {
            return;
        }
        emit bytesReceivedChanged();
        if (!guard || revision != _revision) {
            return;
        }
    }
}

void NTRIPConnectionStats::recordMessage(int bytes, int messageId)
{
    const QPointer<NTRIPConnectionStats> guard(this);
    const auto revision = ++_revision;
    if (bytes <= 0) {
        return;
    }
    _rateTracker.recordBytes(bytes);
    _messagesReceived++;
    ++_messageCountsById[messageId];
    _messageCountsDirty = true;
    _lastMessageTime.restart();
    if (_dataStale) {
        _dataStale = false;
        emit dataStaleChanged();
        if (!guard || revision != _revision) {
            return;
        }
    }
    if (_rateTracker.rateUpdated()) {
        _prevBytesReceived = _rateTracker.totalBytes();
        emit dataRateChanged();
        if (!guard || revision != _revision) {
            return;
        }
        emit bytesReceivedChanged();
        if (!guard || revision != _revision) {
            return;
        }
    }
}

void NTRIPConnectionStats::reset()
{
    const QPointer<NTRIPConnectionStats> guard(this);
    const auto revision = ++_revision;
    _rateTracker.reset();
    _prevBytesReceived = 0;
    _messagesReceived = 0;
    _lastMessageTime.invalidate();
    _streamStarted.invalidate();
    _networkBytesReceived = 0;
    _validatedFrames = 0;
    _filteredFrames = 0;
    _messageCountsById.clear();
    _messageCountsDirty = false;
    if (_dataStale) {
        _dataStale = false;
        emit dataStaleChanged();
        if (!guard || revision != _revision) {
            return;
        }
    }
    emit bytesReceivedChanged();
    if (!guard || revision != _revision) {
        return;
    }
    emit messagesReceivedChanged();
    if (!guard || revision != _revision) {
        return;
    }
    emit dataRateChanged();
    if (!guard || revision != _revision) {
        return;
    }
    emit correctionAgeChanged();
    if (!guard || revision != _revision) {
        return;
    }
    emit messageCountsByIdChanged();
    if (!guard || revision != _revision) {
        return;
    }
    emit validationChanged();
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

void NTRIPConnectionStats::recordNetworkBytes(qint64 bytes)
{
    ++_revision;
    if (bytes > 0) {
        _networkBytesReceived += bytes;
        emit validationChanged();
    }
}

void NTRIPConnectionStats::recordValidatedFrame(bool filtered)
{
    const QPointer<NTRIPConnectionStats> guard(this);
    const auto revision = ++_revision;
    ++_validatedFrames;
    if (filtered) {
        ++_filteredFrames;
    }
    _lastMessageTime.restart();
    if (_dataStale) {
        _dataStale = false;
        emit dataStaleChanged();
        if (!guard || revision != _revision) {
            return;
        }
    }
    emit validationChanged();
}
