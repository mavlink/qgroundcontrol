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

#include "APMRadioComponent.h"
#include "APMAutoPilotPlugin.h"
#include "APMAirframeComponent.h"

APMRadioComponent::APMRadioComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent) :
    APMComponent(vehicle, autopilot, parent),
    _name(tr("Radio"))
{
}

QString APMRadioComponent::name(void) const
{
    return _name;
}

QString APMRadioComponent::description(void) const
{
    return tr("The Radio Component is used to setup which channels on your RC Transmitter you will use for each vehicle control such as Roll, Pitch, Yaw and Throttle. "
              "It also allows you to assign switches and dials to the various flight modes. "
              "Prior to flight you must also calibrate the extents for all of your channels.");
}

QString APMRadioComponent::iconResource(void) const
{
    return "/qmlimages/RadioComponentIcon.png";
}

bool APMRadioComponent::requiresSetup(void) const
{
    return true;
}

bool APMRadioComponent::setupComplete(void) const
{
    // The best we can do to detect the need for a radio calibration is look for attitude
    // controls to be mapped.
    QStringList attitudeMappings;
    attitudeMappings << "RCMAP_ROLL" << "RCMAP_PITCH" << "RCMAP_YAW" << "RCMAP_THROTTLE";
    foreach(const QString &mapParam, attitudeMappings) {
        if (_autopilot->getParameterFact(FactSystem::defaultComponentId, mapParam)->rawValue().toInt() == 0) {
            return false;
        }
    }
    
    return true;
}

QStringList APMRadioComponent::setupCompleteChangedTriggerList(void) const
{
    QStringList triggers;
    
    triggers << "RCMAP_ROLL" << "RCMAP_PITCH" << "RCMAP_YAW" << "RCMAP_THROTTLE";
    
    return triggers;
}

QUrl APMRadioComponent::setupSource(void) const
{
    return QUrl::fromUserInput("qrc:/qml/RadioComponent.qml");
}

QUrl APMRadioComponent::summaryQmlSource(void) const
{
    return QUrl::fromUserInput("qrc:/qml/APMRadioComponentSummary.qml");
}

QString APMRadioComponent::prerequisiteSetup(void) const
{
    APMAutoPilotPlugin* plugin = dynamic_cast<APMAutoPilotPlugin*>(_autopilot);

    if (!plugin->airframeComponent()->setupComplete()) {
        return plugin->airframeComponent()->name();

    }
    
    return QString();
}
