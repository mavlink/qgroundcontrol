#pragma once

#include <QtPositioning/QGeoPositionInfoSource>

#include "ScheduledTask.h"
class Vehicle;

class SimulatedPosition : public QGeoPositionInfoSource
{
   Q_OBJECT

public:
    explicit SimulatedPosition(QObject* parent = nullptr, RuntimeScheduler* scheduler = nullptr);
    ~SimulatedPosition() override;

    QGeoPositionInfo lastKnownPosition(bool /*fromSatellitePositioningMethodsOnly = false*/) const final { return _lastPosition; }

    PositioningMethods supportedPositioningMethods() const final { return PositioningMethod::AllPositioningMethods; }
    int minimumUpdateInterval() const final { return kUpdateIntervalMsecs; }
    Error error() const final { return QGeoPositionInfoSource::NoError; }

public slots:
    void startUpdates() final;
    void stopUpdates() final;
    void requestUpdate(int timeout = 5000) final;

private slots:
    void _updatePosition();
    void _vehicleAdded(Vehicle *vehicle);
    void _vehicleHomePositionChanged(QGeoCoordinate homePosition);

private:
    void _scheduleUpdate();

    QPointer<RuntimeScheduler> _scheduler;
    ScheduledTask _updateTask;
    quint64 _lastUpdateUs = 0;
    QGeoPositionInfo _lastPosition;
    QMetaObject::Connection _homePositionChangedConnection;

    static constexpr int kUpdateIntervalMsecs = 1000;
    static constexpr qreal kHorizontalVelocityMetersPerSec = 0.5;
    static constexpr qreal kVerticalVelocityMetersPerSec = 0.1;
    static constexpr qreal kHeading = 45.;
};
