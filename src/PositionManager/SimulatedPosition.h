/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtPositioning/qgeopositioninfosource.h>
#include "QGCToolbox.h"
#include <QTimer>

class Vehicle;

class SimulatedPosition : public QGeoPositionInfoSource
{
   Q_OBJECT

public:
    SimulatedPosition();

    QGeoPositionInfo lastKnownPosition(bool fromSatellitePositioningMethodsOnly = false) const override;

    PositioningMethods  supportedPositioningMethods (void) const override;
    int                 minimumUpdateInterval       (void) const override { return _updateIntervalMsecs; }
    Error               error                       (void) const override;

public slots:
    void startUpdates   (void) override;
    void stopUpdates    (void) override;
    void requestUpdate  (int timeout = 5000) override;

private slots:
    void _updatePosition                (void);
    void _vehicleAdded                  (Vehicle* vehicle);
    void _vehicleHomePositionChanged    (QGeoCoordinate homePosition);

private:
    QTimer              _updateTimer;
    QGeoPositionInfo    _lastPosition;

    static constexpr int    _updateIntervalMsecs =              1000;
    static constexpr double _horizontalVelocityMetersPerSec =   0.5;
    static constexpr double _verticalVelocityMetersPerSec =     0.1;
    static constexpr double _heading =                          45;
};
