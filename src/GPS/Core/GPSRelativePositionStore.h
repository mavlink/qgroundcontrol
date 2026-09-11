#pragma once

#include <QtCore/QObject>
#include <QtCore/QPointer>

#include "GPSObservation.h"
#include "ScheduledTask.h"

/// Accepted relative receiver data and its session-scoped freshness, independent of presentation.
class GPSRelativePositionStore : public QObject
{
    Q_OBJECT

public:
    explicit GPSRelativePositionStore(QObject* parent = nullptr, int freshnessTimeoutMs = 5000,
                                      RuntimeScheduler* scheduler = nullptr);
    ~GPSRelativePositionStore() override;

    void beginSession(const QString& sourceId, quint64 sessionId);
    void updateObservation(const GPSRelativeObservation& observation);
    void reset();

    QString sourceId() const { return _sourceId; }

    quint64 sessionId() const { return _sessionId; }

    bool fresh() const { return _fresh; }

    GPSRelativeObservation observation() const { return _observation; }

signals:
    void observationChanged(const GPSRelativeObservation& observation);

private:
    void _expire();
    void _publish();

    QPointer<RuntimeScheduler> _scheduler;
    ScheduledTask _expiryTask;
    QString _sourceId;
    quint64 _sessionId = 0;
    GPSRelativeObservation _observation;
    int _freshnessTimeoutMs;
    bool _fresh = false;
};
