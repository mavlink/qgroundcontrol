/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "FirmwarePluginManager.h"
#include "FirmwarePlugin.h"
#include "FirmwarePluginFactory.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QGlobalStatic>

QGC_LOGGING_CATEGORY(FirmwarePluginManagerLog, "FirmwarePlugin.FirmwarePluginManager");

Q_GLOBAL_STATIC(FirmwarePluginManager, _firmwarePluginManagerInstance);

FirmwarePluginManager::FirmwarePluginManager(QObject *parent)
    : QObject(parent)
{
    // qCDebug(FirmwarePluginManagerLog) << Q_FUNC_INFO << this;
}

FirmwarePluginManager::~FirmwarePluginManager()
{
    // qCDebug(FirmwarePluginManagerLog) << Q_FUNC_INFO << this;
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
    const QList<FirmwarePluginFactory*> factoryList = FirmwarePluginFactoryRegister::instance()->pluginFactories();

    for (int i = factoryList.count() - 1; i >= 0; --i) {
        const FirmwarePluginFactory *const factory = factoryList[i];
        if (!factory->supportedFirmwareClasses().contains(firmwareClass)) {
            continue;
        }

        const QList<QGCMAVLink::VehicleClass_t> supportedVehicleClasses = factory->supportedVehicleClasses();
        for (const QGCMAVLink::VehicleClass_t vehicleClass : supportedVehicleClasses) {
            if (!vehicleClasses.contains(vehicleClass)) {
                vehicleClasses.append(vehicleClass);
            }
        }
    }

    if (vehicleClasses.isEmpty()) {
        if (firmwareClass == QGCMAVLink::FirmwareClassGeneric) {
            // Generic supports all specific vehicle class
            vehicleClasses = QGCMAVLink::allVehicleClasses();
            (void) vehicleClasses.removeOne(QGCMAVLink::VehicleClassGeneric);
        } else {
            qCWarning(FirmwarePluginManagerLog) << "Request for unknown firmware plugin factory" << firmwareClass;
        }
    }

    return vehicleClasses;
}

FirmwarePlugin *FirmwarePluginManager::firmwarePluginForAutopilot(MAV_AUTOPILOT firmwareType, MAV_TYPE vehicleType)
{
    const QGCMAVLink::FirmwareClass_t firmwareClass = QGCMAVLink::firmwareClass(firmwareType);
    const QGCMAVLink::VehicleClass_t vehicleClass = QGCMAVLink::vehicleClass(vehicleType);
    FirmwarePluginFactory *const factory = _findPluginFactory(firmwareClass, vehicleClass);
    FirmwarePlugin *plugin = nullptr;

    if (factory) {
        plugin = factory->firmwarePluginForAutopilot(firmwareType, vehicleType);
    }

    if (!plugin) {
        if (!_genericFirmwarePlugin) {
            _genericFirmwarePlugin = new FirmwarePlugin(this);
        }
        plugin = _genericFirmwarePlugin;
    }

    return plugin;
}

FirmwarePluginFactory *FirmwarePluginManager::_findPluginFactory(QGCMAVLink::FirmwareClass_t firmwareClass,
                                                                 QGCMAVLink::VehicleClass_t vehicleClass)
{
    const QList<FirmwarePluginFactory*> factoryList = FirmwarePluginFactoryRegister::instance()->pluginFactories();
    FirmwarePluginFactory *firmwareMatch = nullptr;

    for (int i = factoryList.count() - 1; i >= 0; --i) {
        FirmwarePluginFactory *const factory = factoryList[i];
        if (!factory->supportedFirmwareClasses().contains(firmwareClass)) {
            continue;
        }

        if (!firmwareMatch) {
            firmwareMatch = factory;
        }

        const QList<QGCMAVLink::VehicleClass_t> supportedVehicleClasses = factory->supportedVehicleClasses();
        if (supportedVehicleClasses.contains(vehicleClass)) {
            return factory;
        }
    }

    return firmwareMatch;
}
