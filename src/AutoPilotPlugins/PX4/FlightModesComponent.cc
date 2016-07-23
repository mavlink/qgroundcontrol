/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


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
    return tr("Flight Modes Setup is used to configure the transmitter switches associated with Flight Modes.");
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
        } else if (!plugin->vehicle()->hilMode() && !plugin->sensorsComponent()->setupComplete()) {
            return plugin->sensorsComponent()->name();
        }
    }

    return QString();
}
