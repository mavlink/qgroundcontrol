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

/// @file
///     @author Gus Grubba <mavlink@grubba.com>

#include "PowerComponent.h"
#include "QGCQmlWidgetHolder.h"
#include "PX4AutoPilotPlugin.h"

PowerComponent::PowerComponent(UASInterface* uas, AutoPilotPlugin* autopilot, QObject* parent) :
    PX4Component(uas, autopilot, parent),
    _name(tr("Power"))
{
}

QString PowerComponent::name(void) const
{
    return _name;
}

QString PowerComponent::description(void) const
{
    return tr("The Power Component is used to setup battery parameters as well as advanced settings for propellers and magnetometer.");
}

QString PowerComponent::iconResource(void) const
{
    return "/qmlimages/PowerComponentIcon.png";
}

bool PowerComponent::requiresSetup(void) const
{
    return true;
}

bool PowerComponent::setupComplete(void) const
{
    QVariant cvalue, evalue, nvalue;
    return _autopilot->getParameterFact(FactSystem::defaultComponentId, "BAT_V_CHARGED")->value().toFloat() != 0.0f &&
        _autopilot->getParameterFact(FactSystem::defaultComponentId, "BAT_V_EMPTY")->value().toFloat() != 0.0f &&
        _autopilot->getParameterFact(FactSystem::defaultComponentId, "BAT_N_CELLS")->value().toInt() != 0;
}

QString PowerComponent::setupStateDescription(void) const
{
    const char* stateDescription;

    if (requiresSetup()) {
        stateDescription = "Requires setup";
    } else {
        stateDescription = "Setup complete";
    }
    return QString(stateDescription);
}

QStringList PowerComponent::setupCompleteChangedTriggerList(void) const
{
    return QStringList();
}

QStringList PowerComponent::paramFilterList(void) const
{
    QStringList list;

    return list;
}

QUrl PowerComponent::setupSource(void) const
{
    return QUrl::fromUserInput("qrc:/qml/PowerComponent.qml");
}

QUrl PowerComponent::summaryQmlSource(void) const
{
    return QUrl::fromUserInput("qrc:/qml/PowerComponentSummary.qml");
}

QString PowerComponent::prerequisiteSetup(void) const
{
    PX4AutoPilotPlugin* plugin = dynamic_cast<PX4AutoPilotPlugin*>(_autopilot);
    Q_ASSERT(plugin);
    if (!plugin->airframeComponent()->setupComplete()) {
        return plugin->airframeComponent()->name();
    }
    return QString();
}
