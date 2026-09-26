#pragma once

#include <chrono>

#include <QtPositioning/QGeoCoordinate>
#include <QtPositioning/QGeoPositionInfoSource>

#include "ScheduledTask.h"

/// Moves at a constant velocity from a reference position, which the application may set.
class SimulatedPosition : public QGeoPositionInfoSource
{
    Q_OBJECT

public:
    explicit SimulatedPosition(QObject* parent = nullptr, RuntimeScheduler* scheduler = nullptr);
    ~SimulatedPosition() override;

    QGeoPositionInfo lastKnownPosition(bool /*fromSatellitePositioningMethodsOnly = false*/) const final
    {
        return _lastPosition;
    }

    PositioningMethods supportedPositioningMethods() const final { return PositioningMethod::AllPositioningMethods; }

    int minimumUpdateInterval() const final { return static_cast<int>(kUpdateInterval.count()); }

    Error error() const final { return QGeoPositionInfoSource::NoError; }

public slots:
    void startUpdates() final;
    void stopUpdates() final;
    void requestUpdate(int timeout = 5000) final;
    /// Continues the simulation from @a coordinate; an invalid coordinate is ignored.
    void setReferencePosition(const QGeoCoordinate& coordinate);

private slots:
    void _updatePosition();

private:
    void _scheduleUpdate();

    RuntimeScheduler* const _scheduler;
    ScheduledTask _updateTask;
    quint64 _lastUpdateUs = 0;
    QGeoPositionInfo _lastPosition;

    static constexpr std::chrono::milliseconds kUpdateInterval{1000};
    static constexpr qreal kHorizontalVelocityMetersPerSec = 0.5;
    static constexpr qreal kVerticalVelocityMetersPerSec = 0.1;
    static constexpr qreal kHeading = 45.;
};
