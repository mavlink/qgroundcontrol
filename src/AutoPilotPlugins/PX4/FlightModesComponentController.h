/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
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

/// @file
///     @author Don Gagne <don@thegagnes.com>

#ifndef FLIGHTMODESCOMPONENTCONTROLLER_H
#define FLIGHTMODESCOMPONENTCONTROLLER_H

#include <QObject>
#include <QQuickItem>

#include "UASInterface.h"
#include "AutoPilotPlugin.h"

/// MVC Controller for FlightModesComponent.qml.
class FlightModesComponentController : public QObject
{
    Q_OBJECT
    
public:
    FlightModesComponentController(QObject* parent = NULL);
    
#if 0
    Q_PROPERTY(bool fixedWing READ fixedWing CONSTANT)
    
    Q_PROPERTY(QQuickItem* statusLog MEMBER _statusLog)
    Q_PROPERTY(QQuickItem* progressBar MEMBER _progressBar)
    
    Q_PROPERTY(bool showGyroCalArea MEMBER _showGyroCalArea NOTIFY showGyroCalAreaChanged)
    Q_PROPERTY(bool showAccelCalArea MEMBER _showAccelCalArea NOTIFY showAccelCalAreaChanged)
    
    Q_PROPERTY(bool gyroCalInProgress MEMBER _gyroCalInProgress NOTIFY gyroCalInProgressChanged)
    
    Q_PROPERTY(bool accelCalDownSideDone MEMBER _accelCalDownSideDone NOTIFY accelCalSidesDoneChanged)
    Q_PROPERTY(bool accelCalUpsideDownSideDone MEMBER _accelCalUpsideDownSideDone NOTIFY accelCalSidesDoneChanged)
    Q_PROPERTY(bool accelCalLeftSideDone MEMBER _accelCalLeftSideDone NOTIFY accelCalSidesDoneChanged)
    Q_PROPERTY(bool accelCalRightSideDone MEMBER _accelCalRightSideDone NOTIFY accelCalSidesDoneChanged)
    Q_PROPERTY(bool accelCalNoseDownSideDone MEMBER _accelCalNoseDownSideDone NOTIFY accelCalSidesDoneChanged)
    Q_PROPERTY(bool accelCalTailDownSideDone MEMBER _accelCalTailDownSideDone NOTIFY accelCalSidesDoneChanged)
    
    Q_PROPERTY(bool accelCalDownSideInProgress MEMBER _accelCalDownSideInProgress NOTIFY accelCalSidesInProgressChanged)
    Q_PROPERTY(bool accelCalUpsideDownSideInProgress MEMBER _accelCalUpsideDownSideInProgress NOTIFY accelCalSidesInProgressChanged)
    Q_PROPERTY(bool accelCalLeftSideInProgress MEMBER _accelCalLeftSideInProgress NOTIFY accelCalSidesInProgressChanged)
    Q_PROPERTY(bool accelCalRightSideInProgress MEMBER _accelCalRightSideInProgress NOTIFY accelCalSidesInProgressChanged)
    Q_PROPERTY(bool accelCalNoseDownSideInProgress MEMBER _accelCalNoseDownSideInProgress NOTIFY accelCalSidesInProgressChanged)
    Q_PROPERTY(bool accelCalTailDownSideInProgress MEMBER _accelCalTailDownSideInProgress NOTIFY accelCalSidesInProgressChanged)
    
    Q_INVOKABLE void calibrateCompass(void);
    Q_INVOKABLE void calibrateGyro(void);
    Q_INVOKABLE void calibrateAccel(void);
    Q_INVOKABLE void calibrateAirspeed(void);
#endif
    
signals:
    
private:
    UASInterface*   _uas;
};

#endif
