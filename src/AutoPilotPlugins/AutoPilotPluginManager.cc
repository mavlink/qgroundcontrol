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
#include "Generic/GenericAutoPilotPlugin.h"
#include "QGCApplication.h"

IMPLEMENT_QGC_SINGLETON(AutoPilotPluginManager, AutoPilotPluginManager)

AutoPilotPluginManager::AutoPilotPluginManager(QObject* parent) :
    QGCSingleton(parent)
{
    // All plugins are constructed here so that they end up on the correct thread
    _pluginMap[MAV_AUTOPILOT_PX4] = new PX4AutoPilotPlugin(this);
    Q_ASSERT(_pluginMap.contains(MAV_AUTOPILOT_PX4));

    _pluginMap[MAV_AUTOPILOT_GENERIC] = new GenericAutoPilotPlugin(this);
    Q_ASSERT(_pluginMap.contains(MAV_AUTOPILOT_GENERIC));
}

AutoPilotPlugin* AutoPilotPluginManager::getInstanceForAutoPilotPlugin(int autopilotType)
{
    switch (autopilotType) {
        case MAV_AUTOPILOT_PX4:
            Q_ASSERT(_pluginMap.contains(MAV_AUTOPILOT_PX4));
            return _pluginMap[MAV_AUTOPILOT_PX4];
            
        default:
            Q_ASSERT(_pluginMap.contains(MAV_AUTOPILOT_GENERIC));
            return _pluginMap[MAV_AUTOPILOT_GENERIC];
    }
}
