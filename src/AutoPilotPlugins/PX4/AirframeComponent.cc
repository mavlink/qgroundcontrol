#include "AirframeComponent.h"

#include "AirframeComponentAirframes.h"
#include "Fact.h"
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

QStringList AirframeComponent::summaryChangedTriggerList(void) const
{
    return {QStringLiteral("MAV_SYS_ID")};
}

void AirframeComponent::setupTriggerSignals()
{
    VehicleComponent::setupTriggerSignals();

    (void) connect(_vehicle, &Vehicle::firmwareVersionChanged, this, &AirframeComponent::summaryItemsChanged);
    (void) connect(_vehicle, &Vehicle::firmwareCustomVersionChanged, this, &AirframeComponent::summaryItemsChanged);
}

QList<VehicleSummaryItem> AirframeComponent::summaryItems(void) const
{
    QList<VehicleSummaryItem> items;

    const Fact* const systemId = _parameterFact(QStringLiteral("MAV_SYS_ID"));
    if (systemId) {
        items.append({tr("System ID"), systemId->cookedValueString(), VehicleSummaryItem::Normal});
    }

    const Fact* const autostart = _parameterFact(QStringLiteral("SYS_AUTOSTART"));
    const int autostartId = autostart ? autostart->rawValue().toInt() : 0;
    QString typeName;
    QString vehicleName;
    if (autostartId == 0) {
        items.append({tr("Airframe type"), tr("Setup required"), VehicleSummaryItem::Attention});
        items.append({tr("Vehicle"), tr("Setup required"), VehicleSummaryItem::Attention});
    } else if (AirframeComponentAirframes::find(autostartId, typeName, vehicleName)) {
        items.append({tr("Airframe type"), typeName, VehicleSummaryItem::Normal});
        items.append({tr("Vehicle"), vehicleName, VehicleSummaryItem::Normal});
    } else {
        items.append({tr("Vehicle"), QString::number(autostartId), VehicleSummaryItem::Normal});
    }

    if (_vehicle->firmwareMajorVersion() == Vehicle::versionNotSetValue) {
        items.append({tr("Firmware Version"), tr("Unknown"), VehicleSummaryItem::Muted});
    } else {
        items.append({tr("Firmware Version"),
                      QStringLiteral("%1.%2.%3%4")
                          .arg(_vehicle->firmwareMajorVersion())
                          .arg(_vehicle->firmwareMinorVersion())
                          .arg(_vehicle->firmwarePatchVersion())
                          .arg(_vehicle->firmwareVersionTypeString()),
                      VehicleSummaryItem::Normal});
    }

    if (_vehicle->firmwareCustomMajorVersion() != Vehicle::versionNotSetValue) {
        items.append({tr("Custom Fw. Ver."),
                      QStringLiteral("%1.%2.%3")
                          .arg(_vehicle->firmwareCustomMajorVersion())
                          .arg(_vehicle->firmwareCustomMinorVersion())
                          .arg(_vehicle->firmwareCustomPatchVersion()),
                      VehicleSummaryItem::Normal});
    }

    return items;
}
