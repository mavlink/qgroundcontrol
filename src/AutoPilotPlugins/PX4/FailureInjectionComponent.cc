/****************************************************************************
 * FailureInjectionComponent.cc
 ****************************************************************************/
#include "FailureInjectionComponent.h"

#include "AutoPilotPlugin.h"
#include "Vehicle.h"

FailureInjectionComponent::FailureInjectionComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent)
    : VehicleComponent(vehicle, autopilot, AutoPilotPlugin::UnknownVehicleComponent, parent),
      _name(tr("Failure Injection"))
{}

QString FailureInjectionComponent::name(void) const
{
    return _name;
}

QString FailureInjectionComponent::description(void) const
{
    return tr(
        "Failure Injection is used to simulate sensor and system failures (MAV_CMD_INJECT_FAILURE) "
        "to validate failsafes. Requires SYS_FAILURE_EN = 1 and a vehicle reboot.");
}

QString FailureInjectionComponent::iconResource(void) const
{
    return QStringLiteral("/qmlimages/WarningEmergency.svg");
}

QUrl FailureInjectionComponent::setupSource(void) const
{
    return QUrl::fromUserInput(
        QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/PX4/FailureInjectionComponent.qml"));
}
