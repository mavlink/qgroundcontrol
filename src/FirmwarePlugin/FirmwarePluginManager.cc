/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "FirmwarePluginManager.h"
#include "FirmwarePlugin.h"
#include "FirmwarePluginFactory.h"
#include "QGCLoggingCategory.h"

#include <QtCore/qapplicationstatic.h>

QGC_LOGGING_CATEGORY(FirmwarePluginManagerLog, "qgc.firmwareplugin.firmwarepluginmanager");

Q_APPLICATION_STATIC(FirmwarePluginManager, _firmwarePluginManagerInstance);

FirmwarePluginManager::FirmwarePluginManager(QObject *parent)
    : QObject(parent)
{
    // qCDebug(FirmwarePluginManagerLog) << Q_FUNC_INFO << this;
}

FirmwarePluginManager::~FirmwarePluginManager()
{
    // qCDebug(FirmwarePluginManagerLog) << Q_FUNC_INFO << this;

    delete _genericFirmwarePlugin;
}

FirmwarePluginManager *FirmwarePluginManager::instance()
{
    return _firmwarePluginManagerInstance();
}

QList<QGCMAVLink::FirmwareClass_t> FirmwarePluginManager::supportedFirmwareClasses()
{
    if (_supportedFirmwareClasses.isEmpty()) {
        const QList<FirmwarePluginFactory*> factoryList = FirmwarePluginFactoryRegister::instance()->pluginFactories();
        for (const FirmwarePluginFactory *factory: factoryList) {
            _supportedFirmwareClasses.append(factory->supportedFirmwareClasses());
        }
        _supportedFirmwareClasses.append(QGCMAVLink::FirmwareClassGeneric);
    }

    return _supportedFirmwareClasses;
}

QList<QGCMAVLink::VehicleClass_t> FirmwarePluginManager::supportedVehicleClasses(QGCMAVLink::FirmwareClass_t firmwareClass)
{
    QList<QGCMAVLink::VehicleClass_t> vehicleClasses;
    const FirmwarePluginFactory *const factory = _findPluginFactory(firmwareClass);

    if (factory) {
        vehicleClasses = factory->supportedVehicleClasses();
    } else if (firmwareClass == QGCMAVLink::FirmwareClassGeneric) {
        // Generic supports all specific vehicle class
        vehicleClasses = QGCMAVLink::allVehicleClasses();
        (void) vehicleClasses.removeOne(QGCMAVLink::VehicleClassGeneric);
    } else {
        qCWarning(FirmwarePluginManagerLog) << "Request for unknown firmware plugin factory" << firmwareClass;
    }

    return vehicleClasses;
}

FirmwarePlugin *FirmwarePluginManager::firmwarePluginForAutopilot(MAV_AUTOPILOT firmwareType, MAV_TYPE vehicleType)
{
    FirmwarePluginFactory *const factory = _findPluginFactory(firmwareType);
    FirmwarePlugin *plugin = nullptr;

    if (factory) {
        plugin = factory->firmwarePluginForAutopilot(firmwareType, vehicleType);
    }

    if (!plugin) {
        // Default plugin fallback
        if (!_genericFirmwarePlugin) {
            _genericFirmwarePlugin = new FirmwarePlugin();
        }
        plugin = _genericFirmwarePlugin;
    }

    return plugin;
}

FirmwarePluginFactory *FirmwarePluginManager::_findPluginFactory(QGCMAVLink::FirmwareClass_t firmwareClass)
{
    const QList<FirmwarePluginFactory*> factoryList = FirmwarePluginFactoryRegister::instance()->pluginFactories();

    // Find the plugin which supports this vehicle
    for (FirmwarePluginFactory *factory: factoryList) {
        if (factory->supportedFirmwareClasses().contains(firmwareClass)) {
            return factory;
        }
    }

    return nullptr;
}
