#include "PX4BatteryComponent.h"

PX4BatteryComponent::PX4BatteryComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent)
    : VehicleComponent(vehicle, autopilot, AutoPilotPlugin::UnknownVehicleComponent, parent)
    , _name(tr("Battery"))
{}

QString PX4BatteryComponent::description() const
{
    return tr("Configure each battery's source and estimate, the shared charge levels and the power monitors.");
}

QString PX4BatteryComponent::iconResource() const
{
    return QStringLiteral("/qmlimages/Battery.svg");
}

QUrl PX4BatteryComponent::setupSource() const
{
    return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/PX4/PX4BatteryComponent.qml"));
}
