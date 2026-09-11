#pragma once

#include <QtCore/QDateTime>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtPositioning/QGeoPositionInfo>

#include "GPSObservation.h"
#include "ScheduledTask.h"

/// Session health is independent of transport readiness and RTK survey-in validity.
class GPSSourceHealth : public QObject
{
    Q_OBJECT
    Q_PROPERTY(State state READ state NOTIFY positionChanged)
    Q_PROPERTY(bool usable READ usable NOTIFY positionChanged)
    Q_PROPERTY(QGeoCoordinate coordinate READ coordinate NOTIFY positionChanged)
    Q_PROPERTY(double horizontalAccuracy READ horizontalAccuracy NOTIFY positionChanged)
    Q_PROPERTY(QDateTime receivedAt READ receivedAt NOTIFY positionChanged)
    Q_PROPERTY(int satellitesInViewCount READ satellitesInViewCount NOTIFY satellitesChanged)
    Q_PROPERTY(int satellitesInUseCount READ satellitesInUseCount NOTIFY satellitesChanged)

    friend class GPSSourceHealthTest;
    friend class PositionManagerTest;
    friend class NMEASourceManagerTest;

public:
    enum State
    {
        NoData,
        Usable,
        Invalid,
        Stale
    };
    Q_ENUM(State)

    explicit GPSSourceHealth(QObject* parent = nullptr, RuntimeScheduler* scheduler = nullptr);
    ~GPSSourceHealth() override;

    static constexpr int FRESHNESS_TIMEOUT_MS = 5000;
    static qint64 ageMilliseconds(quint64 monotonicTimestampUs);

    int freshnessTimeoutMs() const { return _freshnessTimeoutMs; }

    void setFreshnessTimeoutMs(int timeoutMs);

    State state() const { return _state; }

    bool usable() const { return _state == Usable; }

    GPSObservation observation() const { return _observation; }

    std::optional<GPSObservation> acceptedObservation(GPSObservation::PositionUse use) const;

    QGeoCoordinate coordinate() const { return usable() ? _observation.coordinate() : QGeoCoordinate(); }

    double horizontalAccuracy() const;

    QDateTime receivedAt() const { return _observation.receivedAt; }

    int satellitesInViewCount() const { return _satellitesInViewCount; }

    int satellitesInUseCount() const
    {
        return _fixSatellitesInUseCount >= 0 ? _fixSatellitesInUseCount : _satellitesInUseCount;
    }

    void updateObservation(const GPSObservation& observation);
    void updatePosition(const QGeoPositionInfo& position, qint64 ageMs = 0);
    void invalidatePosition();
    void reset();
    void applySatelliteObservation(const GPSSatelliteObservation& observation);
    /// Satellite decoder failures do not invalidate a fresh count supplied with a position fix.
    void clearSatelliteReports();
    void clearSatellites();

signals:
    void positionChanged();
    void satellitesChanged();

private:
    void _setState(State state);
    void _schedulePositionExpiry();
    qint64 _age(quint64 timestampUs) const;
    void _updateFixSatelliteCount(int count, qint64 ageMs);

    int _freshnessTimeoutMs = FRESHNESS_TIMEOUT_MS;
    GPSObservation _observation;
    State _state = NoData;
    bool _positionInvalidated = true;
    QPointer<RuntimeScheduler> _scheduler;
    ScheduledTask _positionTask;
    ScheduledTask _fixSatellitesTask;
    int _satellitesInViewCount = -1;
    int _satellitesInUseCount = -1;
    int _fixSatellitesInUseCount = -1;
    quint64 _revision = 0;
};
