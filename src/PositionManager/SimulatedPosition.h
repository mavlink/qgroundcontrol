/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtPositioning/QGeoPositionInfoSource>
#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(SimulatedPositionLog)

class Vehicle;
class QTimer;

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
    QTimer *_updateTimer = nullptr;
    QGeoPositionInfo _lastPosition;
    QMetaObject::Connection _homePositionChangedConnection;

    static constexpr int kUpdateIntervalMsecs = 1000;
    static constexpr qreal kHorizontalVelocityMetersPerSec = 0.5;
    static constexpr qreal kVerticalVelocityMetersPerSec = 0.1;
    static constexpr qreal kHeading = 45.;
};
