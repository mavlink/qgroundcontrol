#include "PX4BatteryComponent.h"

#include "Fact.h"
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
    const bool estimated = source == 0 || source == 2 || _canFiltered();
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

QStringList PX4BatteryComponent::summaryChangedTriggerList() const
{
    QStringList triggers;
    for (int index = 1; _parameterFact(_batteryParam(index, "SOURCE")); index++) {
        for (const char* const suffix : {"SOURCE", "V_CHARGED", "V_EMPTY", "N_CELLS"}) {
            triggers.append(_batteryParam(index, suffix));
        }
    }

    return triggers;
}

void PX4BatteryComponent::setupTriggerSignals()
{
    VehicleComponent::setupTriggerSignals();

    // PX4 only streams the parameters of batteries it uses, and the rest can arrive later
    (void) connect(_vehicle->parameterManager(), &ParameterManager::factAdded, this,
                   [this](int componentId, Fact* fact) {
                       if (componentId != _vehicle->defaultComponentId() || !fact->name().startsWith("BAT")) {
                           return;
                       }

                       if (setupCompleteChangedTriggerList().contains(fact->name())) {
                           (void) connect(fact, &Fact::valueChanged, this, &PX4BatteryComponent::_triggerUpdated);
                           emit setupCompleteChanged();
                       } else {
                           (void) connect(fact, &Fact::valueChanged, this, &PX4BatteryComponent::summaryItemsChanged);
                           emit summaryItemsChanged();
                       }
                   });
}

QList<VehicleSummaryItem> PX4BatteryComponent::summaryItems() const
{
    int batteryCount = 0;
    while (_parameterFact(_batteryParam(batteryCount + 1, "SOURCE"))) {
        batteryCount++;
    }

    // Only battery 1 is checked by setupComplete
    const bool battery1Incomplete = !setupComplete();

    // Battery 1 is always listed, the others unless they are disabled (BATn_SOURCE -1), which still leaves the
    // DroneCAN battery estimate running
    QList<int> shownBatteries;
    for (int index = 1; index <= batteryCount; index++) {
        if (index == 1 || _parameterFact(_batteryParam(index, "SOURCE"))->rawValue().toInt() != -1 ||
            _canFiltered()) {
            shownBatteries.append(index);
        }
    }

    // A single battery needs no box of its own
    const bool grouped = shownBatteries.size() > 1;

    QList<VehicleSummaryItem> items;
    for (const int index : shownBatteries) {
        const QString group = grouped ? tr("Battery %1").arg(index) : QString();
        const auto caption = [grouped](const QString& single, const QString& inGroup) {
            return grouped ? inGroup : single;
        };
        const auto addValue = [this, &items, &group, index, battery1Incomplete](const char* suffix,
                                                                               const QString& text, bool isCount) {
            const Fact* const fact = _parameterFact(_batteryParam(index, suffix));
            if (!fact) {
                items.append({text, tr("N/A"), VehicleSummaryItem::Muted, group});
                return;
            }
            const bool unset = fact->rawValue().toFloat() == 0.0f;
            items.append({text, isCount ? fact->cookedValueString() : _valueWithUnits(fact),
                          (index == 1 && battery1Incomplete && unset) ? VehicleSummaryItem::Attention
                                                                      : VehicleSummaryItem::Normal,
                          group});
        };

        Fact* const source = _parameterFact(_batteryParam(index, "SOURCE"));
        items.append({caption(tr("Battery Source"), tr("Source")), source->enumStringValue(),
                      VehicleSummaryItem::Normal, group});
        addValue("V_CHARGED", caption(tr("Battery Full"), tr("Full")), false);
        addValue("V_EMPTY", caption(tr("Battery Empty"), tr("Empty")), false);
        addValue("N_CELLS", caption(tr("Number of Cells"), tr("Number of Cells")), true);
    }

    return items;
}

QString PX4BatteryComponent::_batteryParam(int batteryIndex, const char* suffix)
{
    return QStringLiteral("BAT%1_%2").arg(batteryIndex).arg(QLatin1String(suffix));
}

bool PX4BatteryComponent::_canFiltered() const
{
    const Fact* const enable = _parameterFact(QStringLiteral("UAVCAN_ENABLE"));
    const Fact* const subscription = _parameterFact(QStringLiteral("UAVCAN_SUB_BAT"));
    return enable && subscription && enable->rawValue().toInt() > 0 && subscription->rawValue().toInt() == 2;
}
