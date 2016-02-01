/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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

#include "APMPowerComponent.h"
#include "APMAutoPilotPlugin.h"
#include "APMAirframeComponent.h"

APMPowerComponent::APMPowerComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent)
    : VehicleComponent(vehicle, autopilot, parent),
    _name("Power")
{
}

QString APMPowerComponent::name(void) const
{
    return _name;
}

QString APMPowerComponent::description(void) const
{
    return tr("The Power Component is used to setup battery parameters.");
}

QString APMPowerComponent::iconResource(void) const
{
    return QStringLiteral("/qmlimages/PowerComponentIcon.png");
}

bool APMPowerComponent::requiresSetup(void) const
{
    return true;
}

bool APMPowerComponent::setupComplete(void) const
{
    return _autopilot->getParameterFact(FactSystem::defaultComponentId, QStringLiteral("BATT_CAPACITY"))->rawValue().toInt() != 0;
}

QStringList APMPowerComponent::setupCompleteChangedTriggerList(void) const
{
    QStringList list;

    list << QStringLiteral("BATT_CAPACITY");

    return list;
}

QUrl APMPowerComponent::setupSource(void) const
{
    return QUrl::fromUserInput(QStringLiteral("qrc:/qml/APMPowerComponent.qml"));
}

QUrl APMPowerComponent::summaryQmlSource(void) const
{
    return QUrl::fromUserInput(QStringLiteral("qrc:/qml/APMPowerComponentSummary.qml"));
}

QString APMPowerComponent::prerequisiteSetup(void) const
{
    APMAutoPilotPlugin* plugin = dynamic_cast<APMAutoPilotPlugin*>(_autopilot);
    Q_ASSERT(plugin);
    if (!plugin->airframeComponent()->setupComplete()) {
        return plugin->airframeComponent()->name();
    }
    return QString();
}
