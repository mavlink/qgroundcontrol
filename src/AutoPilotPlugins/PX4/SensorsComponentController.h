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
    Q_PROPERTY(bool showOrientationCalArea MEMBER _showOrientationCalArea NOTIFY showOrientationCalAreaChanged)
    
    Q_PROPERTY(bool gyroCalInProgress MEMBER _gyroCalInProgress NOTIFY gyroCalInProgressChanged)
    
    Q_PROPERTY(QString calInProgressText MEMBER _calInProgressText NOTIFY calInProgressTextChanged)
    
    Q_PROPERTY(bool orientationCalDownSideDone MEMBER _orientationCalDownSideDone NOTIFY orientationCalSidesDoneChanged)
    Q_PROPERTY(bool orientationCalUpsideDownSideDone MEMBER _orientationCalUpsideDownSideDone NOTIFY orientationCalSidesDoneChanged)
    Q_PROPERTY(bool orientationCalLeftSideDone MEMBER _orientationCalLeftSideDone NOTIFY orientationCalSidesDoneChanged)
    Q_PROPERTY(bool orientationCalRightSideDone MEMBER _orientationCalRightSideDone NOTIFY orientationCalSidesDoneChanged)
    Q_PROPERTY(bool orientationCalNoseDownSideDone MEMBER _orientationCalNoseDownSideDone NOTIFY orientationCalSidesDoneChanged)
    Q_PROPERTY(bool orientationCalTailDownSideDone MEMBER _orientationCalTailDownSideDone NOTIFY orientationCalSidesDoneChanged)
    
    Q_PROPERTY(bool orientationCalDownSideInProgress MEMBER _orientationCalDownSideInProgress NOTIFY orientationCalSidesInProgressChanged)
    Q_PROPERTY(bool orientationCalUpsideDownSideInProgress MEMBER _orientationCalUpsideDownSideInProgress NOTIFY orientationCalSidesInProgressChanged)
    Q_PROPERTY(bool orientationCalLeftSideInProgress MEMBER _orientationCalLeftSideInProgress NOTIFY orientationCalSidesInProgressChanged)
    Q_PROPERTY(bool orientationCalRightSideInProgress MEMBER _orientationCalRightSideInProgress NOTIFY orientationCalSidesInProgressChanged)
    Q_PROPERTY(bool orientationCalNoseDownSideInProgress MEMBER _orientationCalNoseDownSideInProgress NOTIFY orientationCalSidesInProgressChanged)
    Q_PROPERTY(bool orientationCalTailDownSideInProgress MEMBER _orientationCalTailDownSideInProgress NOTIFY orientationCalSidesInProgressChanged)
    
    Q_INVOKABLE void calibrateCompass(void);
    Q_INVOKABLE void calibrateGyro(void);
    Q_INVOKABLE void calibrateAccel(void);
    Q_INVOKABLE void calibrateAirspeed(void);
    
    bool fixedWing(void);
    
signals:
    void showGyroCalAreaChanged(void);
    void showOrientationCalAreaChanged(void);
    void gyroCalInProgressChanged(void);
    void orientationCalSidesDoneChanged(void);
    void orientationCalSidesInProgressChanged(void);
    void calInProgressTextChanged(const QString& newText);
    
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
    
    void _updateAndEmitCalInProgressText(const QString& text);
    
    void _updateAndEmitShowGyroCalArea(bool show);
    void _updateAndEmitShowOrientationCalArea(bool show);

    QQuickItem* _statusLog;
    QQuickItem* _progressBar;
    QQuickItem* _compassButton;
    QQuickItem* _gyroButton;
    QQuickItem* _accelButton;
    QQuickItem* _airspeedButton;
    
    bool _showGyroCalArea;
    bool _showOrientationCalArea;
    
    bool _showCompass0;
    bool _showCompass1;
    bool _showCompass2;
    
    bool _gyroCalInProgress;
    bool _magCalInProgress;
    bool _accelCalInProgress;
    
    QString _calInProgressText;
    
    bool _orientationCalDownSideDone;
    bool _orientationCalUpsideDownSideDone;
    bool _orientationCalLeftSideDone;
    bool _orientationCalRightSideDone;
    bool _orientationCalNoseDownSideDone;
    bool _orientationCalTailDownSideDone;
    
    bool _orientationCalDownSideInProgress;
    bool _orientationCalUpsideDownSideInProgress;
    bool _orientationCalLeftSideInProgress;
    bool _orientationCalRightSideInProgress;
    bool _orientationCalNoseDownSideInProgress;
    bool _orientationCalTailDownSideInProgress;
    
    bool _textLoggingStarted;
    
    AutoPilotPlugin*    _autopilot;
};

#endif
