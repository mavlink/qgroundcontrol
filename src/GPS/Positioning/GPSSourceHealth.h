#pragma once

#include <chrono>
#include <optional>

#include <QtCore/QObject>

#include "GPSObservation.h"
#include "ScheduledTask.h"

/// Session health is independent of transport readiness and RTK survey-in validity.
class GPSSourceHealth : public QObject
{
    Q_OBJECT
    friend class GPSSourceHealthTest;
    friend class PositionManagerTest;

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

    /// Default position age at which health becomes Stale and the position stops being usable.
    static constexpr std::chrono::milliseconds FRESHNESS_TIMEOUT{5000};

    void setFreshnessTimeout(std::chrono::milliseconds timeout);

    State state() const { return _position.state; }

    quint64 observationRevision() const { return _observationRevision; }

    std::optional<GPSObservation> acceptedObservation(
        GPSObservation::PositionUse use = GPSObservation::PositionUse::GroundStation,
        std::optional<std::chrono::milliseconds> maximumAge = std::nullopt) const;

    void updateObservation(const GPSObservation& observation);
    void invalidatePosition();
    void reset();

signals:
    void positionChanged();

private:
    /// The latest observation, whether or not it is accepted.
    GPSObservation _observation() const { return _position.observation; }

    void _logStateChange(State previous) const;
    void _setState(State state);
    State _updatedPositionState() const;
    void _schedulePositionExpiry();
    /// Empty for a missing or future timestamp.
    std::optional<std::chrono::milliseconds> _age(quint64 timestampUs) const;
    std::chrono::microseconds _remaining(quint64 timestampUs,
                                         std::optional<std::chrono::milliseconds> maximumAge = std::nullopt) const;

    struct PositionState
    {
        GPSObservation observation;
        State state = State::NoData;
        bool invalidated = true;
    };

    std::chrono::milliseconds _freshnessTimeout = FRESHNESS_TIMEOUT;
    PositionState _position;
    RuntimeScheduler* const _scheduler;
    ScheduledTask _positionTask;
    quint64 _observationRevision = 0;
};
