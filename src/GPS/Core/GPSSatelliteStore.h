#pragma once

#include <QtCore/QObject>

#include "GPSSatelliteState.h"
#include "ScheduledTask.h"

/// Authoritative accepted satellite state, with independent constellation and view/use deadlines.
class GPSSatelliteStore : public QObject
{
    Q_OBJECT
public:
    explicit GPSSatelliteStore(QObject* parent = nullptr, int freshnessTimeoutMs = 5000,
                               RuntimeScheduler* scheduler = nullptr);
    ~GPSSatelliteStore() override;

    void beginSession(const QString& sourceId, quint64 sessionId);
    void updateObservation(const GPSSatelliteObservation& observation);
    void clear();
    void reset();
    void setFreshnessTimeoutMs(int timeoutMs);

    GPSSatelliteObservation observation() const { return _observation; }

signals:
    void observationChanged(const GPSSatelliteObservation& observation);

private:
    void _publish();

    GPSSatelliteState _state;
    GPSSatelliteObservation _observation;
    QPointer<RuntimeScheduler> _scheduler;
    ScheduledTask _expiryTask;
    quint64 _revision = 0;
};
