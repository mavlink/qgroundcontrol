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

/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "SensorsComponent.h"
#include "PX4AutoPilotPlugin.h"
#include "QGCQmlWidgetHolder.h"
#include "SensorsComponentController.h"

// These two list must be kept in sync

SensorsComponent::SensorsComponent(UASInterface* uas, AutoPilotPlugin* autopilot, QObject* parent) :
    PX4Component(uas, autopilot, parent),
    _name(tr("Sensors"))
{

}

QString SensorsComponent::name(void) const
{
    return _name;
}

QString SensorsComponent::description(void) const
{
    return tr("The Sensors Component allows you to calibrate the sensors within your vehicle. "
              "Prior to flight you must calibrate the Magnetometer, Gyroscope and Accelerometer.");
}

QString SensorsComponent::iconResource(void) const
{
    return "/qmlimages/SensorsComponentIcon.png";
}

bool SensorsComponent::requiresSetup(void) const
{
    return true;
}

bool SensorsComponent::setupComplete(void) const
{
    foreach(QString triggerParam, setupCompleteChangedTriggerList()) {
        if (_autopilot->getParameterFact(FactSystem::defaultComponentId, triggerParam)->value().toFloat() == 0.0f) {
            return false;
        }
    }

    return true;
}

QString SensorsComponent::setupStateDescription(void) const
{
    const char* stateDescription;
    
    if (requiresSetup()) {
        stateDescription = "Requires calibration";
    } else {
        stateDescription = "Calibrated";
    }
    return QString(stateDescription);
}

QStringList SensorsComponent::setupCompleteChangedTriggerList(void) const
{
    QStringList triggers;
    
    triggers << "CAL_MAG0_ID" << "CAL_GYRO0_ID" << "CAL_ACC0_ID";
    if (_uas->getSystemType() == MAV_TYPE_FIXED_WING ||
        _uas->getSystemType() == MAV_TYPE_VTOL_DUOROTOR ||
        _uas->getSystemType() == MAV_TYPE_VTOL_QUADROTOR ||
        _uas->getSystemType() == MAV_TYPE_VTOL_TILTROTOR) {
        triggers << "SENS_DPRES_OFF";
    }
    
    return triggers;
}

QStringList SensorsComponent::paramFilterList(void) const
{
    QStringList list;
    
    list << "SENS_*" << "CAL_*";
    
    return list;
}

QUrl SensorsComponent::setupSource(void) const
{
    return QUrl::fromUserInput("qrc:/qml/SensorsComponent.qml");
}

QUrl SensorsComponent::summaryQmlSource(void) const
{
    QString summaryQml;
    
    qDebug() << _uas->getSystemType();
    if (_uas->getSystemType() == MAV_TYPE_FIXED_WING ||
        _uas->getSystemType() == MAV_TYPE_VTOL_DUOROTOR ||
        _uas->getSystemType() == MAV_TYPE_VTOL_QUADROTOR ||
        _uas->getSystemType() == MAV_TYPE_VTOL_TILTROTOR) {
        summaryQml = "qrc:/qml/SensorsComponentSummaryFixedWing.qml";
    } else {
        summaryQml = "qrc:/qml/SensorsComponentSummary.qml";
    }
    
    return QUrl::fromUserInput(summaryQml);
}

QString SensorsComponent::prerequisiteSetup(void) const
{
    PX4AutoPilotPlugin* plugin = dynamic_cast<PX4AutoPilotPlugin*>(_autopilot);
    Q_ASSERT(plugin);
    
    if (!plugin->airframeComponent()->setupComplete()) {
        return plugin->airframeComponent()->name();
    }
    
    return QString();
}
