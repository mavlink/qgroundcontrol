#pragma once

#include <QtCore/QObject>

#include <map>

#include "GPSObservation.h"
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
    struct ConstellationState
    {
        quint64 viewReceiptUs = 0;
        quint64 useReceiptUs = 0;
        quint64 viewRetiredThroughUs = 0;
        quint64 useRetiredThroughUs = 0;
        QList<GPSSatellite> satellites;
        std::map<std::pair<int, int>, bool> used;
        std::optional<int> usedCount;
        std::optional<QList<int>> usedIds;
    };

    void _publish();
    void _expire(quint64 nowUs);
    bool _accept(quint64 receipt, quint64 current, quint64& retired, quint64 nowUs) const;

    std::map<GPSSatellite::Constellation, ConstellationState> _constellations;
    GPSSatelliteObservation _observation;
    QPointer<RuntimeScheduler> _scheduler;
    ScheduledTask _expiryTask;
    int _freshnessTimeoutMs;
    quint64 _clearedThroughUs = 0;
    quint64 _revision = 0;
    quint64 _fullSnapshotReceiptUs = 0;
};
