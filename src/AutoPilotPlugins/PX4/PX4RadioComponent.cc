#include "PX4RadioComponent.h"

#include "Fact.h"
#include "ParameterManager.h"
#include "Vehicle.h"

PX4RadioComponent::PX4RadioComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent)
    : VehicleComponent(vehicle, autopilot, AutoPilotPlugin::KnownRadioVehicleComponent, parent)
    , _name(tr("Radio"))
{
}

QString PX4RadioComponent::name(void) const
{
    return _name;
}

QString PX4RadioComponent::description(void) const
{
    return tr("Configure and calibrate your transmitter and assign control channels.");
}

QString PX4RadioComponent::iconResource(void) const
{
    return "/qmlimages/RadioComponentIcon.png";
}

bool PX4RadioComponent::requiresSetup(void) const
{
    return _vehicle->parameterManager()->getParameter(-1, "COM_RC_IN_MODE")->rawValue().toInt() == 1 ? false : true;
}

bool PX4RadioComponent::setupComplete(void) const
{
    if (_vehicle->parameterManager()->getParameter(-1, "COM_RC_IN_MODE")->rawValue().toInt() != 1) {
        // The best we can do to detect the need for a radio calibration is look for attitude
        // controls to be mapped.
        for(const char* const&mapParam : {"RC_MAP_ROLL", "RC_MAP_PITCH", "RC_MAP_YAW", "RC_MAP_THROTTLE"}) {
            if (_vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, mapParam)->rawValue().toInt() == 0) {
                return false;
            }
        }
    }

    return true;
}

QStringList PX4RadioComponent::setupCompleteChangedTriggerList(void) const
{
    return {"COM_RC_IN_MODE", "RC_MAP_ROLL", "RC_MAP_PITCH", "RC_MAP_YAW", "RC_MAP_THROTTLE"};
}

QUrl PX4RadioComponent::setupSource(void) const
{
    return QUrl::fromUserInput("qrc:/qml/QGroundControl/AutoPilotPlugins/Common/RadioComponent.qml");
}

QStringList PX4RadioComponent::summaryChangedTriggerList() const
{
    return {"RC_MAP_FLAPS", "RC_MAP_AUX1", "RC_MAP_AUX2"};
}

void PX4RadioComponent::setupTriggerSignals()
{
    VehicleComponent::setupTriggerSignals();

    (void) connect(_vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, "COM_RC_IN_MODE"),
                   &Fact::valueChanged, this, &VehicleComponent::requiresSetupChanged);
}

QList<VehicleSummaryItem> PX4RadioComponent::summaryItems() const
{
    // Attitude controls must be mapped, unless the vehicle flies without a transmitter
    const VehicleSummaryItem::State attitudeState =
        setupComplete() ? VehicleSummaryItem::Muted : VehicleSummaryItem::Attention;

    QList<VehicleSummaryItem> items;
    const auto addChannel = [this, &items](const char* param, const QString& caption, const QString& unmappedText,
                                           VehicleSummaryItem::State unmappedState) {
        const Fact* const fact = _parameterFact(QString::fromLatin1(param));
        if (!fact) {
            return;
        }
        if (fact->rawValue().toInt() != 0) {
            items.append({caption, fact->cookedValueString(), VehicleSummaryItem::Normal});
        } else {
            items.append({caption, unmappedText, unmappedState});
        }
    };

    addChannel("RC_MAP_ROLL", tr("Roll"), tr("Setup required"), attitudeState);
    addChannel("RC_MAP_PITCH", tr("Pitch"), tr("Setup required"), attitudeState);
    addChannel("RC_MAP_YAW", tr("Yaw"), tr("Setup required"), attitudeState);
    addChannel("RC_MAP_THROTTLE", tr("Throttle"), tr("Setup required"), attitudeState);
    if (!_vehicle->multiRotor()) {
        addChannel("RC_MAP_FLAPS", tr("Flaps"), tr("Disabled"), VehicleSummaryItem::Muted);
    }
    addChannel("RC_MAP_AUX1", tr("Aux1"), tr("Disabled"), VehicleSummaryItem::Muted);
    addChannel("RC_MAP_AUX2", tr("Aux2"), tr("Disabled"), VehicleSummaryItem::Muted);

    return items;
}
