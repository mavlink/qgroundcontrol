/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>

#include "QGCMAVLink.h"

class FirmwarePlugin;
class FirmwarePluginFactory;

Q_DECLARE_LOGGING_CATEGORY(FirmwarePluginManagerLog)

/// FirmwarePluginManager is a singleton which is used to return the correct FirmwarePlugin for a MAV_AUTOPILOT type.
class FirmwarePluginManager : public QObject
{
    Q_OBJECT

public:
    /// Constructs an FirmwarePluginManager object.
    ///     @param parent The parent QObject.
    explicit FirmwarePluginManager(QObject *parent = nullptr);

    /// Destructor for the FirmwarePluginManager class.
    ~FirmwarePluginManager();

    /// Gets the singleton instance of FirmwarePluginManager.
    ///     @return The singleton instance.
    static FirmwarePluginManager *instance();

    /// Returns list of firmwares which are supported by the system
    QList<QGCMAVLink::FirmwareClass_t> supportedFirmwareClasses();

    /// Returns the list of supported vehicle types for the specified firmware
    QList<QGCMAVLink::VehicleClass_t> supportedVehicleClasses(QGCMAVLink::FirmwareClass_t firmwareClass);

    /// Returns appropriate plugin for autopilot type.
    ///     @param firmwareType Type of firmwware to return plugin for.
    ///     @param vehicleType Vehicle type to return plugin for.
    /// @return Singleton FirmwarePlugin instance for the specified MAV_AUTOPILOT.
    FirmwarePlugin *firmwarePluginForAutopilot(MAV_AUTOPILOT firmwareType, MAV_TYPE vehicleType);

private:
    FirmwarePluginFactory *_findPluginFactory(QGCMAVLink::FirmwareClass_t firmwareClass);

    FirmwarePlugin *_genericFirmwarePlugin = nullptr;
    QList<QGCMAVLink::FirmwareClass_t> _supportedFirmwareClasses;
};
