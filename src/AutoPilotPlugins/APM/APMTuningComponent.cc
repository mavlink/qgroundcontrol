#include "APMTuningComponent.h"
#include "Vehicle.h"

APMTuningComponent::APMTuningComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent)
    : VehicleComponent(vehicle, autopilot, AutoPilotPlugin::UnknownVehicleComponent, parent)
{

}

QString APMTuningComponent::vehicleConfigJson() const
{
    return QStringLiteral(":/qml/QGroundControl/AutoPilotPlugins/APM/VehicleConfig/APMTuningCopter.VehicleConfig.json");
}

QUrl APMTuningComponent::setupSource() const
{
    switch (_vehicle->vehicleType()) {
    case MAV_TYPE_QUADROTOR:
    case MAV_TYPE_COAXIAL:
    case MAV_TYPE_HELICOPTER:
    case MAV_TYPE_HEXAROTOR:
    case MAV_TYPE_OCTOROTOR:
    case MAV_TYPE_TRICOPTER:
        // Generated from APMTuningCopter.VehicleConfig.json
        return QUrl::fromUserInput("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMTuningCopterComponent.qml");
    case MAV_TYPE_SUBMARINE:
        return QUrl::fromUserInput("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMTuningComponentSub.qml");
    default:
        return QUrl::fromUserInput(QString());
    }
}
