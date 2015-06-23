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

/// @brief Used to translate from RC_MAP_* parameters name to user string
static const SwitchListItem switchList[] = {
    { "RC_MAP_MODE_SW",     "Mode Switch:" },                // First entry must be mode switch
    { "RC_MAP_POSCTL_SW",   "Position Control Switch:" },
    { "RC_MAP_LOITER_SW",   "Loiter Switch:" },
    { "RC_MAP_RETURN_SW",   "Return Switch:" },
    { "RC_MAP_ACRO_SW",     "Acro Switch:" },
};
static const size_t cSwitchList = sizeof(switchList) / sizeof(switchList[0]);

FlightModesComponent::FlightModesComponent(UASInterface* uas, AutoPilotPlugin* autopilot, QObject* parent) :
    PX4Component(uas, autopilot, parent),
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
    return true;
}

bool FlightModesComponent::setupComplete(void) const
{
    return _autopilot->getParameterFact(FactSystem::defaultComponentId, "RC_MAP_MODE_SW")->value().toInt() != 0;
}

QString FlightModesComponent::setupStateDescription(void) const
{
    const char* stateDescription;
    
    if (requiresSetup()) {
        stateDescription = "Requires calibration";
    } else {
        stateDescription = "Calibrated";
    }
    return QString(stateDescription);
}

QStringList FlightModesComponent::setupCompleteChangedTriggerList(void) const
{
    return QStringList("RC_MAP_MODE_SW");
}

QStringList FlightModesComponent::paramFilterList(void) const
{
    QStringList list;
    
    for (size_t i=0; i<cSwitchList; i++) {
        list << switchList[i].param;
    }

    return list;
}

QUrl FlightModesComponent::setupSource(void) const
{
    return QUrl::fromUserInput("qrc:/qml/FlightModesComponent.qml");
}

QUrl FlightModesComponent::summaryQmlSource(void) const
{
    return QUrl::fromUserInput("qrc:/qml/FlightModesComponentSummary.qml");
}

QString FlightModesComponent::prerequisiteSetup(void) const
{
    PX4AutoPilotPlugin* plugin = dynamic_cast<PX4AutoPilotPlugin*>(_autopilot);
    Q_ASSERT(plugin);

    if (!plugin->airframeComponent()->setupComplete()) {
        return plugin->airframeComponent()->name();
    } else if (!plugin->radioComponent()->setupComplete()) {
        return plugin->radioComponent()->name();
    }
    
    return QString();
}
