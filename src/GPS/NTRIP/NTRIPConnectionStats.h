#pragma once

#include <chrono>

#include <QtCore/QChronoTimer>
#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtCore/QVariant>
#include <QtCore/QVariantList>
#include <QtQmlIntegration/QtQmlIntegration>

#include "DataRateTracker.h"
#include "MonotonicClock.h"

class NTRIPConnectionStats : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_PROPERTY(quint64 bytesReceived READ bytesReceived NOTIFY bytesReceivedChanged)
    Q_PROPERTY(quint32 messagesReceived READ messagesReceived NOTIFY messagesReceivedChanged)
    Q_PROPERTY(double dataRateBytesPerSec READ dataRateBytesPerSec NOTIFY dataRateChanged)
    Q_PROPERTY(double correctionAgeSec READ correctionAgeSec NOTIFY correctionAgeChanged)
    Q_PROPERTY(bool dataStale READ dataStale NOTIFY dataStaleChanged)
    /// Per-RTCM-message-ID counts since the current connection started.
    /// Returned as a list of [id, count] pairs sorted ascending by id so the
    /// QML Repeater can render deterministic chips without re-sorting.
    Q_PROPERTY(QVariantList messageCountsById READ messageCountsById NOTIFY messageCountsByIdChanged)

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

    QVariantList messageCountsById() const;

signals:
    void bytesReceivedChanged();
    void messagesReceivedChanged();
    void dataRateChanged();
    void correctionAgeChanged();
    void dataStaleChanged();
    void messageCountsByIdChanged();

private:
    void _updateDataStale(qint64 nowMs);

    static constexpr std::chrono::milliseconds kStaleThreshold{5000};

    DataRateTracker _rateTracker;
    quint64 _prevBytesReceived = 0;
    quint32 _messagesReceived = 0;
    quint32 _prevMessagesReceived = 0;
    bool _dataStale = false;
    bool _messageCountsDirty = false;
    qint64 _lastReceivedAtMs = 0;
    QChronoTimer _rateTimer;
    // Per-ID counts. Using int for compatibility with QVariant in QML.
    QHash<int, quint32> _messageCountsById;
};
