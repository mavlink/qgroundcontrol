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
    return tr("Configure transmitter switch assignments and flight mode selection.");
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

QStringList FlightModesComponent::sectionIds() const
{
    return { QStringLiteral("Flight Modes"), QStringLiteral("Switch Settings") };
}

QString FlightModesComponent::sectionDisplayName(const QString& sectionId) const
{
    if (sectionId == QStringLiteral("Flight Modes")) {
        return tr("Flight Modes");
    }
    if (sectionId == QStringLiteral("Switch Settings")) {
        return tr("Switch Settings");
    }
    return sectionId;
}
