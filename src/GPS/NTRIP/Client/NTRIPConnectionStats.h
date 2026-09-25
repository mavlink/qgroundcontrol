#pragma once

#include <chrono>

#include <QtCore/QChronoTimer>
#include <QtCore/QHash>
#include <QtCore/QObject>

#include "DataRateTracker.h"
#include "MonotonicClock.h"
#include "RTCMMessageCount.h"

class NTRIPConnectionStats : public QObject
{
    Q_OBJECT
    Q_PROPERTY(quint64 bytesReceived READ bytesReceived NOTIFY bytesReceivedChanged FINAL)
    Q_PROPERTY(quint32 messagesReceived READ messagesReceived NOTIFY messagesReceivedChanged FINAL)
    Q_PROPERTY(double dataRateBytesPerSec READ dataRateBytesPerSec NOTIFY dataRateChanged FINAL)
    Q_PROPERTY(double correctionAgeSec READ correctionAgeSec NOTIFY correctionAgeChanged FINAL)
    Q_PROPERTY(bool dataStale READ dataStale NOTIFY dataStaleChanged FINAL)
    /// Per-RTCM-message-ID counts since the current connection started.
    /// Returned as a list of [id, count] pairs sorted ascending by id so the
    /// QML Repeater can render deterministic chips without re-sorting.
    Q_PROPERTY(QList<RTCMMessageCount> messageCountsById READ messageCountsById NOTIFY messageCountsByIdChanged FINAL)

public:
    explicit NTRIPConnectionStats(QObject* parent = nullptr);

    void start();
    void stop();
    /// Count every message; health uses the newest valid monotonic receipt.
    void recordMessage(int bytes, int messageId = 0,
                       qint64 receivedAtMs = static_cast<qint64>(MonotonicClock::nowUs() / 1000));
    void reset();

    quint64 bytesReceived() const { return _rateTracker.totalBytes(); }

    quint32 messagesReceived() const { return _messagesReceived; }

    double dataRateBytesPerSec() const { return _rateTracker.bytesPerSec(); }

    double correctionAgeSec() const;

    bool dataStale() const { return _dataStale; }

    QList<RTCMMessageCount> messageCountsById() const;

signals:
    void bytesReceivedChanged();
    void messagesReceivedChanged();
    void dataRateChanged();
    void correctionAgeChanged();
    void dataStaleChanged();
    void messageCountsByIdChanged();

private:
    void _updateDataStale(std::chrono::milliseconds now);

    /// UI indicator only, measured from the last receipt or stream start; routing freshness is separate.
    static constexpr std::chrono::milliseconds kStaleThreshold{5000};

    DataRateTracker _rateTracker;
    quint64 _prevBytesReceived = 0;
    quint32 _messagesReceived = 0;
    quint32 _prevMessagesReceived = 0;
    bool _dataStale = false;
    bool _messageCountsDirty = false;
    /// Monotonic receipt times since the steady-clock epoch; zero when unset.
    std::chrono::milliseconds _lastReceivedAt{0};
    std::chrono::milliseconds _startedAt{0};
    QChronoTimer _rateTimer;
    // Per-ID counts.
    QHash<int, quint32> _messageCountsById;
};
