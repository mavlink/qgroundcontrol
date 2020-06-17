/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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

/// FirmwarePluginManager is a singleton which is used to return the correct FirmwarePlugin for a MAV_AUTOPILOT type.

class FirmwarePluginManager : public QGCTool
{
    Q_OBJECT

public:
    FirmwarePluginManager(QGCApplication* app, QGCToolbox* toolbox);
    ~FirmwarePluginManager();

    /// Returns list of firmwares which are supported by the system
    QList<MAV_AUTOPILOT> supportedFirmwareTypes(void);

    /// Returns the list of supported vehicle types for the specified firmware
    QList<MAV_TYPE> supportedVehicleTypes(MAV_AUTOPILOT firmwareType);

    /// Returns appropriate plugin for autopilot type.
    ///     @param firmwareType Type of firmwware to return plugin for.
    ///     @param vehicleType Vehicle type to return plugin for.
    /// @return Singleton FirmwarePlugin instance for the specified MAV_AUTOPILOT.
    FirmwarePlugin* firmwarePluginForAutopilot(MAV_AUTOPILOT firmwareType, MAV_TYPE vehicleType);

private:
    FirmwarePluginFactory* _findPluginFactory(MAV_AUTOPILOT firmwareType);

    FirmwarePlugin*         _genericFirmwarePlugin;
    QList<MAV_AUTOPILOT>    _supportedFirmwareTypes;
};

#endif
