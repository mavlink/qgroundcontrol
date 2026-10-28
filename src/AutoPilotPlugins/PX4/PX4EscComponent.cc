#include "PX4EscComponent.h"

#include "Fact.h"

namespace {

// Keep in sync with PX4EscRows.js timerParams()
constexpr int kTimerGroupCount = 8;

}  // namespace

PX4EscComponent::PX4EscComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent)
    : VehicleComponent(vehicle, autopilot, AutoPilotPlugin::UnknownVehicleComponent, parent)
    , _name(tr("ESC"))
{}

QString PX4EscComponent::description() const
{
    return tr("Calibrate PWM ESCs and configure DShot and DroneCAN ESC settings.");
}

QString PX4EscComponent::iconResource() const
{
    return QStringLiteral("/InstrumentValueIcons/bolt.svg");
}

QStringList PX4EscComponent::searchKeywords() const
{
    return {
        tr("Electronic speed controller"), tr("Calibration"), tr("Motor"), tr("PWM"), tr("DShot"), tr("DroneCAN"),
    };
}

QUrl PX4EscComponent::setupSource() const
{
    return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/PX4/PX4EscComponent.qml"));
}

QStringList PX4EscComponent::summaryChangedTriggerList() const
{
    QStringList triggers{"DSHOT_BIDIR_EN", "MOT_POLE_COUNT"};
    for (int i = 0; i < kTimerGroupCount; i++) {
        triggers.append(QStringLiteral("PWM_MAIN_TIM%1").arg(i));
        triggers.append(QStringLiteral("PWM_AUX_TIM%1").arg(i));
    }

    return triggers;
}

QList<VehicleSummaryItem> PX4EscComponent::summaryItems() const
{
    // Output protocol values of -1 and above are PWM or OneShot, below -1 are DShot
    bool pwm = false;
    bool dshot = false;
    for (int i = 0; i < kTimerGroupCount; i++) {
        for (const QString& prefix : {QStringLiteral("PWM_MAIN_TIM"), QStringLiteral("PWM_AUX_TIM")}) {
            const Fact* const protocol = _parameterFact(prefix + QString::number(i));
            if (!protocol) {
                continue;
            }
            if (protocol->rawValue().toInt() < -1) {
                dshot = true;
            } else {
                pwm = true;
            }
        }
    }

    QList<VehicleSummaryItem> items;
    if (dshot && pwm) {
        items.append({tr("Output"), tr("DShot and PWM")});
    } else if (dshot) {
        items.append({tr("Output"), tr("DShot")});
    } else if (pwm) {
        items.append({tr("Output"), tr("PWM")});
    }

    if (dshot) {
        Fact* const bidirectional = _parameterFact(QStringLiteral("DSHOT_BIDIR_EN"));
        if (bidirectional) {
            items.append({tr("Bidirectional DShot"), bidirectional->enumStringValue(),
                          bidirectional->rawValue().toInt() == 0 ? VehicleSummaryItem::Muted
                                                                 : VehicleSummaryItem::Normal});
        }
        const Fact* const poles = _parameterFact(QStringLiteral("MOT_POLE_COUNT"));
        if (poles) {
            items.append({tr("Motor Poles"), poles->cookedValueString()});
        }
    }

    return items;
}
