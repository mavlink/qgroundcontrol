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

    friend class GPSSourceHealthTest;

public:
    enum class State
    {
        NoData = 0,
        Usable = 1,
        Invalid = 2,
        Stale = 3,
    };
    Q_ENUM(State)

    explicit GPSSourceHealth(QObject* parent = nullptr, RuntimeScheduler* scheduler = nullptr);
    ~GPSSourceHealth() override;

    static constexpr int FRESHNESS_TIMEOUT_MS = 5000;

    int freshnessTimeoutMs() const { return _freshnessTimeoutMs; }

    void setFreshnessTimeoutMs(int timeoutMs);

    State state() const { return _state; }

    bool usable() const { return _state == State::Usable; }

    GPSObservation observation() const { return _observation; }

    std::optional<GPSObservation> acceptedObservation() const;

    QGeoCoordinate coordinate() const { return usable() ? _observation.coordinate() : QGeoCoordinate(); }

    double horizontalAccuracy() const;

    QDateTime receivedAt() const { return _observation.receivedAt; }

    void updateObservation(const GPSObservation& observation);
    void invalidatePosition();
    void reset();

signals:
    void positionChanged();

private:
    void _setState(State state);
    void _schedulePositionExpiry();
    qint64 _age(quint64 timestampUs) const;

    int _freshnessTimeoutMs = FRESHNESS_TIMEOUT_MS;
    GPSObservation _observation;
    State _state = State::NoData;
    bool _positionInvalidated = true;
    QPointer<RuntimeScheduler> _scheduler;
    ScheduledTask _positionTask;
    quint64 _revision = 0;
};
