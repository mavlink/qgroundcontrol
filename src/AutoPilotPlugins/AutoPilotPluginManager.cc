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

#include "AutoPilotPluginManager.h"
#include "PX4/PX4AutoPilotPlugin.h"
#include "APM/APMAutoPilotPlugin.h"
#include "Generic/GenericAutoPilotPlugin.h"

AutoPilotPlugin* AutoPilotPluginManager::newAutopilotPluginForVehicle(Vehicle* vehicle)
{
    switch (vehicle->firmwareType()) {
        case MAV_AUTOPILOT_PX4:
            return new PX4AutoPilotPlugin(vehicle, vehicle);
        case MAV_AUTOPILOT_ARDUPILOTMEGA:
            return new APMAutoPilotPlugin(vehicle, vehicle);
        default:
            return new GenericAutoPilotPlugin(vehicle, vehicle);
    }
}
