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

#include "SensorsComponentController.h"
#include "QGCMAVLink.h"
#include "UASManager.h"
#include "QGCMessageBox.h"

#include <QVariant>
#include <QQmlProperty>

SensorsComponentController::SensorsComponentController(AutoPilotPlugin* autopilot, QObject* parent) :
    QObject(parent),
    _statusLog(NULL),
    _progressBar(NULL),
    _compassButton(NULL),
    _gyroButton(NULL),
    _accelButton(NULL),
    _airspeedButton(NULL),
    _cancelButton(NULL),
    _showOrientationCalArea(false),
    _gyroCalInProgress(false),
    _magCalInProgress(false),
    _accelCalInProgress(false),
    _orientationCalDownSideDone(false),
    _orientationCalUpsideDownSideDone(false),
    _orientationCalLeftSideDone(false),
    _orientationCalRightSideDone(false),
    _orientationCalNoseDownSideDone(false),
    _orientationCalTailDownSideDone(false),
    _orientationCalDownSideVisible(false),
    _orientationCalUpsideDownSideVisible(false),
    _orientationCalLeftSideVisible(false),
    _orientationCalRightSideVisible(false),
    _orientationCalNoseDownSideVisible(false),
    _orientationCalTailDownSideVisible(false),
    _orientationCalDownSideInProgress(false),
    _orientationCalUpsideDownSideInProgress(false),
    _orientationCalLeftSideInProgress(false),
    _orientationCalRightSideInProgress(false),
    _orientationCalNoseDownSideInProgress(false),
    _orientationCalTailDownSideInProgress(false),
    _orientationCalDownSideRotate(false),
    _orientationCalLeftSideRotate(false),
    _orientationCalNoseDownSideRotate(false),
    _unknownFirmwareVersion(false),
    _waitingForCancel(false),
    _autopilot(autopilot)
{
    Q_ASSERT(_autopilot);
    Q_ASSERT(_autopilot->pluginReady());
    
    _uas = _autopilot->uas();
    Q_ASSERT(_uas);
    
    // Mag rotation parameters are optional
    _showCompass0 = _autopilot->parameterExists("CAL_MAG0_ROT") &&
                        _autopilot->getParameterFact("CAL_MAG0_ROT")->value().toInt() >= 0;
    _showCompass1 = _autopilot->parameterExists("CAL_MAG1_ROT") &&
                        _autopilot->getParameterFact("CAL_MAG1_ROT")->value().toInt() >= 0;
    _showCompass2 = _autopilot->parameterExists("CAL_MAG2_ROT") &&
                        _autopilot->getParameterFact("CAL_MAG2_ROT")->value().toInt() >= 0;
}

/// Appends the specified text to the status log area in the ui
void SensorsComponentController::_appendStatusLog(const QString& text)
{
    Q_ASSERT(_statusLog);
    
    QVariant returnedValue;
    QVariant varText = text;
    QMetaObject::invokeMethod(_statusLog,
                              "append",
                              Q_RETURN_ARG(QVariant, returnedValue),
                              Q_ARG(QVariant, varText));
}

void SensorsComponentController::_startLogCalibration(void)
{
    _unknownFirmwareVersion = false;
    _hideAllCalAreas();
    
    connect(_uas, &UASInterface::textMessageReceived, this, &SensorsComponentController::_handleUASTextMessage);
    
    _cancelButton->setEnabled(false);
}

void SensorsComponentController::_startVisualCalibration(void)
{
    _compassButton->setEnabled(false);
    _gyroButton->setEnabled(false);
    _accelButton->setEnabled(false);
    _airspeedButton->setEnabled(false);
    _cancelButton->setEnabled(true);
    
    _progressBar->setProperty("value", 0);
}

void SensorsComponentController::_stopCalibration(SensorsComponentController::StopCalibrationCode code)
{
    disconnect(_uas, &UASInterface::textMessageReceived, this, &SensorsComponentController::_handleUASTextMessage);
    
    _compassButton->setEnabled(true);
    _gyroButton->setEnabled(true);
    _accelButton->setEnabled(true);
    _airspeedButton->setEnabled(true);
    _cancelButton->setEnabled(false);
    
    if (code == StopCalibrationSuccess) {
        _orientationCalDownSideDone = true;
        _orientationCalUpsideDownSideDone = true;
        _orientationCalLeftSideDone = true;
        _orientationCalRightSideDone = true;
        _orientationCalTailDownSideDone = true;
        _orientationCalNoseDownSideDone = true;
        _orientationCalDownSideInProgress = false;
        _orientationCalUpsideDownSideInProgress = false;
        _orientationCalLeftSideInProgress = false;
        _orientationCalRightSideInProgress = false;
        _orientationCalNoseDownSideInProgress = false;
        _orientationCalTailDownSideInProgress = false;
        emit orientationCalSidesDoneChanged();
        emit orientationCalSidesInProgressChanged();
        
        _progressBar->setProperty("value", 1);
    } else {
        _progressBar->setProperty("value", 0);
    }
    
    _waitingForCancel = false;
    emit waitingForCancelChanged();

    _refreshParams();
    
    switch (code) {
        case StopCalibrationSuccess:
            _orientationCalAreaHelpText->setProperty("text", "Calibration complete");
            emit resetStatusTextArea();
            if (_magCalInProgress) {
                emit setCompassRotations();
            }
            break;
            
        case StopCalibrationCancelled:
            emit resetStatusTextArea();
            _hideAllCalAreas();
            break;
            
        default:
            // Assume failed
            _hideAllCalAreas();
            QGCMessageBox::warning("Calibration", "Calibration failed. Calibration log will be displayed.");
            break;
    }
    
    _magCalInProgress = false;
    _accelCalInProgress = false;
    _gyroCalInProgress = false;
}

