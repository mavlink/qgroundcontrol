#include "APMLoggingComponent.h"

APMLoggingComponent::APMLoggingComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent)
    : VehicleComponent(vehicle, autopilot, AutoPilotPlugin::UnknownVehicleComponent, parent)
{

}

QString APMLoggingComponent::vehicleConfigJson() const
{
    return QStringLiteral(":/qml/QGroundControl/AutoPilotPlugins/APM/VehicleConfig/APMLogging.VehicleConfig.json");
}

QUrl APMLoggingComponent::setupSource() const
{
    return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMLoggingComponent.qml"));
}
