#include "CustomFirmwarePlugin.h"
#include "CustomAutoPilotPlugin.h"
#include "px4_custom_mode.h"
#include "Vehicle.h"

CustomFirmwarePlugin::CustomFirmwarePlugin()
{
    for (auto &mode: _flightModeList) {
        //-- Narrow the flight mode options to only these
        if ((mode.mode_name != pauseFlightMode()) && (mode.mode_name != rtlFlightMode()) && (mode.mode_name != missionFlightMode())) {
            // No other flight modes can be set
            mode.canBeSet = false;
        }
    }
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
    // This custom build only allows Hold, Return and Mission to be set by the user.
    for (auto &mode: modeList) {
        const PX4CustomMode::Mode cMode = static_cast<PX4CustomMode::Mode>(mode.custom_mode);
        mode.canBeSet = (cMode == PX4CustomMode::AUTO_LOITER) ||
                        (cMode == PX4CustomMode::AUTO_RTL) ||
                        (cMode == PX4CustomMode::AUTO_MISSION);
    }

    // Let the base class do the standard airframe (fixed wing / multi rotor) classification
    // and update the internal flight mode lists.
    PX4FirmwarePlugin::updateAvailableFlightModes(modeList);
}
