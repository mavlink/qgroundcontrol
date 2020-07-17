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

#include "FirmwarePluginManager.h"
#include "FirmwarePlugin.h"

FirmwarePluginManager::FirmwarePluginManager(QGCApplication* app, QGCToolbox* toolbox)
    : QGCTool(app, toolbox)
    , _genericFirmwarePlugin(nullptr)
{

}

FirmwarePluginManager::~FirmwarePluginManager()
{
    delete _genericFirmwarePlugin;
}

QList<QGCMAVLink::FirmwareClass_t> FirmwarePluginManager::supportedFirmwareClasses(void)
{
    if (_supportedFirmwareClasses.isEmpty()) {
        QList<FirmwarePluginFactory*> factoryList = FirmwarePluginFactoryRegister::instance()->pluginFactories();
        for (int i = 0; i < factoryList.count(); i++) {
            _supportedFirmwareClasses.append(factoryList[i]->supportedFirmwareClasses());
        }
        _supportedFirmwareClasses.append(QGCMAVLink::FirmwareClassGeneric);
    }
    return _supportedFirmwareClasses;
}

QList<QGCMAVLink::VehicleClass_t> FirmwarePluginManager::supportedVehicleClasses(QGCMAVLink::FirmwareClass_t firmwareClass)
{
    QList<QGCMAVLink::VehicleClass_t> vehicleClasses;

    FirmwarePluginFactory* factory = _findPluginFactory(firmwareClass);

    if (factory) {
        vehicleClasses = factory->supportedVehicleClasses();
    } else if (firmwareClass == QGCMAVLink::FirmwareClassGeneric) {
        // Generic supports all specific vehicle class
        vehicleClasses = QGCMAVLink::allVehicleClasses();
        vehicleClasses.removeOne(QGCMAVLink::VehicleClassGeneric);
    } else {
        qWarning() << "Request for unknown firmware plugin factory" << firmwareClass;
    }

    return vehicleClasses;
}

FirmwarePlugin* FirmwarePluginManager::firmwarePluginForAutopilot(MAV_AUTOPILOT firmwareType, MAV_TYPE vehicleType)
{
    FirmwarePluginFactory*  factory = _findPluginFactory(firmwareType);
    FirmwarePlugin*         plugin = nullptr;

    if (factory) {
        plugin = factory->firmwarePluginForAutopilot(firmwareType, vehicleType);
    }

    if (!plugin) {
        // Default plugin fallback
        if (!_genericFirmwarePlugin) {
            _genericFirmwarePlugin = new FirmwarePlugin;
        }
        plugin = _genericFirmwarePlugin;
    }

    return plugin;
}

FirmwarePluginFactory* FirmwarePluginManager::_findPluginFactory(QGCMAVLink::FirmwareClass_t firmwareClass)
{
    QList<FirmwarePluginFactory*> factoryList = FirmwarePluginFactoryRegister::instance()->pluginFactories();

    // Find the plugin which supports this vehicle
    for (int i=0; i<factoryList.count(); i++) {
        FirmwarePluginFactory* factory = factoryList[i];
        if (factory->supportedFirmwareClasses().contains(firmwareClass)) {
            return factory;
        }
    }

    return nullptr;
}
