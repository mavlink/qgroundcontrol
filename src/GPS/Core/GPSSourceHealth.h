#pragma once

#include <chrono>
#include <optional>

#include <QtCore/QDateTime>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtPositioning/QGeoPositionInfo>

#include "GPSObservation.h"
#include "GPSRevision.h"
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

    State state() const { return _position.state; }

    bool usable() const { return state() == State::Usable; }

    GPSObservation observation() const { return _position.observation; }

    quint64 observationRevision() const { return _observationRevision; }

    std::optional<GPSObservation> acceptedObservation(
        GPSObservation::PositionUse use = GPSObservation::PositionUse::GroundStation,
        std::optional<std::chrono::milliseconds> maximumAge = std::nullopt) const;

    QGeoCoordinate coordinate() const { return usable() ? _position.observation.coordinate() : QGeoCoordinate(); }

    double horizontalAccuracy() const;

    QDateTime receivedAt() const { return _position.observation.receivedAt; }

    void updateObservation(const GPSObservation& observation);
    void invalidatePosition();
    void reset();

signals:
    void positionChanged();

private:
    void _logStateChange(State previous) const;
    void _setState(State state);
    State _updatedPositionState() const;
    void _schedulePositionExpiry();
    qint64 _age(quint64 timestampUs) const;
    std::chrono::microseconds _remaining(quint64 timestampUs,
                                         std::optional<std::chrono::milliseconds> maximumAge = std::nullopt) const;

    struct PositionState
    {
        GPSObservation observation;
        State state = State::NoData;
        bool invalidated = true;
    };

    int _freshnessTimeoutMs = FRESHNESS_TIMEOUT_MS;
    PositionState _position;
    RuntimeScheduler* const _scheduler;
    ScheduledTask _positionTask;
    quint64 _observationRevision = 0;
    GPSRevision _revision;
};
