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

#include "APMFlightModesComponent.h"
#include "APMAutoPilotPlugin.h"
#include "APMAirframeComponent.h"
#include "APMRadioComponent.h"

APMFlightModesComponent::APMFlightModesComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent) :
    VehicleComponent(vehicle, autopilot, parent),
    _name(tr("Flight Modes"))
{
}

QString APMFlightModesComponent::name(void) const
{
    return _name;
}

QString APMFlightModesComponent::description(void) const
{
    return QStringLiteral("The Flight Modes Component is used to assign FLight Modes to Channel 5.");
}

QString APMFlightModesComponent::iconResource(void) const
{
    return QStringLiteral("/qmlimages/FlightModesComponentIcon.png");
}

bool APMFlightModesComponent::requiresSetup(void) const
{
    return true;
}

bool APMFlightModesComponent::setupComplete(void) const
{
    return true;
}

QStringList APMFlightModesComponent::setupCompleteChangedTriggerList(void) const
{
    return QStringList();
}

QUrl APMFlightModesComponent::setupSource(void) const
{
    return QUrl::fromUserInput(QStringLiteral("qrc:/qml/APMFlightModesComponent.qml"));
}

QUrl APMFlightModesComponent::summaryQmlSource(void) const
{
    return QUrl::fromUserInput(QStringLiteral("qrc:/qml/APMFlightModesComponentSummary.qml"));
}

QString APMFlightModesComponent::prerequisiteSetup(void) const
{
    APMAutoPilotPlugin* plugin = dynamic_cast<APMAutoPilotPlugin*>(_autopilot);
    Q_ASSERT(plugin);

    if (!plugin->airframeComponent()->setupComplete()) {
        return plugin->airframeComponent()->name();
    } else if (!plugin->radioComponent()->setupComplete()) {
        return plugin->radioComponent()->name();
    }
    
    return QString();
}
