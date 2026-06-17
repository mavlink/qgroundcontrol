#pragma once

#include <QtCore/QChronoTimer>
#include <QtCore/QElapsedTimer>
#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtCore/QVariant>
#include <QtCore/QVariantList>
#include <QtQmlIntegration/QtQmlIntegration>
#include <chrono>

#include "DataRateTracker.h"

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
    /// Record a received RTCM message. messageId = 0 is treated as "unknown/unparseable"
    /// and tracked under a distinct bucket so it still shows up in diagnostics.
    void recordMessage(int bytes, int messageId = 0);
    void reset();

    quint64 bytesReceived() const { return _rateTracker.totalBytes(); }

    quint32 messagesReceived() const { return _messagesReceived; }

    double dataRateBytesPerSec() const { return _rateTracker.bytesPerSec(); }

    double correctionAgeSec() const { return _lastMessageTime.isValid() ? _lastMessageTime.elapsed() / 1000.0 : -1.0; }

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
    static constexpr std::chrono::milliseconds kStaleThreshold{5000};

    DataRateTracker _rateTracker;
    quint64 _prevBytesReceived = 0;
    quint32 _messagesReceived = 0;
    quint32 _prevMessagesReceived = 0;
    bool _dataStale = false;
    bool _messageCountsDirty = false;
    QElapsedTimer _lastMessageTime;
    QChronoTimer _rateTimer;
    // Per-ID counts. Using int for compatibility with QVariant in QML.
    QHash<int, quint32> _messageCountsById;
};
