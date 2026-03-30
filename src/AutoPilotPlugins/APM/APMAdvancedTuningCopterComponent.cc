#include "APMAdvancedTuningCopterComponent.h"
#include "Vehicle.h"

APMAdvancedTuningCopterComponent::APMAdvancedTuningCopterComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent)
    : VehicleComponent(vehicle, autopilot, AutoPilotPlugin::UnknownVehicleComponent, parent)
{
}

QUrl APMAdvancedTuningCopterComponent::setupSource() const
{
    switch (_vehicle->vehicleType()) {
    case MAV_TYPE_QUADROTOR:
    case MAV_TYPE_COAXIAL:
    case MAV_TYPE_HELICOPTER:
    case MAV_TYPE_HEXAROTOR:
    case MAV_TYPE_OCTOROTOR:
    case MAV_TYPE_TRICOPTER:
        return QUrl::fromUserInput("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMAdvancedTuningCopterComponent.qml");
    default:
        return QUrl::fromUserInput(QString());
    }
}
