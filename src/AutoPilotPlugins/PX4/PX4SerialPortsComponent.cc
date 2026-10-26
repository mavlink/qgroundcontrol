#include "PX4SerialPortsComponent.h"

#include "Fact.h"
#include "PX4SerialPortConfig.h"
#include "ParameterManager.h"
#include "Vehicle.h"

PX4SerialPortsComponent::PX4SerialPortsComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent)
    : VehicleComponent(vehicle, autopilot, AutoPilotPlugin::UnknownVehicleComponent, parent)
    , _name(tr("Ports"))
{}

QList<Fact*> PX4SerialPortsComponent::functionFacts(ParameterManager* parameterManager)
{
    QList<Fact*> facts;

    if (!parameterManager) {
        return facts;
    }
    for (const QString& name : parameterManager->parameterNames(ParameterManager::defaultComponentId)) {
        Fact* const fact = parameterManager->getParameter(ParameterManager::defaultComponentId, name);
        if (fact && PX4SerialPortConfig::isFunctionParam(fact->enumValues())) {
            facts.append(fact);
        }
    }

    return facts;
}

QString PX4SerialPortsComponent::description() const
{
    return tr("Assign functions such as MAVLink telemetry, GPS and RC input to the vehicle's serial ports.");
}

QString PX4SerialPortsComponent::iconResource() const
{
    return QStringLiteral("/InstrumentValueIcons/link.svg");
}

QUrl PX4SerialPortsComponent::setupSource() const
{
    return QUrl::fromUserInput(
        QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/PX4/PX4SerialPortsComponent.qml"));
}

QStringList PX4SerialPortsComponent::summaryChangedTriggerList() const
{
    QStringList triggers;
    for (const Fact* const fact : functionFacts(_vehicle->parameterManager())) {
        triggers.append(fact->name());
    }

    return triggers;
}

QList<VehicleSummaryItem> PX4SerialPortsComponent::summaryItems() const
{
    QList<PX4SerialPortConfig::Function> functions;
    for (const Fact* const fact : functionFacts(_vehicle->parameterManager())) {
        functions.append(PX4SerialPortConfig::makeFunction(fact->name(), fact->shortDescription(), fact->enumValues(),
                                                           fact->enumStrings(), fact->rawValue().toInt()));
    }

    QList<VehicleSummaryItem> items;
    for (const PX4SerialPortConfig::Port& port : PX4SerialPortConfig::ports(functions)) {
        QStringList labels;
        for (const PX4SerialPortConfig::Function& function : functions) {
            if (function.currentPortId == port.id) {
                labels.append(function.label);
            }
        }
        if (!labels.isEmpty()) {
            items.append({port.label, labels.join(QStringLiteral(", "))});
        }
    }

    return items;
}
