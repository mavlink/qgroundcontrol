#include "PX4BatteryComponent.h"

#include "ParameterManager.h"
#include "Vehicle.h"

PX4BatteryComponent::PX4BatteryComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent)
    : VehicleComponent(vehicle, autopilot, AutoPilotPlugin::KnownPowerVehicleComponent, parent)
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

QStringList PX4BatteryComponent::searchKeywords() const
{
    return {
        tr("Voltage"),  tr("Current"),     tr("Power module"),     tr("Cells"),     tr("LiPo"),
        tr("Capacity"), tr("Low battery"), tr("Critical battery"), tr("Emergency"),
    };
}

bool PX4BatteryComponent::setupComplete() const
{
    ParameterManager* const parameterManager = _vehicle->parameterManager();
    const int componentId = ParameterManager::defaultComponentId;
    if (!parameterManager->parameterExists(componentId, "BAT1_SOURCE") ||
        !parameterManager->parameterExists(componentId, "BAT1_V_CHARGED") ||
        !parameterManager->parameterExists(componentId, "BAT1_V_EMPTY") ||
        !parameterManager->parameterExists(componentId, "BAT1_N_CELLS")) {
        return true;
    }

    // Cells and voltages are only used, and only shown on the page, when PX4 estimates the battery itself.
    // Keep in sync with PX4BatteryRows.batteryInputs().
    const int source = parameterManager->getParameter(componentId, "BAT1_SOURCE")->rawValue().toInt();
    const bool canFiltered = parameterManager->parameterExists(componentId, "UAVCAN_ENABLE") &&
                             parameterManager->parameterExists(componentId, "UAVCAN_SUB_BAT") &&
                             parameterManager->getParameter(componentId, "UAVCAN_ENABLE")->rawValue().toInt() > 0 &&
                             parameterManager->getParameter(componentId, "UAVCAN_SUB_BAT")->rawValue().toInt() == 2;
    const bool estimated = source == 0 || source == 2 || canFiltered;
    if (!estimated) {
        return true;
    }

    return parameterManager->getParameter(componentId, "BAT1_V_CHARGED")->rawValue().toFloat() != 0.0f &&
           parameterManager->getParameter(componentId, "BAT1_V_EMPTY")->rawValue().toFloat() != 0.0f &&
           parameterManager->getParameter(componentId, "BAT1_N_CELLS")->rawValue().toInt() != 0;
}

QStringList PX4BatteryComponent::setupCompleteChangedTriggerList() const
{
    return {"BAT1_SOURCE", "BAT1_V_CHARGED", "BAT1_V_EMPTY", "BAT1_N_CELLS", "UAVCAN_ENABLE", "UAVCAN_SUB_BAT"};
}

QUrl PX4BatteryComponent::setupSource() const
{
    return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/PX4/PX4BatteryComponent.qml"));
}

QUrl PX4BatteryComponent::summaryQmlSource() const
{
    return QUrl::fromUserInput(
        QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/PX4/PX4BatteryComponentSummary.qml"));
}
