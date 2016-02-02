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

#include "PX4RadioComponent.h"
#include "PX4AutoPilotPlugin.h"

PX4RadioComponent::PX4RadioComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent) :
    VehicleComponent(vehicle, autopilot, parent),
    _name(tr("Radio"))
{
}

QString PX4RadioComponent::name(void) const
{
    return _name;
}

QString PX4RadioComponent::description(void) const
{
    return tr("The Radio Component is used to setup which channels on your RC Transmitter you will use for each vehicle control such as Roll, Pitch, Yaw and Throttle. "
              "It also allows you to assign switches and dials to the various flight modes. "
              "Prior to flight you must also calibrate the extents for all of your channels.");
}

QString PX4RadioComponent::iconResource(void) const
{
    return "/qmlimages/RadioComponentIcon.png";
}

bool PX4RadioComponent::requiresSetup(void) const
{
    return _autopilot->getParameterFact(-1, "COM_RC_IN_MODE")->rawValue().toInt() == 1 ? false : true;
}

bool PX4RadioComponent::setupComplete(void) const
{
    if (_autopilot->getParameterFact(-1, "COM_RC_IN_MODE")->rawValue().toInt() != 1) {
        // The best we can do to detect the need for a radio calibration is look for attitude
        // controls to be mapped.
        QStringList attitudeMappings;
        attitudeMappings << "RC_MAP_ROLL" << "RC_MAP_PITCH" << "RC_MAP_YAW" << "RC_MAP_THROTTLE";
        foreach(const QString &mapParam, attitudeMappings) {
            if (_autopilot->getParameterFact(FactSystem::defaultComponentId, mapParam)->rawValue().toInt() == 0) {
                return false;
            }
        }
    }
    
    return true;
}

QStringList PX4RadioComponent::setupCompleteChangedTriggerList(void) const
{
    QStringList triggers;
    
    triggers << "COM_RC_IN_MODE" << "RC_MAP_ROLL" << "RC_MAP_PITCH" << "RC_MAP_YAW" << "RC_MAP_THROTTLE";
    
    return triggers;
}

QUrl PX4RadioComponent::setupSource(void) const
{
    return QUrl::fromUserInput("qrc:/qml/RadioComponent.qml");
}

QUrl PX4RadioComponent::summaryQmlSource(void) const
{
    return QUrl::fromUserInput("qrc:/qml/PX4RadioComponentSummary.qml");
}

QString PX4RadioComponent::prerequisiteSetup(void) const
{
    if (_autopilot->getParameterFact(-1, "COM_RC_IN_MODE")->rawValue().toInt() != 1) {
        PX4AutoPilotPlugin* plugin = dynamic_cast<PX4AutoPilotPlugin*>(_autopilot);
        
        if (!plugin->airframeComponent()->setupComplete()) {
            return plugin->airframeComponent()->name();

        }
    }
    
    return QString();
}
