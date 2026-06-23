#include "APMPowerComponent.h"
#include "Vehicle.h"

APMPowerComponent::APMPowerComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent)
    : VehicleComponent(vehicle, autopilot, AutoPilotPlugin::KnownPowerVehicleComponent, parent)
{

}

QString APMPowerComponent::vehicleConfigJson() const
{
    return QStringLiteral(":/qml/QGroundControl/AutoPilotPlugins/APM/VehicleConfig/APMPower.VehicleConfig.json");
}

QUrl APMPowerComponent::setupSource() const
{
    // Generated from APMPower.VehicleConfig.json
    return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMPowerComponent.qml"));
}
