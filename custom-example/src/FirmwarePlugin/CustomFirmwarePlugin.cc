/****************************************************************************
 *
 * (c) 2009-2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 * @file
 *   @brief Custom Firmware Plugin (PX4)
 *   @author Gus Grubba <gus@auterion.com>
 *
 */

#include "CustomFirmwarePlugin.h"
#include "CustomAutoPilotPlugin.h"
#include "Vehicle.h"
#include "px4_custom_mode.h"

//-----------------------------------------------------------------------------
CustomFirmwarePlugin::CustomFirmwarePlugin()
{
    for (auto &mode: _availableFlightModeList){
        //-- Narrow the flight mode options to only these
        if(mode.mode_name != _holdFlightMode && mode.mode_name != _rtlFlightMode && mode.mode_name != _missionFlightMode){
            // No other flight modes can be set
            mode.canBeSet = false;
        }
    }
}

//-----------------------------------------------------------------------------
AutoPilotPlugin* CustomFirmwarePlugin::autopilotPlugin(Vehicle* vehicle)
{
    return new CustomAutoPilotPlugin(vehicle, vehicle);
}

const QVariantList& CustomFirmwarePlugin::toolIndicators(const Vehicle* vehicle)
{
    if (_toolIndicatorList.size() == 0) {
        // First call the base class to get the standard QGC list. This way we are guaranteed to always get
        // any new toolbar indicators which are added upstream in our custom build.
        _toolIndicatorList = FirmwarePlugin::toolIndicators(vehicle);
        // Then specifically remove the RC RSSI indicator.
        _toolIndicatorList.removeOne(QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/RCRSSIIndicator.qml")));
    }
    return _toolIndicatorList;
}

// Tells QGC that your vehicle has a gimbal on it. This will in turn cause thing like gimbal commands to point
// the camera straight down for surveys to be automatically added to Plans.
bool CustomFirmwarePlugin::hasGimbal(Vehicle* /*vehicle*/, bool& rollSupported, bool& pitchSupported, bool& yawSupported)
{
    rollSupported = false;
    pitchSupported = true;
    yawSupported = true;

    return true;
}

void CustomFirmwarePlugin::updateAvailableFlightModes(FlightModeList modeList)
{
    _availableFlightModeList.clear();
    for(auto mode: modeList){

        // Update Multi Rotor
        switch (mode.custom_mode) {
        case PX4CustomMode::POSCTL_ORBIT:
            mode.multiRotor = false;
            break;
        default:
            mode.multiRotor = true;
            break;
        }

        // Update Fixed Wing
        switch (mode.custom_mode){
        case PX4CustomMode::OFFBOARD:
        case PX4CustomMode::SIMPLE:
        case PX4CustomMode::POSCTL_ORBIT:
        case PX4CustomMode::AUTO_FOLLOW_TARGET:
        case PX4CustomMode::AUTO_PRECLAND:
            mode.fixedWing = false;
            break;
        default:
            mode.fixedWing = true;
        }

        // Update CanBeSet
        switch (mode.custom_mode){
        case PX4CustomMode::AUTO_LOITER:
        case PX4CustomMode::AUTO_RTL:
        case PX4CustomMode::AUTO_MISSION:
            mode.canBeSet = true;
            break;
        default:
            mode.canBeSet = false;
        }

        _updateModeMappings(mode);
    }
}