#include "AirframeComponent.h"
#include "ParameterManager.h"
#include "Vehicle.h"

AirframeComponent::AirframeComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent)
    : VehicleComponent(vehicle, autopilot, AutoPilotPlugin::UnknownVehicleComponent, parent)
    , _name(tr("Airframe"))
{

}

QString AirframeComponent::name(void) const
{
    return _name;
}

QString AirframeComponent::description(void) const
{
    return tr("Configure the airframe type and apply default tuning parameters.");
}

QString AirframeComponent::iconResource(void) const
{
    return "/qmlimages/AirframeComponentIcon.png";
}

bool AirframeComponent::requiresSetup(void) const
{
    return true;
}

bool AirframeComponent::setupComplete(void) const
{
    return _vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, QStringLiteral("SYS_AUTOSTART"))->rawValue().toInt() != 0;
}

QStringList AirframeComponent::setupCompleteChangedTriggerList(void) const
{
    return QStringList(QStringLiteral("SYS_AUTOSTART"));
}

QUrl AirframeComponent::setupSource(void) const
{
    return QUrl::fromUserInput("qrc:/qml/QGroundControl/AutoPilotPlugins/PX4/AirframeComponent.qml");
}

QUrl AirframeComponent::summaryQmlSource(void) const
{
    return QUrl::fromUserInput("qrc:/qml/QGroundControl/AutoPilotPlugins/PX4/AirframeComponentSummary.qml");
}
