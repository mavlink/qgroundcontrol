#include "PX4FailsafesComponent.h"

#include <optional>

#include "Fact.h"
#include "Vehicle.h"

PX4FailsafesComponent::PX4FailsafesComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent)
    : VehicleComponent(vehicle, autopilot, AutoPilotPlugin::UnknownVehicleComponent, parent)
    , _name(tr("Failsafes"))
{}

QString PX4FailsafesComponent::description() const
{
    return tr("Configure what the vehicle does when it loses a link, runs low on power or detects a fault.");
}

QString PX4FailsafesComponent::iconResource() const
{
    return QStringLiteral("/InstrumentValueIcons/exclamation-outline.svg");
}

QStringList PX4FailsafesComponent::searchKeywords() const
{
    return {
        tr("RC loss"),           tr("Joystick loss"), tr("Radio"),          tr("Signal loss"),
        tr("Data link loss"),    tr("Telemetry"),     tr("Ground station"), tr("GCS"),
        tr("Geofence"),          tr("Fence"),         tr("Breach"),         tr("Low battery"),
        tr("Critical battery"),  tr("GPS loss"),      tr("Wind"),           tr("Flight time"),
        tr("Traffic avoidance"),
    };
}

QUrl PX4FailsafesComponent::setupSource() const
{
    return QUrl::fromUserInput(
        QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/PX4/PX4FailsafesComponent.qml"));
}

QStringList PX4FailsafesComponent::summaryChangedTriggerList() const
{
    return {"COM_LOW_BAT_ACT", "NAV_RCL_ACT", "COM_RC_IN_MODE", "NAV_DLL_ACT", "GF_ACTION", "COM_QC_ACT"};
}

QList<VehicleSummaryItem> PX4FailsafesComponent::summaryItems() const
{
    QList<VehicleSummaryItem> items;

    // offValue is the raw action value which turns the failsafe off, if it has one
    const auto addAction = [this, &items](const char* param, const QString& caption, std::optional<int> offValue) {
        Fact* const fact = _parameterFact(QLatin1String(param));
        if (!fact) {
            return;
        }
        items.append({caption, fact->enumStringValue(),
                      fact->rawValue().toInt() == offValue ? VehicleSummaryItem::Muted : VehicleSummaryItem::Normal});
    };

    addAction("COM_LOW_BAT_ACT", tr("Low Battery"), std::nullopt);

    // Keep in sync with PX4FailsafeRows.js: manual control loss only applies when a manual control source is in use
    const Fact* const manualControlSource = _parameterFact(QStringLiteral("COM_RC_IN_MODE"));
    if (!manualControlSource || manualControlSource->rawValue().toInt() != 4) {
        addAction("NAV_RCL_ACT", tr("Manual Control Loss"), 0);
    }
    addAction("NAV_DLL_ACT", tr("Data Link Loss"), 0);
    addAction("GF_ACTION", tr("Geofence"), 0);
    if (_vehicle && _vehicle->vtol()) {
        addAction("COM_QC_ACT", tr("Quadchute"), std::nullopt);
    }

    return items;
}
