/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
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

#include "APMSensorsComponent.h"
#include "APMAutoPilotPlugin.h"
#include "APMSensorsComponentController.h"

// These two list must be kept in sync

APMSensorsComponent::APMSensorsComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent) :
    APMComponent(vehicle, autopilot, parent),
    _name(tr("Sensors"))
{

}

QString APMSensorsComponent::name(void) const
{
    return _name;
}

QString APMSensorsComponent::description(void) const
{
    return tr("The Sensors Component allows you to calibrate the sensors within your vehicle. "
              "Prior to flight you must calibrate the Magnetometer, Gyroscope and Accelerometer.");
}

QString APMSensorsComponent::iconResource(void) const
{
    return "/qmlimages/SensorsComponentIcon.png";
}

bool APMSensorsComponent::requiresSetup(void) const
{
    return true;
}

bool APMSensorsComponent::setupComplete(void) const
{
    return !compassSetupNeeded() && !accelSetupNeeded();
}

QString APMSensorsComponent::setupStateDescription(void) const
{
    const char* stateDescription;
    
    if (requiresSetup()) {
        stateDescription = "Requires calibration";
    } else {
        stateDescription = "Calibrated";
    }
    return QString(stateDescription);
}

QStringList APMSensorsComponent::setupCompleteChangedTriggerList(void) const
{
    QStringList triggers;
    
    // Compass triggers
    triggers << "COMPASS_DEV_ID" << "COMPASS_DEV_ID2" << "COMPASS_DEV_ID3"
             << "COMPASS_OFS_X" << "COMPASS_OFS_X" << "COMPASS_OFS_X"
             << "COMPASS_OFS2_X" << "COMPASS_OFS2_X" << "COMPASS_OFS2_X"
             << "COMPASS_OFS3_X" << "COMPASS_OFS3_X" << "COMPASS_OFS3_X";

    // Acceleromter triggers
    triggers << "INS_USE" << "INS_USE2" << "INS_USE3"
             << "INS_ACCOFFS_X" << "INS_ACCOFFS_Y" << "INS_ACCOFFS_Z"
             << "INS_ACC2OFFS_X" << "INS_ACC2OFFS_Y" << "INS_ACC2OFFS_Z"
             << "INS_ACC3OFFS_X" << "INS_ACC3OFFS_Y" << "INS_ACC3OFFS_Z";

    return triggers;
}

QStringList APMSensorsComponent::paramFilterList(void) const
{
    return QStringList();
}

QUrl APMSensorsComponent::setupSource(void) const
{
    return QUrl::fromUserInput("qrc:/qml/APMSensorsComponent.qml");
}

QUrl APMSensorsComponent::summaryQmlSource(void) const
{
    return QUrl::fromUserInput("qrc:/qml/APMSensorsComponentSummary.qml");
}

QString APMSensorsComponent::prerequisiteSetup(void) const
{
    APMAutoPilotPlugin* plugin = dynamic_cast<APMAutoPilotPlugin*>(_autopilot);
    Q_ASSERT(plugin);
    
    if (!plugin->airframeComponent()->setupComplete()) {
        return plugin->airframeComponent()->name();
    }
    
    return QString();
}

bool APMSensorsComponent::compassSetupNeeded(void) const
{
    const size_t cCompass = 3;
    const size_t cOffset = 3;
    QStringList devicesIds;
    QStringList rgOffsets[cCompass];

    devicesIds << "COMPASS_DEV_ID" << "COMPASS_DEV_ID2" << "COMPASS_DEV_ID3";
    rgOffsets[0] << "COMPASS_OFS_X" << "COMPASS_OFS_X" << "COMPASS_OFS_X";
    rgOffsets[1] << "COMPASS_OFS2_X" << "COMPASS_OFS2_X" << "COMPASS_OFS2_X";
    rgOffsets[2] << "COMPASS_OFS3_X" << "COMPASS_OFS3_X" << "COMPASS_OFS3_X";

    for (size_t i=0; i<cCompass; i++) {
        if (_autopilot->getParameterFact(FactSystem::defaultComponentId, devicesIds[i])->rawValue().toInt() != 0) {
            for (size_t j=0; j<cOffset; j++) {
                if (_autopilot->getParameterFact(FactSystem::defaultComponentId, rgOffsets[i][j])->rawValue().toFloat() == 0.0f) {
                    return true;
                }
            }
        }
    }

    return false;
}

bool APMSensorsComponent::accelSetupNeeded(void) const
{
    const size_t cAccel = 3;
    const size_t cOffset = 3;
    QStringList insUse;
    QStringList rgOffsets[cAccel];

    insUse << "INS_USE" << "INS_USE2" << "INS_USE3";
    rgOffsets[0] << "INS_ACCOFFS_X" << "INS_ACCOFFS_Y" << "INS_ACCOFFS_Z";
    rgOffsets[1] << "INS_ACC2OFFS_X" << "INS_ACC2OFFS_Y" << "INS_ACC2OFFS_Z";
    rgOffsets[2] << "INS_ACC3OFFS_X" << "INS_ACC3OFFS_Y" << "INS_ACC3OFFS_Z";

    for (size_t i=0; i<cAccel; i++) {
        if (_autopilot->getParameterFact(FactSystem::defaultComponentId, insUse[i])->rawValue().toInt() != 0) {
            for (size_t j=0; j<cOffset; j++) {
                if (_autopilot->getParameterFact(FactSystem::defaultComponentId, rgOffsets[i][j])->rawValue().toFloat() == 0.0f) {
                    return true;
                }
            }
        }
    }

    return false;
}

