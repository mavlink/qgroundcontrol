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

#include "FlightModesComponent.h"
#include "PX4AutoPilotPlugin.h"
#include "QGCQmlWidgetHolder.h"

struct SwitchListItem {
    const char* param;
    const char* name;
};

FlightModesComponent::FlightModesComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent) :
    VehicleComponent(vehicle, autopilot, parent),
    _name(tr("Flight Modes"))
{
}

QString FlightModesComponent::name(void) const
{
    return _name;
}

QString FlightModesComponent::description(void) const
{
    // FIXME: Better text
    return tr("The Flight Modes Component is used to set the switches associated with Flight Modes. "
              "At a minimum the Main Mode Switch must be assigned prior to flight.");
}

QString FlightModesComponent::iconResource(void) const
{
    return "/qmlimages/FlightModesComponentIcon.png";
}

bool FlightModesComponent::requiresSetup(void) const
{
    return _autopilot->getParameterFact(-1, "COM_RC_IN_MODE")->rawValue().toInt() == 1 ? false : true;
}

bool FlightModesComponent::setupComplete(void) const
{
    if (_autopilot->getParameterFact(-1, "COM_RC_IN_MODE")->rawValue().toInt() == 1) {
        return true;
    }

    if (_autopilot->getParameterFact(FactSystem::defaultComponentId, "RC_MAP_MODE_SW")->rawValue().toInt() != 0 ||
            (_autopilot->parameterExists(FactSystem::defaultComponentId, "RC_MAP_FLTMODE") && _autopilot->getParameterFact(FactSystem::defaultComponentId, "RC_MAP_FLTMODE")->rawValue().toInt() != 0)) {
        return true;
    }

    return false;
}

QStringList FlightModesComponent::setupCompleteChangedTriggerList(void) const
{
    QStringList list;

    list << QStringLiteral("RC_MAP_MODE_SW") << QStringLiteral("RC_MAP_FLTMODE");

    return list;
}

QUrl FlightModesComponent::setupSource(void) const
{
    return QUrl::fromUserInput("qrc:/qml/PX4FlightModes.qml");
}

QUrl FlightModesComponent::summaryQmlSource(void) const
{
    return QUrl::fromUserInput("qrc:/qml/FlightModesComponentSummary.qml");
}

QString FlightModesComponent::prerequisiteSetup(void) const
{
    if (_autopilot->getParameterFact(-1, "COM_RC_IN_MODE")->rawValue().toInt() == 1) {
        // No RC input
        return QString();
    } else {
        PX4AutoPilotPlugin* plugin = dynamic_cast<PX4AutoPilotPlugin*>(_autopilot);
        Q_ASSERT(plugin);

        if (!plugin->airframeComponent()->setupComplete()) {
            return plugin->airframeComponent()->name();
        } else if (!plugin->radioComponent()->setupComplete()) {
            return plugin->radioComponent()->name();
        }
    }
    
    return QString();
}
