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

#ifndef SENSORSCOMPONENTCONTROLLER_H
#define SENSORSCOMPONENTCONTROLLER_H

#include <QObject>
#include <QQuickItem>

#include "UASInterface.h"
#include "AutoPilotPlugin.h"

/// Sensors Component MVC Controller for SensorsComponent.qml.
class SensorsComponentController : public QObject
{
    Q_OBJECT
    
public:
    SensorsComponentController(AutoPilotPlugin* autopilot, QObject* parent = NULL);
    
    Q_PROPERTY(bool fixedWing READ fixedWing CONSTANT)
    
    Q_PROPERTY(QQuickItem* statusLog MEMBER _statusLog)
    Q_PROPERTY(QQuickItem* progressBar MEMBER _progressBar)
    
    Q_PROPERTY(QQuickItem* compassButton MEMBER _compassButton)
    Q_PROPERTY(QQuickItem* gyroButton MEMBER _gyroButton)
    Q_PROPERTY(QQuickItem* accelButton MEMBER _accelButton)
    Q_PROPERTY(QQuickItem* airspeedButton MEMBER _airspeedButton)
    
    Q_PROPERTY(bool showCompass0 MEMBER _showCompass0 CONSTANT)
    Q_PROPERTY(bool showCompass1 MEMBER _showCompass1 CONSTANT)
    Q_PROPERTY(bool showCompass2 MEMBER _showCompass2 CONSTANT)
    
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
    
    bool fixedWing(void);
    
signals:
    void showGyroCalAreaChanged(void);
    void showAccelCalAreaChanged(void);
    void gyroCalInProgressChanged(void);
    void accelCalSidesDoneChanged(void);
    void accelCalSidesInProgressChanged(void);
    
private slots:
    void _handleUASTextMessage(int uasId, int compId, int severity, QString text);
    
private:
    void _startCalibration(void);
    void _stopCalibration(bool failed);
    void _beginTextLogging(void);
    void _appendStatusLog(const QString& text);
    void _refreshParams(void);
    void _hideAllCalAreas(void);
    
    void _updateAndEmitGyroCalInProgress(bool inProgress);
    
    void _updateAndEmitShowGyroCalArea(bool show);
    void _updateAndEmitShowAccelCalArea(bool show);

    QQuickItem* _statusLog;
    QQuickItem* _progressBar;
    QQuickItem* _compassButton;
    QQuickItem* _gyroButton;
    QQuickItem* _accelButton;
    QQuickItem* _airspeedButton;
    
    bool _showGyroCalArea;
    bool _showAccelCalArea;
    
    bool _showCompass0;
    bool _showCompass1;
    bool _showCompass2;
    
    bool _gyroCalInProgress;
    
    bool _accelCalDownSideDone;
    bool _accelCalUpsideDownSideDone;
    bool _accelCalLeftSideDone;
    bool _accelCalRightSideDone;
    bool _accelCalNoseDownSideDone;
    bool _accelCalTailDownSideDone;
    
    bool _accelCalDownSideInProgress;
    bool _accelCalUpsideDownSideInProgress;
    bool _accelCalLeftSideInProgress;
    bool _accelCalRightSideInProgress;
    bool _accelCalNoseDownSideInProgress;
    bool _accelCalTailDownSideInProgress;
    
    bool _textLoggingStarted;
    
    AutoPilotPlugin*    _autopilot;
};

#endif
