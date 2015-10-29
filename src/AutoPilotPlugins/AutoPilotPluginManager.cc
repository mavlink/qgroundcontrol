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
