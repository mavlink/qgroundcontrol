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

#include <QVariant>
#include <QQmlProperty>

SensorsComponentController::SensorsComponentController(AutoPilotPlugin* autopilot, QObject* parent) :
    QObject(parent),
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
    Q_UNUSED(compId);
    Q_UNUSED(severity);
    
    UASInterface* uas = _autopilot->uas();
    Q_ASSERT(uas);
    if (uasId != uas->getUASID()) {
        return;
    }
    
    QStringList ignorePrefixList;
    ignorePrefixList << "[cmd]" << "[mavlink pm]" << "[ekf check]";
    foreach (QString ignorePrefix, ignorePrefixList) {
        if (text.startsWith(ignorePrefix)) {
            return;
        }
    }
    
    _appendStatusLog(text);
    
    if (text.endsWith(" calibration: done") || text.endsWith(" calibration: failed")) {
        _refreshParams();
    }
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
