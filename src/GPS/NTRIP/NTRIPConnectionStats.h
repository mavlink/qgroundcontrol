#pragma once

#include <chrono>

#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtQmlIntegration/QtQmlIntegration>

#include "DataRateTracker.h"
#include "RTCMMessageCount.h"
#include "ScheduledTask.h"

class RuntimeScheduler;

/// Statistics of the current NTRIP stream for display. Every property notifies through statsChanged(), which follows
/// each change of the stream's lifecycle and, while it runs, every second.
class NTRIPConnectionStats : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_PROPERTY(quint64 bytesReceived READ bytesReceived NOTIFY statsChanged FINAL)
    Q_PROPERTY(quint32 messagesReceived READ messagesReceived NOTIFY statsChanged FINAL)
    Q_PROPERTY(double dataRateBytesPerSec READ dataRateBytesPerSec NOTIFY statsChanged FINAL)
    Q_PROPERTY(double correctionAgeSec READ correctionAgeSec NOTIFY statsChanged FINAL)
    Q_PROPERTY(bool dataStale READ dataStale NOTIFY statsChanged FINAL)
    /// More than DATA_USAGE_WARNING_BYTES received, which may cost money on metered connections.
    Q_PROPERTY(bool dataUsageHigh READ dataUsageHigh NOTIFY statsChanged FINAL)
    /// Per-RTCM-message-ID counts since the current connection started, ascending by ID so the QML chips need no
    /// sorting.
    Q_PROPERTY(QList<RTCMMessageCount> messageCountsById READ messageCountsById NOTIFY statsChanged FINAL)

public:
    static constexpr quint64 DATA_USAGE_WARNING_BYTES = 50 * 1024 * 1024;

    /// The stream counts as stale once no message has arrived for @a staleAfter.
    explicit NTRIPConnectionStats(std::chrono::milliseconds staleAfter, QObject* parent = nullptr,
                                  RuntimeScheduler* scheduler = nullptr);

    void start();
    void stop();
    /// Count every message; health uses the newest valid receipt on the scheduler's clock.
    void recordMessage(int bytes, int messageId, qint64 receivedAtMs);
    void reset();

    quint64 bytesReceived() const { return _rateTracker.totalBytes(); }

    quint32 messagesReceived() const { return _messagesReceived; }

    double dataRateBytesPerSec() const { return _rateTracker.bytesPerSec(); }

    double correctionAgeSec() const;

    bool dataStale() const { return _dataStale; }

    bool dataUsageHigh() const { return bytesReceived() > DATA_USAGE_WARNING_BYTES; }

    QList<RTCMMessageCount> messageCountsById() const;

signals:
    void statsChanged();

private:
    /// Returns whether the stale state changed.
    bool _updateDataStale(std::chrono::milliseconds now);
    std::chrono::milliseconds _now() const;

    const std::chrono::milliseconds _staleAfter;
    RuntimeScheduler* const _scheduler;
    ScheduledTask _publishTask;
    DataRateTracker _rateTracker;
    quint32 _messagesReceived = 0;
    bool _dataStale = false;
    /// Receipt times on the scheduler's clock; zero when unset.
    std::chrono::milliseconds _lastReceivedAt{0};
    std::chrono::milliseconds _startedAt{0};
    QHash<int, quint32> _messageCountsById;
};
