/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#pragma once

#include <QtPositioning/qgeopositioninfosource.h>
#include "QGCToolbox.h"
#include <QTimer>

class SimulatedPosition : public QGeoPositionInfoSource
{
   Q_OBJECT

public:
    SimulatedPosition();

    QGeoPositionInfo lastKnownPosition(bool fromSatellitePositioningMethodsOnly = false) const;

    PositioningMethods supportedPositioningMethods() const;
    int minimumUpdateInterval() const;
    Error error() const;

public slots:
    virtual void startUpdates();
    virtual void stopUpdates();

    virtual void requestUpdate(int timeout = 5000);

private slots:
    void updatePosition();

private:
    QTimer update_timer;

    QGeoPositionInfo lastPosition;

    // items for simulating QGC movement in jMAVSIM

    int32_t lat_int;
    int32_t lon_int;

    struct simulated_motion_s {
        int lon;
        int lat;
    };

    static simulated_motion_s _simulated_motion[5];

    int getRandomNumber(int size);
    int _step_cnt;
    int _simulate_motion_index;

    bool _simulate_motion;
    float _rotation;
};
