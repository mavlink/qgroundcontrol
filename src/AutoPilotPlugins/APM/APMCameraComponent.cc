#include "APMCameraComponent.h"
#include "Vehicle.h"

APMCameraComponent::APMCameraComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent)
    : VehicleComponent(vehicle, autopilot, AutoPilotPlugin::UnknownVehicleComponent, parent)
{

}

QUrl APMCameraComponent::setupSource() const
{
    if (_vehicle->sub()) {
        return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMCameraSubComponent.qml"));
    }

    return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMCameraComponent.qml"));
}