void SensorsComponentController::calibrateGyro(void)
{
    _startLogCalibration();
    _uas->startCalibration(UASInterface::StartCalibrationGyro);
}

void SensorsComponentController::calibrateCompass(void)
{
    _startLogCalibration();
    _uas->startCalibration(UASInterface::StartCalibrationMag);
}

void SensorsComponentController::calibrateAccel(void)
{
    _startLogCalibration();
    _uas->startCalibration(UASInterface::StartCalibrationAccel);
}

void SensorsComponentController::calibrateAirspeed(void)
{
    _startLogCalibration();
    _uas->startCalibration(UASInterface::StartCalibrationAirspeed);
}

void SensorsComponentController::_handleUASTextMessage(int uasId, int compId, int severity, QString text)
{
    Q_UNUSED(compId);
    Q_UNUSED(severity);
    
    UASInterface* uas = _autopilot->uas();
    Q_ASSERT(uas);
    if (uasId != uas->getUASID()) {
        return;
    }
    
    if (text.contains("progress <")) {
        QString percent = text.split("<").last().split(">").first();
        bool ok;
        int p = percent.toInt(&ok);
        if (ok) {
            Q_ASSERT(_progressBar);
            _progressBar->setProperty("value", (float)(p / 100.0));
        }
        return;
    }

    _appendStatusLog(text);
    
    if (_unknownFirmwareVersion) {
        // We don't know how to do visual cal with the version of firwmare
        return;
    }
    
    // All calibration messages start with [cal]
    QString calPrefix("[cal] ");
    if (!text.startsWith(calPrefix)) {
        return;
    }
    text = text.right(text.length() - calPrefix.length());

    QString calStartPrefix("calibration started: ");
    if (text.startsWith(calStartPrefix)) {
        text = text.right(text.length() - calStartPrefix.length());
        
        // Split version number and cal type
        QStringList parts = text.split(" ");
        if (parts.count() != 2 && parts[0].toInt() != 1) {
            _unknownFirmwareVersion = true;
            qDebug() << "Unknown cal firmware version, using log";
            return;
        }
        
        _startVisualCalibration();
        
        text = parts[1];
        if (text == "accel" || text == "mag" || text == "gyro") {
            // Reset all progress indication
            _orientationCalDownSideDone = false;
            _orientationCalUpsideDownSideDone = false;
            _orientationCalLeftSideDone = false;
            _orientationCalRightSideDone = false;
            _orientationCalTailDownSideDone = false;
            _orientationCalNoseDownSideDone = false;
            _orientationCalDownSideInProgress = false;
            _orientationCalUpsideDownSideInProgress = false;
            _orientationCalLeftSideInProgress = false;
            _orientationCalRightSideInProgress = false;
            _orientationCalNoseDownSideInProgress = false;
            _orientationCalTailDownSideInProgress = false;
            
            // Reset all visibility
            _orientationCalDownSideVisible = false;
            _orientationCalUpsideDownSideVisible = false;
            _orientationCalLeftSideVisible = false;
            _orientationCalRightSideVisible = false;
            _orientationCalTailDownSideVisible = false;
            _orientationCalNoseDownSideVisible = false;
            
            _orientationCalAreaHelpText->setProperty("text", "Place your vehicle into one of the Incomplete orientations shown below and hold it still");
            
            if (text == "accel") {
                _accelCalInProgress = true;
                _orientationCalDownSideVisible = true;
                _orientationCalUpsideDownSideVisible = true;
                _orientationCalLeftSideVisible = true;
                _orientationCalRightSideVisible = true;
                _orientationCalTailDownSideVisible = true;
                _orientationCalNoseDownSideVisible = true;
            } else if (text == "mag") {
                _magCalInProgress = true;
                _orientationCalDownSideVisible = true;
                _orientationCalLeftSideVisible = true;
                _orientationCalNoseDownSideVisible = true;
            } else if (text == "gyro") {
                _gyroCalInProgress = true;
                _orientationCalDownSideVisible = true;
            } else {
                Q_ASSERT(false);
            }
            emit orientationCalSidesDoneChanged();
            emit orientationCalSidesVisibleChanged();
            emit orientationCalSidesInProgressChanged();
            _updateAndEmitShowOrientationCalArea(true);
        }
        return;
    }
    
    if (text.endsWith("orientation detected")) {
        QString side = text.section(" ", 0, 0);
        qDebug() << "Side started" << side;
        
        if (side == "down") {
            _orientationCalDownSideInProgress = true;
            if (_magCalInProgress) {
                _orientationCalDownSideRotate = true;
            }
        } else if (side == "up") {
            _orientationCalUpsideDownSideInProgress = true;
        } else if (side == "left") {
            _orientationCalLeftSideInProgress = true;
            if (_magCalInProgress) {
                _orientationCalLeftSideRotate = true;
            }
        } else if (side == "right") {
            _orientationCalRightSideInProgress = true;
        } else if (side == "front") {
            _orientationCalNoseDownSideInProgress = true;
            if (_magCalInProgress) {
                _orientationCalNoseDownSideRotate = true;
            }
        } else if (side == "back") {
            _orientationCalTailDownSideInProgress = true;
        }
        
        if (_magCalInProgress) {
            _orientationCalAreaHelpText->setProperty("text", "Rotate the vehicle continuously as shown in the diagram until marked as Completed");
        } else {
            _orientationCalAreaHelpText->setProperty("text", "Hold still in the current orientation");
        }
        
        emit orientationCalSidesInProgressChanged();
        emit orientationCalSidesRotateChanged();
        return;
    }
    
    if (text.endsWith("side done, rotate to a different side")) {
        QString side = text.section(" ", 0, 0);
        qDebug() << "Side finished" << side;
        
        if (side == "down") {
            _orientationCalDownSideInProgress = false;
            _orientationCalDownSideDone = true;
            _orientationCalDownSideRotate = false;
        } else if (side == "up") {
            _orientationCalUpsideDownSideInProgress = false;
            _orientationCalUpsideDownSideDone = true;
        } else if (side == "left") {
            _orientationCalLeftSideInProgress = false;
            _orientationCalLeftSideDone = true;
            _orientationCalLeftSideRotate = false;
        } else if (side == "right") {
            _orientationCalRightSideInProgress = false;
            _orientationCalRightSideDone = true;
        } else if (side == "front") {
            _orientationCalNoseDownSideInProgress = false;
            _orientationCalNoseDownSideDone = true;
            _orientationCalNoseDownSideRotate = false;
        } else if (side == "back") {
            _orientationCalTailDownSideInProgress = false;
            _orientationCalTailDownSideDone = true;
        }
        
        _orientationCalAreaHelpText->setProperty("text", "Place you vehicle into one of the orientations shown below and hold it still");

        emit orientationCalSidesInProgressChanged();
        emit orientationCalSidesDoneChanged();
        emit orientationCalSidesRotateChanged();
        return;
    }
    
    QString calCompletePrefix("calibration done:");
    if (text.startsWith(calCompletePrefix)) {
        _stopCalibration(StopCalibrationSuccess);
        return;
    }
    
    if (text.startsWith("calibration cancelled")) {
        _stopCalibration(_waitingForCancel ? StopCalibrationCancelled : StopCalibrationFailed);
        return;
    }
    
    if (text.startsWith("calibration failed")) {
        _stopCalibration(StopCalibrationFailed);
        return;
    }
}

void SensorsComponentController::_refreshParams(void)
{
    _autopilot->refreshParametersPrefix(FactSystem::defaultComponentId, "CAL_");
    _autopilot->refreshParametersPrefix(FactSystem::defaultComponentId, "SENS_");
}

bool SensorsComponentController::fixedWing(void)
{
    UASInterface* uas = _autopilot->uas();
    Q_ASSERT(uas);
    return uas->getSystemType() == MAV_TYPE_FIXED_WING;
}

void SensorsComponentController::_updateAndEmitShowOrientationCalArea(bool show)
{
    _showOrientationCalArea = show;
    emit showOrientationCalAreaChanged();
}

void SensorsComponentController::_hideAllCalAreas(void)
{
    _updateAndEmitShowOrientationCalArea(false);
}

void SensorsComponentController::cancelCalibration(void)
{
    // The firmware doesn't allow us to cancel calibration. The best we can do is wait
    // for it to timeout.
    _waitingForCancel = true;
    emit waitingForCancelChanged();
    _cancelButton->setEnabled(false);
    _uas->stopCalibration();
}