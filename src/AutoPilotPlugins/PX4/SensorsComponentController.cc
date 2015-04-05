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
    _showGyroCalArea(false),
    _showOrientationCalArea(false),
    _showCompass0(false),
    _showCompass1(false),
    _showCompass2(false),
    _gyroCalInProgress(false),
    _magCalInProgress(false),
    _accelCalInProgress(false),
    _orientationCalDownSideDone(false),
    _orientationCalUpsideDownSideDone(false),
    _orientationCalLeftSideDone(false),
    _orientationCalRightSideDone(false),
    _orientationCalNoseDownSideDone(false),
    _orientationCalTailDownSideDone(false),
    _orientationCalDownSideInProgress(false),
    _orientationCalUpsideDownSideInProgress(false),
    _orientationCalLeftSideInProgress(false),
    _orientationCalRightSideInProgress(false),
    _orientationCalNoseDownSideInProgress(false),
    _orientationCalTailDownSideInProgress(false),
    _textLoggingStarted(false),
    _autopilot(autopilot)
{
    Q_ASSERT(_autopilot);
    Q_ASSERT(_autopilot->pluginReady());
    
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

void SensorsComponentController::_startCalibration(void)
{
    _beginTextLogging();
    _hideAllCalAreas();
    
    _compassButton->setEnabled(false);
    _gyroButton->setEnabled(false);
    _accelButton->setEnabled(false);
    _airspeedButton->setEnabled(false);
}

void SensorsComponentController::_stopCalibration(bool failed)
{
    _magCalInProgress = false;
    _accelCalInProgress = false;
    
    _compassButton->setEnabled(true);
    _gyroButton->setEnabled(true);
    _accelButton->setEnabled(true);
    _airspeedButton->setEnabled(true);
    
    _progressBar->setProperty("value", failed ? 0 : 1);
    _updateAndEmitGyroCalInProgress(false);
    _refreshParams();
    
    if (failed) {
        QGCMessageBox::warning("Calibration", "Calibration failed. Calibration log will be displayed.");
        _hideAllCalAreas();
    }
}

void SensorsComponentController::calibrateGyro(void)
{
    _startCalibration();
    
    UASInterface* uas = _autopilot->uas();
    Q_ASSERT(uas);
    uas->executeCommand(MAV_CMD_PREFLIGHT_CALIBRATION, 1, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0);
}

void SensorsComponentController::calibrateCompass(void)
{
    _startCalibration();

    UASInterface* uas = _autopilot->uas();
    Q_ASSERT(uas);
    uas->executeCommand(MAV_CMD_PREFLIGHT_CALIBRATION, 1, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0);
}

void SensorsComponentController::calibrateAccel(void)
{
    _startCalibration();

    UASInterface* uas = _autopilot->uas();
    Q_ASSERT(uas);
    uas->executeCommand(MAV_CMD_PREFLIGHT_CALIBRATION, 1, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0);
}

void SensorsComponentController::calibrateAirspeed(void)
{
    _startCalibration();

    UASInterface* uas = _autopilot->uas();
    Q_ASSERT(uas);
    uas->executeCommand(MAV_CMD_PREFLIGHT_CALIBRATION, 1, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0);
}

void SensorsComponentController::_beginTextLogging(void)
{
    if (!_textLoggingStarted) {
        _textLoggingStarted = true;
        UASInterface* uas = _autopilot->uas();
        Q_ASSERT(uas);
        connect(uas, &UASInterface::textMessageReceived, this, &SensorsComponentController::_handleUASTextMessage);
    }
}

void SensorsComponentController::_handleUASTextMessage(int uasId, int compId, int severity, QString text)
{
    QString startingSidePrefix("Hold still, starting to measure ");
    QString sideDoneSuffix(" side done, rotate to a different side");
    QString orientationDetectedSuffix(" orientation detected");
    
    Q_UNUSED(compId);
    Q_UNUSED(severity);
    
    UASInterface* uas = _autopilot->uas();
    Q_ASSERT(uas);
    if (uasId != uas->getUASID()) {
        return;
    }
    
    QStringList ignorePrefixList;
    ignorePrefixList << "[cmd]" << "[mavlink pm]" << "[ekf check]" << "[pm]" << "[inav]" << "IN AIR MODE" << "LANDED MODE";
    foreach (QString ignorePrefix, ignorePrefixList) {
        if (text.startsWith(ignorePrefix)) {
            return;
        }
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

    if (text == "gyro calibration: started") {
        _updateAndEmitShowGyroCalArea(true);
        _updateAndEmitGyroCalInProgress(true);
    } else if (text == "accel calibration: started" || text == "mag calibration: started") {
        if (text == "accel calibration: started") {
            _accelCalInProgress = true;
            _updateAndEmitCalInProgressText("Hold Still");
        } else {
            _updateAndEmitCalInProgressText("Rotate");
            _magCalInProgress = true;
        }
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
        emit orientationCalSidesDoneChanged();
        emit orientationCalSidesInProgressChanged();
        _updateAndEmitShowOrientationCalArea(true);
    } else if (text.startsWith(startingSidePrefix)) {
        QString side = text.right(text.length() - startingSidePrefix.length()).section(" ", 0, 0);
        qDebug() << "Side started" << side;
        if (side == "down") {
            _orientationCalDownSideInProgress = true;
        } else if (side == "up") {
            _orientationCalUpsideDownSideInProgress = true;
        } else if (side == "left") {
            _orientationCalLeftSideInProgress = true;
        } else if (side == "right") {
            _orientationCalRightSideInProgress = true;
        } else if (side == "front") {
            _orientationCalNoseDownSideInProgress = true;
        } else if (side == "back") {
            _orientationCalTailDownSideInProgress = true;
        }
        emit orientationCalSidesInProgressChanged();
    } else if (text.endsWith(orientationDetectedSuffix)) {
        QString side = text.section(" ", 0, 0);
        qDebug() << "Side started" << side;
        if (side == "down") {
            _orientationCalDownSideInProgress = true;
        } else if (side == "up") {
            _orientationCalUpsideDownSideInProgress = true;
        } else if (side == "left") {
            _orientationCalLeftSideInProgress = true;
        } else if (side == "right") {
            _orientationCalRightSideInProgress = true;
        } else if (side == "front") {
            _orientationCalNoseDownSideInProgress = true;
        } else if (side == "back") {
            _orientationCalTailDownSideInProgress = true;
        }
        emit orientationCalSidesInProgressChanged();
    } else if (text.endsWith(sideDoneSuffix)) {
        QString side = text.section(" ", 0, 0);
        qDebug() << "Side finished" << side;
        if (side == "down") {
            _orientationCalDownSideInProgress = false;
            _orientationCalDownSideDone = true;
        } else if (side == "up") {
            _orientationCalUpsideDownSideInProgress = false;
            _orientationCalUpsideDownSideDone = true;
        } else if (side == "left") {
            _orientationCalLeftSideInProgress = false;
            _orientationCalLeftSideDone = true;
        } else if (side == "right") {
            _orientationCalRightSideInProgress = false;
            _orientationCalRightSideDone = true;
        } else if (side == "front") {
            _orientationCalNoseDownSideInProgress = false;
            _orientationCalNoseDownSideDone = true;
        } else if (side == "back") {
            _orientationCalTailDownSideInProgress = false;
            _orientationCalTailDownSideDone = true;
        }
        emit orientationCalSidesInProgressChanged();
        emit orientationCalSidesDoneChanged();
    } else if (text == "accel calibration: done" || text == "mag calibration: done") {
        _progressBar->setProperty("value", 1);
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
        _stopCalibration(false /* success */);
    } else if (text == "gyro calibration: done") {
        _stopCalibration(false /* success */);
    } else if (text == "dpress calibration: done") {
        _stopCalibration(false /* success */);
    } else if (text.endsWith(" calibration: failed")) {
        _stopCalibration(true /* failed */);
    }
}

void SensorsComponentController::_refreshParams(void)
{
    // Pull specified params first so red/green indicators update quickly
    _autopilot->refreshParameter(FactSystem::defaultComponentId, "CAL_MAG0_ID");
    _autopilot->refreshParameter(FactSystem::defaultComponentId, "CAL_GYRO0_ID");
    _autopilot->refreshParameter(FactSystem::defaultComponentId, "CAL_ACC0_ID");
    _autopilot->refreshParameter(FactSystem::defaultComponentId, "SENS_DPRES_OFF");
    
    _autopilot->refreshParameter(FactSystem::defaultComponentId, "SENS_BOARD_ROT");
    
    // Mag rotation parameters are optional
    if (_autopilot->parameterExists("CAL_MAG0_ROT")) {
        _autopilot->refreshParameter(FactSystem::defaultComponentId, "CAL_MAG0_ROT");
    }
    if (_autopilot->parameterExists("CAL_MAG1_ROT")) {
        _autopilot->refreshParameter(FactSystem::defaultComponentId, "CAL_MAG1_ROT");
    }
    if (_autopilot->parameterExists("CAL_MAG2_ROT")) {
        _autopilot->refreshParameter(FactSystem::defaultComponentId, "CAL_MAG2_ROT");
    }
    
    // Pull full set in order to get all cal values back
    _autopilot->refreshAllParameters();
}

bool SensorsComponentController::fixedWing(void)
{
    UASInterface* uas = _autopilot->uas();
    Q_ASSERT(uas);
    return uas->getSystemType() == MAV_TYPE_FIXED_WING;
}

void SensorsComponentController::_updateAndEmitGyroCalInProgress(bool inProgress)
{
    _gyroCalInProgress = inProgress;
    emit gyroCalInProgressChanged();
}

void SensorsComponentController::_updateAndEmitShowGyroCalArea(bool show)
{
    _showGyroCalArea = show;
    emit showGyroCalAreaChanged();
}

void SensorsComponentController::_updateAndEmitShowOrientationCalArea(bool show)
{
    _showOrientationCalArea = show;
    emit showOrientationCalAreaChanged();
}

void SensorsComponentController::_hideAllCalAreas(void)
{
    _updateAndEmitShowGyroCalArea(false);
    _updateAndEmitShowOrientationCalArea(false);
}

void SensorsComponentController::_updateAndEmitCalInProgressText(const QString& text)
{
    _calInProgressText = text;
    emit calInProgressTextChanged(text);
}
