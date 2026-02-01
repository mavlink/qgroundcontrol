#pragma once

#include <QtCore/QList>

#include "QGCMAVLink.h"

class FirmwarePlugin;


class FirmwarePluginFactory : public QObject
{
    Q_OBJECT

public:
    explicit FirmwarePluginFactory(QObject *parent = nullptr);
    virtual ~FirmwarePluginFactory();

    /// Returns appropriate plugin for autopilot type.
    ///     @param autopilotType Type of autopilot to return plugin for.
    ///     @param vehicleType Vehicle type of autopilot to return plugin for.
    /// @return Singleton FirmwarePlugin instance for the specified MAV_AUTOPILOT.
    virtual FirmwarePlugin *firmwarePluginForAutopilot(MAV_AUTOPILOT autopilotType, MAV_TYPE vehicleType) = 0;

    /// @return List of firmware classes this plugin supports.
    virtual QList<QGCMAVLink::FirmwareClass_t> supportedFirmwareClasses() const = 0;

    /// @return List of vehicle classes this plugin supports.
    virtual QList<QGCMAVLink::VehicleClass_t> supportedVehicleClasses() const { return QGCMAVLink::allVehicleClasses(); }
};

/*===========================================================================*/

class FirmwarePluginFactoryRegister : public QObject
{
    Q_OBJECT

public:
    static FirmwarePluginFactoryRegister *instance();

    /// Registers the specified logging category to the system.
    void registerPluginFactory(FirmwarePluginFactory *pluginFactory) { _factoryList.append(pluginFactory); }

    QList<FirmwarePluginFactory*> pluginFactories() const { return _factoryList; }

private:
    QList<FirmwarePluginFactory*> _factoryList;
};
