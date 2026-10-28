#include "PX4ReturnLandComponent.h"

#include "Fact.h"

PX4ReturnLandComponent::PX4ReturnLandComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent)
    : VehicleComponent(vehicle, autopilot, AutoPilotPlugin::UnknownVehicleComponent, parent)
    , _name(tr("Return & Land"))
{}

QString PX4ReturnLandComponent::description() const
{
    return tr("Configure where and how high the vehicle returns, what it does on arrival, and how it lands.");
}

QString PX4ReturnLandComponent::iconResource() const
{
    return QStringLiteral("/InstrumentValueIcons/home.svg");
}

QStringList PX4ReturnLandComponent::searchKeywords() const
{
    return {
        tr("RTL"),     tr("Return to launch"), tr("Return home"), tr("Loiter"),   tr("Landing"),
        tr("Descent"), tr("Disarm"),           tr("Touchdown"),   tr("Altitude"),
    };
}

QUrl PX4ReturnLandComponent::setupSource() const
{
    return QUrl::fromUserInput(
        QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/PX4/PX4ReturnLandComponent.qml"));
}

QList<VehicleSummaryItem> PX4ReturnLandComponent::summaryItems() const
{
    const Fact* const returnAlt = _parameterFact(QStringLiteral("RTL_RETURN_ALT"));
    const Fact* const descendAlt = _parameterFact(QStringLiteral("RTL_DESCEND_ALT"));
    const Fact* const landDelay = _parameterFact(QStringLiteral("RTL_LAND_DELAY"));
    const int landDelayValue = landDelay ? landDelay->rawValue().toInt() : 0;

    QList<VehicleSummaryItem> items;
    if (returnAlt) {
        items.append({tr("RTL Climb To"), _valueWithUnits(returnAlt), VehicleSummaryItem::Normal});
    }
    if (landDelay) {
        const QString then = landDelayValue == 0  ? tr("Land immediately")
                             : landDelayValue < 0 ? tr("Loiter and do not land")
                                                  : tr("Loiter and land after specified time");
        items.append({tr("RTL, Then"), then, VehicleSummaryItem::Normal});
    }
    if (descendAlt && landDelayValue != 0) {
        items.append({tr("Loiter Alt"), _valueWithUnits(descendAlt), VehicleSummaryItem::Normal});
    }
    if (landDelay && landDelayValue > 0) {
        items.append({tr("Land Delay"), _valueWithUnits(landDelay), VehicleSummaryItem::Normal});
    }

    return items;
}
