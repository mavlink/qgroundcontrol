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
#include "PX4/PX4FirmwarePlugin.h"

IMPLEMENT_QGC_SINGLETON(FirmwarePluginManager, FirmwarePluginManager)

FirmwarePluginManager::FirmwarePluginManager(QObject* parent) :
    QGCSingleton(parent)
{

}

FirmwarePluginManager::~FirmwarePluginManager()
{

}

FirmwarePlugin* FirmwarePluginManager::firmwarePluginForAutopilot(MAV_AUTOPILOT autopilotType)
{
    if (autopilotType == MAV_AUTOPILOT_PX4) {
        return PX4FirmwarePlugin::instance();
    } else {
        return GenericFirmwarePlugin::instance();
    }
}
