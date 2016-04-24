/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009 - 2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

 This file is part of the QGROUNDCONTROL project

 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

 ======================================================================*/

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

    // items for simulating QGC movment in jMAVSIM

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
