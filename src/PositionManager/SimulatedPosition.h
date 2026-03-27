#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QTimer>
#include <QtPositioning/QGeoPositionInfoSource>

Q_DECLARE_LOGGING_CATEGORY(SimulatedPositionLog)

class Vehicle;

class SimulatedPosition : public QGeoPositionInfoSource
{
   Q_OBJECT

public:
    SimulatedPosition(QObject* parent = nullptr);
    ~SimulatedPosition();

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
    QTimer _updateTimer;
    QGeoPositionInfo _lastPosition;
    QMetaObject::Connection _homePositionChangedConnection;

    static constexpr int kUpdateIntervalMsecs = 1000;
    static constexpr qreal kHorizontalVelocityMetersPerSec = 0.5;
    static constexpr qreal kVerticalVelocityMetersPerSec = 0.1;
    static constexpr qreal kHeading = 45.;
};
