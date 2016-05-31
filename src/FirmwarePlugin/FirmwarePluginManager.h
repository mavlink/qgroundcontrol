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
