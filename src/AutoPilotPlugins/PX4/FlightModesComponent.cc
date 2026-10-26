#include "FlightModesComponent.h"

#include "Fact.h"
#include "PX4FirmwarePlugin.h"
#include "PX4FlightModeConfig.h"
#include "ParameterManager.h"
#include "StandardModes.h"
#include "Vehicle.h"

struct SwitchListItem {
    const char* param;
    const char* name;
};

FlightModesComponent::FlightModesComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent)
    : VehicleComponent(vehicle, autopilot, AutoPilotPlugin::KnownFlightModesVehicleComponent, parent)
    , _name(tr("Flight Modes"))
{
}

QString FlightModesComponent::name(void) const
{
    return _name;
}

QString FlightModesComponent::description(void) const
{
    return tr("Choose how the pilot selects flight modes, and what each transmitter switch does.");
}

QString FlightModesComponent::iconResource(void) const
{
    return "/qmlimages/FlightModesComponentIcon.png";
}

QUrl FlightModesComponent::setupSource(void) const
{
    return QUrl::fromUserInput("qrc:/qml/QGroundControl/AutoPilotPlugins/PX4/PX4FlightModes.qml");
}

QStringList FlightModesComponent::summaryChangedTriggerList() const
{
    QStringList triggers{QStringLiteral("RC_MAP_FLTMODE"), QStringLiteral("RC_MAP_FLTM_BTN"),
                         QStringLiteral("COM_FLTMODE_BOOT")};
    for (int slot = 1; slot <= PX4FlightModeConfig::kModeSlotCount; slot++) {
        triggers.append(QStringLiteral("COM_FLTMODE%1").arg(slot));
    }

    return triggers;
}

void FlightModesComponent::setupTriggerSignals()
{
    VehicleComponent::setupTriggerSignals();

    // The names of the external modes come from the vehicle
    (void) connect(_vehicle, &Vehicle::flightModesChanged, this, &FlightModesComponent::summaryItemsChanged);
}

QString FlightModesComponent::_modeName(const QString& paramName) const
{
    Fact* const fact = _parameterFact(paramName);
    if (!fact) {
        return {};
    }

    const QStringList externalNames =
        _vehicle->standardModes() ? PX4FirmwarePlugin::externalFlightModeNames(_vehicle->standardModes()->modeList())
                                  : QStringList();
    const QString name = PX4FlightModeConfig::modeName(paramName, fact->enumStrings(), fact->enumValues(),
                                                       externalNames, fact->rawValue().toInt());

    return name.isEmpty() ? fact->cookedValueString() : name;
}

QList<VehicleSummaryItem> FlightModesComponent::summaryItems() const
{
    Fact* const modeChannel = _parameterFact(QStringLiteral("RC_MAP_FLTMODE"));
    const Fact* const buttons = _parameterFact(QStringLiteral("RC_MAP_FLTM_BTN"));
    const int modeChannelValue = modeChannel ? modeChannel->rawValue().toInt() : 0;
    const int buttonMask = buttons ? buttons->rawValue().toInt() : 0;

    QList<VehicleSummaryItem> items;
    bool selectionAssigned = true;
    switch (PX4FlightModeConfig::selectionMethod(modeChannelValue, buttonMask)) {
    case PX4FlightModeConfig::SelectionMethod::Switch:
        items.append({tr("Mode selection"), modeChannel->enumStringValue(), VehicleSummaryItem::Normal});
        break;
    case PX4FlightModeConfig::SelectionMethod::Buttons: {
        QStringList channels;
        for (const int channel : PX4FlightModeConfig::buttonChannels(buttonMask)) {
            channels.append(tr("Ch %1").arg(channel));
        }
        items.append({tr("Mode selection"), tr("Buttons on %1").arg(channels.join(QStringLiteral(", "))),
                      VehicleSummaryItem::Normal});
        break;
    }
    case PX4FlightModeConfig::SelectionMethod::None:
        selectionAssigned = false;
        items.append({tr("Mode selection"), tr("Not assigned"), VehicleSummaryItem::Muted});
        break;
    }

    if (selectionAssigned) {
        for (int slot = 1; slot <= PX4FlightModeConfig::kModeSlotCount; slot++) {
            const QString paramName = QStringLiteral("COM_FLTMODE%1").arg(slot);
            if (_parameterFact(paramName)) {
                items.append({tr("Flight Mode %1").arg(slot), _modeName(paramName), VehicleSummaryItem::Normal});
            }
        }
    }

    if (_parameterFact(QStringLiteral("COM_FLTMODE_BOOT"))) {
        items.append({tr("Mode on boot"), _modeName(QStringLiteral("COM_FLTMODE_BOOT")), VehicleSummaryItem::Normal});
    }

    return items;
}
