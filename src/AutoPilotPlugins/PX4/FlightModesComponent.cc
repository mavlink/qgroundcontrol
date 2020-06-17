/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "FlightModesComponent.h"
#include "PX4AutoPilotPlugin.h"

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
    return _vehicle->parameterManager()->getParameter(-1, "COM_RC_IN_MODE")->rawValue().toInt() == 1 ? false : true;
}

bool FlightModesComponent::setupComplete(void) const
{
    if (_vehicle->parameterManager()->getParameter(-1, "COM_RC_IN_MODE")->rawValue().toInt() == 1) {
        return true;
    }

    if (_vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, "RC_MAP_MODE_SW")->rawValue().toInt() != 0 ||
            (_vehicle->parameterManager()->parameterExists(FactSystem::defaultComponentId, "RC_MAP_FLTMODE") && _vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, "RC_MAP_FLTMODE")->rawValue().toInt() != 0)) {
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
