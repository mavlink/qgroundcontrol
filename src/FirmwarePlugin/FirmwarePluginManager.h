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

#ifndef FirmwarePluginManager_H
#define FirmwarePluginManager_H

#include <QObject>

#include "FirmwarePlugin.h"
#include "QGCMAVLink.h"
#include "QGCToolbox.h"

class QGCApplication;
class ArduCopterFirmwarePlugin;
class ArduPlaneFirmwarePlugin;
class ArduRoverFirmwarePlugin;
class PX4FirmwarePlugin;

/// FirmwarePluginManager is a singleton which is used to return the correct FirmwarePlugin for a MAV_AUTOPILOT type.

class FirmwarePluginManager : public QGCTool
{
    Q_OBJECT

public:
    FirmwarePluginManager(QGCApplication* app);
    ~FirmwarePluginManager();

    /// Returns appropriate plugin for autopilot type.
    ///     @param autopilotType Type of autopilot to return plugin for.
    ///     @param vehicleType Vehicle type of autopilot to return plugin for.
    /// @return Singleton FirmwarePlugin instance for the specified MAV_AUTOPILOT.
    FirmwarePlugin* firmwarePluginForAutopilot(MAV_AUTOPILOT autopilotType, MAV_TYPE vehicleType);

    /// Clears settings from all firmware plugins.
    void clearSettings(void);

private:
    ArduCopterFirmwarePlugin*   _arduCopterFirmwarePlugin;
    ArduPlaneFirmwarePlugin*    _arduPlaneFirmwarePlugin;
    ArduRoverFirmwarePlugin*    _arduRoverFirmwarePlugin;
    FirmwarePlugin*             _genericFirmwarePlugin;
    PX4FirmwarePlugin*          _px4FirmwarePlugin;
};

#endif
