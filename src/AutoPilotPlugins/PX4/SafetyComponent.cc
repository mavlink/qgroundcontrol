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

#include "SafetyComponent.h"
#include "PX4AutoPilotPlugin.h"

SafetyComponent::SafetyComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent) :
    VehicleComponent(vehicle, autopilot, parent),
    _name(tr("Safety"))
{
}

QString SafetyComponent::name(void) const
{
    return _name;
}

QString SafetyComponent::description(void) const
{
    return tr("The Safety Component is used to setup triggers for Return to Land as well as the settings for Return to Land itself.");
}

QString SafetyComponent::iconResource(void) const
{
    return "/qmlimages/SafetyComponentIcon.png";
}

bool SafetyComponent::requiresSetup(void) const
{
    return false;
}

bool SafetyComponent::setupComplete(void) const
{
    // FIXME: What aboout invalid settings?
    return true;
}

QStringList SafetyComponent::setupCompleteChangedTriggerList(void) const
{
    return QStringList();
}

QUrl SafetyComponent::setupSource(void) const
{
    return QUrl::fromUserInput("qrc:/qml/SafetyComponent.qml");
}

QUrl SafetyComponent::summaryQmlSource(void) const
{
    return QUrl::fromUserInput("qrc:/qml/SafetyComponentSummary.qml");
}

QString SafetyComponent::prerequisiteSetup(void) const
{
    PX4AutoPilotPlugin* plugin = dynamic_cast<PX4AutoPilotPlugin*>(_autopilot);
    Q_ASSERT(plugin);

    if (!plugin->airframeComponent()->setupComplete()) {
        return plugin->airframeComponent()->name();
    }

    return QString();
}
