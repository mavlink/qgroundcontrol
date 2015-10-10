/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "FirmwarePluginManager.h"
#include "Generic/GenericFirmwarePlugin.h"
#include "APM/ArduCopterFirmwarePlugin.h"
#include "PX4/PX4FirmwarePlugin.h"

IMPLEMENT_QGC_SINGLETON(FirmwarePluginManager, FirmwarePluginManager)

FirmwarePluginManager::FirmwarePluginManager(QObject* parent) :
    QGCSingleton(parent)
{

}

FirmwarePluginManager::~FirmwarePluginManager()
{

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
                    return ArduCopterFirmwarePlugin::instance();
                    break;
                // FIXME: The remainder of these need to be correctly assigned and new plugin classes created as needed.
                // Once done, the unused cases can be removed and just the fall back default: left
                case MAV_TYPE_FIXED_WING:
                case MAV_TYPE_GENERIC:
                case MAV_TYPE_COAXIAL:
                case MAV_TYPE_HELICOPTER:
                case MAV_TYPE_ANTENNA_TRACKER:
                case MAV_TYPE_GCS:
                case MAV_TYPE_AIRSHIP:
                case MAV_TYPE_FREE_BALLOON:
                case MAV_TYPE_ROCKET:
                case MAV_TYPE_GROUND_ROVER:
                case MAV_TYPE_SURFACE_BOAT:
                case MAV_TYPE_SUBMARINE:
                case MAV_TYPE_FLAPPING_WING:
                case MAV_TYPE_KITE:
                case MAV_TYPE_ONBOARD_CONTROLLER:
                case MAV_TYPE_VTOL_DUOROTOR:
                case MAV_TYPE_VTOL_QUADROTOR:
                case MAV_TYPE_VTOL_TILTROTOR:
                case MAV_TYPE_VTOL_RESERVED2:
                case MAV_TYPE_VTOL_RESERVED3:
                case MAV_TYPE_VTOL_RESERVED4:
                case MAV_TYPE_VTOL_RESERVED5:
                case MAV_TYPE_GIMBAL:
                default:
                    return GenericFirmwarePlugin::instance();
                    break;
            }
        case MAV_AUTOPILOT_PX4:
            return PX4FirmwarePlugin::instance();
        default:
            return GenericFirmwarePlugin::instance();
    }
}
