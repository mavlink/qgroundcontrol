#include "PX4FailsafesComponent.h"

PX4FailsafesComponent::PX4FailsafesComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent)
    : VehicleComponent(vehicle, autopilot, AutoPilotPlugin::UnknownVehicleComponent, parent)
    , _name(tr("Failsafes"))
{}

QString PX4FailsafesComponent::description() const
{
    return tr("Configure what the vehicle does when it loses a link, runs low on power or detects a fault.");
}

QString PX4FailsafesComponent::iconResource() const
{
    return QStringLiteral("/InstrumentValueIcons/exclamation-outline.svg");
}

QUrl PX4FailsafesComponent::setupSource() const
{
    return QUrl::fromUserInput(
        QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/PX4/PX4FailsafesComponent.qml"));
}
