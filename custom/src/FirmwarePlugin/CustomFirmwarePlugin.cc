#include <algorithm>

#include "CustomFirmwarePlugin.h"
#include "CustomAutoPilotPlugin.h"
#include "px4_custom_mode.h"
#include "Vehicle.h"

namespace {
constexpr uint32_t EXTERNAL_MODE_3 = (uint32_t(PX4_CUSTOM_MAIN_MODE_AUTO) << 16) |
                                     (uint32_t(PX4_CUSTOM_SUB_MODE_EXTERNAL3) << 24);
constexpr uint32_t EXTERNAL_MODE_4 = (uint32_t(PX4_CUSTOM_MAIN_MODE_AUTO) << 16) |
                                     (uint32_t(PX4_CUSTOM_SUB_MODE_EXTERNAL4) << 24);
} // namespace

CustomFirmwarePlugin::CustomFirmwarePlugin()
{
    // Base-constructor virtual calls cannot apply our policy. Keep the complete fallback table for
    // telemetry and safety commands; a later authoritative AVAILABLE_MODES list replaces it entirely.
    FlightModeList modeList = _flightModeList;
    modeList.append({tr("External Mode 3"), uint8_t(0), EXTERNAL_MODE_3, true, false, true, true});
    modeList.append({tr("External Mode 4"), uint8_t(0), EXTERNAL_MODE_4, true, false, true, true});
    updateAvailableFlightModes(modeList);
}

AutoPilotPlugin* CustomFirmwarePlugin::autopilotPlugin(Vehicle *vehicle) const
{
    return new CustomAutoPilotPlugin(vehicle, vehicle);
}

const QVariantList& CustomFirmwarePlugin::toolIndicators(const Vehicle *vehicle)
{
    if (_toolIndicatorList.size() == 0) {
        // First call the base class to get the standard QGC list. This way we are guaranteed to always get
        // any new toolbar indicators which are added upstream in our custom build.
        _toolIndicatorList = FirmwarePlugin::toolIndicators(vehicle);
        // Then specifically remove the RC RSSI indicator.
        _toolIndicatorList.removeOne(QVariant::fromValue(QUrl::fromUserInput("qrc:/qml/QGroundControl/Toolbar/RCRSSIIndicator.qml")));
    }

    return _toolIndicatorList;
}

bool CustomFirmwarePlugin::hasGimbal(Vehicle* /*vehicle*/, bool &rollSupported, bool &pitchSupported, bool &yawSupported) const
{
    rollSupported = false;
    pitchSupported = true;
    yawSupported = true;

    return true;
}

void CustomFirmwarePlugin::updateAvailableFlightModes(FlightModeList &modeList)
{
    for (auto &mode : modeList) {
        mode.canBeSet = mode.canBeSet &&
                        ((mode.custom_mode == EXTERNAL_MODE_3) || (mode.custom_mode == EXTERNAL_MODE_4)) &&
                        !mode.mode_name.trimmed().isEmpty();
        if (mode.canBeSet) {
            // PX4 setFlightMode resolves names case-insensitively, including non-selectable entries.
            // Do not expose a label that could command a different numeric mode.
            mode.canBeSet = std::count_if(modeList.cbegin(), modeList.cend(), [&mode](const auto &other) {
                return mode.mode_name.compare(other.mode_name, Qt::CaseInsensitive) == 0;
            }) == 1;
        }
    }

    // Let the base class do the standard airframe (fixed wing / multi rotor) classification
    // and update the internal flight mode lists.
    PX4FirmwarePlugin::updateAvailableFlightModes(modeList);
}
