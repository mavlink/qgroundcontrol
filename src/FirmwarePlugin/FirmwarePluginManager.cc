/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "FirmwarePluginManager.h"
#include "APM/ArduCopterFirmwarePlugin.h"
#include "APM/ArduPlaneFirmwarePlugin.h"
#include "APM/ArduRoverFirmwarePlugin.h"
#include "PX4/PX4FirmwarePlugin.h"

FirmwarePluginManager::FirmwarePluginManager(QGCApplication* app)
    : QGCTool(app)
    , _arduCopterFirmwarePlugin(NULL)
    , _arduPlaneFirmwarePlugin(NULL)
    , _arduRoverFirmwarePlugin(NULL)
    , _genericFirmwarePlugin(NULL)
    , _px4FirmwarePlugin(NULL)
{

}

FirmwarePluginManager::~FirmwarePluginManager()
{
    delete _arduCopterFirmwarePlugin;
    delete _arduPlaneFirmwarePlugin;
    delete _arduRoverFirmwarePlugin;
    delete _genericFirmwarePlugin;
    delete _px4FirmwarePlugin;
}

FirmwarePlugin* FirmwarePluginManager::firmwarePluginForAutopilot(MAV_AUTOPILOT autopilotType, MAV_TYPE vehicleType)
{
    switch (autopilotType) {
    case MAV_AUTOPILOT_ARDUPILOTMEGA:
        switch (vehicleType) {
        case MAV_TYPE_QUADROTOR:
        case MAV_TYPE_HEXAROTOR:
        case MAV_TYPE_OCTOROTOR:
        case MAV_TYPE_TRICOPTER:
        case MAV_TYPE_COAXIAL:
        case MAV_TYPE_HELICOPTER:
            if (!_arduCopterFirmwarePlugin) {
                _arduCopterFirmwarePlugin = new ArduCopterFirmwarePlugin;
            }
            return _arduCopterFirmwarePlugin;
        case MAV_TYPE_FIXED_WING:
            if (!_arduPlaneFirmwarePlugin) {
                _arduPlaneFirmwarePlugin = new ArduPlaneFirmwarePlugin;
            }
            return _arduPlaneFirmwarePlugin;
        case MAV_TYPE_GROUND_ROVER:
        case MAV_TYPE_SURFACE_BOAT:
        case MAV_TYPE_SUBMARINE:
            if (!_arduRoverFirmwarePlugin) {
                _arduRoverFirmwarePlugin = new ArduRoverFirmwarePlugin;
            }
            return _arduRoverFirmwarePlugin;
        default:
            break;
        }
    case MAV_AUTOPILOT_PX4:
        if (!_px4FirmwarePlugin) {
            _px4FirmwarePlugin = new PX4FirmwarePlugin;
        }
        return _px4FirmwarePlugin;
    default:
        break;
    }

    if (!_genericFirmwarePlugin) {
        _genericFirmwarePlugin = new FirmwarePlugin;
    }
    return _genericFirmwarePlugin;
}

void FirmwarePluginManager::clearSettings(void)
{
    // FIXME: NYI
}
