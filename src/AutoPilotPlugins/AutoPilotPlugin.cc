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

#include "AutoPilotPlugin.h"
#include "PX4/PX4AutoPilotPlugin.h"
#include "Generic/GenericAutoPilotPlugin.h"

static AutoPilotPlugin* PX4_AutoPilot = NULL;       ///< Singleton plugin for MAV_AUTOPILOT_PX4
static AutoPilotPlugin* Generic_AutoPilot = NULL;   ///< Singleton plugin for AutoPilots which do not have a specifically implemented plugin

AutoPilotPlugin::AutoPilotPlugin(void)
{
    
}

AutoPilotPlugin* AutoPilotPlugin::getInstanceForAutoPilotPlugin(int autopilotType)
{
    switch (autopilotType) {
        case MAV_AUTOPILOT_PX4:
            if (PX4_AutoPilot == NULL) {
                PX4_AutoPilot = new PX4AutoPilotPlugin;
            }
            Q_ASSERT(PX4_AutoPilot);
            return PX4_AutoPilot;
            
        default:
            if (Generic_AutoPilot == NULL) {
                Generic_AutoPilot = new GenericAutoPilotPlugin;
            }
            Q_ASSERT(Generic_AutoPilot);
            return Generic_AutoPilot;
    }
}
