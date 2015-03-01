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
    _showAccelCalArea(false),
    _gyroCalInProgress(false),
    _accelCalDownSideDone(false),
    _accelCalUpsideDownSideDone(false),
    _accelCalLeftSideDone(false),
    _accelCalRightSideDone(false),
    _accelCalNoseDownSideDone(false),
    _accelCalTailDownSideDone(false),
    _accelCalDownSideInProgress(false),
    _accelCalUpsideDownSideInProgress(false),
    _accelCalLeftSideInProgress(false),
    _accelCalRightSideInProgress(false),
    _accelCalNoseDownSideInProgress(false),
    _accelCalTailDownSideInProgress(false),
    _autopilot(autopilot)
{
    Q_ASSERT(autopilot);
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

void SensorsComponentController::calibrateGyro(void)
{
    _beginTextLogging();
    
    UASInterface* uas = _autopilot->uas();
    Q_ASSERT(uas);
    uas->executeCommand(MAV_CMD_PREFLIGHT_CALIBRATION, 1, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0);
}

void SensorsComponentController::calibrateCompass(void)
{
    _hideAllCalAreas();
    _beginTextLogging();

    UASInterface* uas = _autopilot->uas();
    Q_ASSERT(uas);
    uas->executeCommand(MAV_CMD_PREFLIGHT_CALIBRATION, 1, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0);
}

void SensorsComponentController::calibrateAccel(void)
{
    _beginTextLogging();

    UASInterface* uas = _autopilot->uas();
    Q_ASSERT(uas);
    uas->executeCommand(MAV_CMD_PREFLIGHT_CALIBRATION, 1, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0);
}

void SensorsComponentController::calibrateAirspeed(void)
{
    _hideAllCalAreas();
    _beginTextLogging();

    UASInterface* uas = _autopilot->uas();
    Q_ASSERT(uas);
    uas->executeCommand(MAV_CMD_PREFLIGHT_CALIBRATION, 1, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0);
}

void SensorsComponentController::_beginTextLogging(void)
{
    UASInterface* uas = _autopilot->uas();
    Q_ASSERT(uas);
    connect(uas, &UASInterface::textMessageReceived, this, &SensorsComponentController::_handleUASTextMessage);
}

void SensorsComponentController::_handleUASTextMessage(int uasId, int compId, int severity, QString text)
{
    QString startingSidePrefix("Hold still, starting to measure ");
    QString sideDoneSuffix(" side done, rotate to a different side");
    
    Q_UNUSED(compId);
    Q_UNUSED(severity);
    
    UASInterface* uas = _autopilot->uas();
    Q_ASSERT(uas);
    if (uasId != uas->getUASID()) {
        return;
    }
    
    QStringList ignorePrefixList;
    ignorePrefixList << "[cmd]" << "[mavlink pm]" << "[ekf check]" << "[pm]" << "[inav]";
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
        _hideAllCalAreas();
        _updateAndEmitShowGyroCalArea(true);
        _updateAndEmitGyroCalInProgress(true);
    } else if (text == "accel calibration: started") {
        _hideAllCalAreas();
        _accelCalDownSideDone = false;
        _accelCalUpsideDownSideDone = false;
        _accelCalLeftSideDone = false;
        _accelCalRightSideDone = false;
        _accelCalTailDownSideDone = false;
        _accelCalNoseDownSideDone = false;
        _accelCalDownSideInProgress = false;
        _accelCalUpsideDownSideInProgress = false;
        _accelCalLeftSideInProgress = false;
        _accelCalRightSideInProgress = false;
        _accelCalNoseDownSideInProgress = false;
        _accelCalTailDownSideInProgress = false;
        emit accelCalSidesDoneChanged();
        emit accelCalSidesInProgressChanged();
        _updateAndEmitShowAccelCalArea(true);
    } else if (text.startsWith(startingSidePrefix)) {
        QString side = text.right(text.length() - startingSidePrefix.length()).section(" ", 0, 0);
        qDebug() << "Side started" << side;
        if (side == "down") {
            _accelCalDownSideInProgress = true;
        } else if (side == "up") {
            _accelCalUpsideDownSideInProgress = true;
        } else if (side == "left") {
            _accelCalLeftSideInProgress = true;
        } else if (side == "right") {
            _accelCalRightSideInProgress = true;
        } else if (side == "front") {
            _accelCalNoseDownSideInProgress = true;
        } else if (side == "back") {
            _accelCalTailDownSideInProgress = true;
        }
        emit accelCalSidesInProgressChanged();
    } else if (text.endsWith(sideDoneSuffix)) {
        QString side = text.section(" ", 0, 0);
        qDebug() << "Side finished" << side;
        if (side == "down") {
            _accelCalDownSideInProgress = false;
            _accelCalDownSideDone = true;
        } else if (side == "up") {
            _accelCalUpsideDownSideInProgress = false;
            _accelCalUpsideDownSideDone = true;
        } else if (side == "left") {
            _accelCalLeftSideInProgress = false;
            _accelCalLeftSideDone = true;
        } else if (side == "right") {
            _accelCalRightSideInProgress = false;
            _accelCalRightSideDone = true;
        } else if (side == "front") {
            _accelCalNoseDownSideInProgress = false;
            _accelCalNoseDownSideDone = true;
        } else if (side == "back") {
            _accelCalTailDownSideInProgress = false;
            _accelCalTailDownSideDone = true;
        }
        emit accelCalSidesInProgressChanged();
        emit accelCalSidesDoneChanged();
    } else if (text == "accel calibration: done") {
        _progressBar->setProperty("value", 1);
        _accelCalDownSideDone = true;
        _accelCalUpsideDownSideDone = true;
        _accelCalLeftSideDone = true;
        _accelCalRightSideDone = true;
        _accelCalTailDownSideDone = true;
        _accelCalNoseDownSideDone = true;
        _accelCalDownSideInProgress = false;
        _accelCalUpsideDownSideInProgress = false;
        _accelCalLeftSideInProgress = false;
        _accelCalRightSideInProgress = false;
        _accelCalNoseDownSideInProgress = false;
        _accelCalTailDownSideInProgress = false;
        emit accelCalSidesDoneChanged();
        emit accelCalSidesInProgressChanged();
        _refreshParams();
    } else if (text == "gyro calibration: done") {
        _progressBar->setProperty("value", 1);
        _updateAndEmitGyroCalInProgress(false);
        _refreshParams();
    } else if (text.endsWith(" calibration: failed")) {
        QGCMessageBox::warning("Calibration", "Calibration failed. Calibration log will be displayed.");
        _hideAllCalAreas();
        _progressBar->setProperty("value", 0);
        _updateAndEmitGyroCalInProgress(false);
        _refreshParams();
    }
    
#if 0
    if (text.startsWith("Hold still, starting to measure ")) {
        QString axis = text.section(" ", -2, -2);
        setInstructionImage(QString(":/files/images/px4/calibration/accel_%1.png").arg(axis));
    }
    
    if (text.startsWith("pending: ")) {
        QString axis = text.section(" ", 1, 1);
        setInstructionImage(QString(":/files/images/px4/calibration/accel_%1.png").arg(axis));
    }
    
    if (text == "rotate in a figure 8 around all axis" /* support for old typo */
        || text == "rotate in a figure 8 around all axes" /* current version */) {
        setInstructionImage(":/files/images/px4/calibration/mag_calibration_figure8.png");
    }
    
    if (text.endsWith(" calibration: done") || text.endsWith(" calibration: failed")) {
        // XXX use a confirmation image or something
        setInstructionImage(":/files/images/px4/calibration/accel_down.png");
        if (text.endsWith(" calibration: done")) {
            ui->progressBar->setValue(100);
        } else {
            ui->progressBar->setValue(0);
        }
        
        if (activeUAS) {
            _requestAllSensorParameters();
        }
    }
    
    if (text.endsWith(" calibration: started")) {
        setInstructionImage(":/files/images/px4/calibration/accel_down.png");
    }
#endif    
}

void SensorsComponentController::_refreshParams(void)
{
#if 0
    // FIXME: Not sure if firmware issue yet
    _autopilot->refreshParametersPrefix("CAL_");
    _autopilot->refreshParametersPrefix("SENS_");
#else
    // Sending too many parameter requests like above doesn't seem to work. So for now,
    // ask for everything back
    _autopilot->refreshAllParameters();
#endif
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

void SensorsComponentController::_updateAndEmitShowAccelCalArea(bool show)
{
    _showAccelCalArea = show;
    emit showAccelCalAreaChanged();
}

void SensorsComponentController::_hideAllCalAreas(void)
{
    _updateAndEmitShowGyroCalArea(false);
    _updateAndEmitShowAccelCalArea(false);
}
