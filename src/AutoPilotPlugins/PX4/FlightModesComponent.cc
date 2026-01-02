/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "FlightModesComponent.h"
#include "ParameterManager.h"
#include "Vehicle.h"

struct SwitchListItem {
    const char* param;
    const char* name;
};

FlightModesComponent::FlightModesComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent)
    : VehicleComponent(vehicle, autopilot, AutoPilotPlugin::KnownFlightModesVehicleComponent, parent)
    , _name(tr("Flight Modes"))
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

QUrl FlightModesComponent::setupSource(void) const
{
    return QUrl::fromUserInput("qrc:/qml/QGroundControl/AutoPilotPlugins/PX4/PX4FlightModes.qml");
}

QUrl FlightModesComponent::summaryQmlSource(void) const
{
    return QUrl::fromUserInput("qrc:/qml/QGroundControl/AutoPilotPlugins/PX4/FlightModesComponentSummary.qml");
}
