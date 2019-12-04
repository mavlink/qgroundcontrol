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
#include "CustomCameraManager.h"
#include "CustomCameraControl.h"

//-----------------------------------------------------------------------------
CustomFirmwarePlugin::CustomFirmwarePlugin()
{
    for (int i = 0; i < _flightModeInfoList.count(); i++) {
        FlightModeInfo_t& info = _flightModeInfoList[i];
        //-- Narrow the options to only these two
        if (info.name != _altCtlFlightMode &&
            info.name != _posCtlFlightMode) {
            info.canBeSet = false;
        }
    }
}

//-----------------------------------------------------------------------------
AutoPilotPlugin* CustomFirmwarePlugin::autopilotPlugin(Vehicle* vehicle)
{
    return new CustomAutoPilotPlugin(vehicle, vehicle);
}

//-----------------------------------------------------------------------------
QGCCameraManager*
CustomFirmwarePlugin::createCameraManager(Vehicle *vehicle)
{
    return new CustomCameraManager(vehicle);
}

//-----------------------------------------------------------------------------
QGCCameraControl*
CustomFirmwarePlugin::createCameraControl(const mavlink_camera_information_t* info, Vehicle *vehicle, int compID, QObject* parent)
{
    return new CustomCameraControl(info, vehicle, compID, parent);
}

//-----------------------------------------------------------------------------
const QVariantList&
CustomFirmwarePlugin::toolBarIndicators(const Vehicle*)
{
    if(_toolBarIndicatorList.size() == 0) {
#if defined(QGC_ENABLE_PAIRING)
        _toolBarIndicatorList.append(QVariant::fromValue(QUrl::fromUserInput("qrc:/custom/PairingIndicator.qml")));
#endif
        _toolBarIndicatorList.append(QVariant::fromValue(QUrl::fromUserInput("qrc:/custom/CustomROIIndicator.qml")));
        _toolBarIndicatorList.append(QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/GPSIndicator.qml")));
        _toolBarIndicatorList.append(QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/TelemetryRSSIIndicator.qml")));
        _toolBarIndicatorList.append(QVariant::fromValue(QUrl::fromUserInput("qrc:/custom/CustomRCRSSIIndicator.qml")));
        _toolBarIndicatorList.append(QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/BatteryIndicator.qml")));
    }
    return _toolBarIndicatorList;
}

//-----------------------------------------------------------------------------
QStringList
CustomFirmwarePlugin::joystickFlightModes(Vehicle* vehicle)
{
    QStringList flightModes;
    foreach (const FlightModeInfo_t& info, _flightModeInfoList) {
        bool fw = (vehicle->fixedWing() && info.fixedWing);
        bool mc = (vehicle->multiRotor() && info.multiRotor);
        // show all modes for generic, vtol, etc
        bool other = !vehicle->fixedWing() && !vehicle->multiRotor();
        if (fw || mc || other) {
            flightModes += *info.name;
        }
    }
    return flightModes;
}
