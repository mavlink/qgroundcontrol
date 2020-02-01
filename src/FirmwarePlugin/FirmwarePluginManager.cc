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

QList<MAV_AUTOPILOT> FirmwarePluginManager::supportedFirmwareTypes(void)
{
    if (_supportedFirmwareTypes.isEmpty()) {
        QList<FirmwarePluginFactory*> factoryList = FirmwarePluginFactoryRegister::instance()->pluginFactories();
        for (int i = 0; i < factoryList.count(); i++) {
            _supportedFirmwareTypes.append(factoryList[i]->supportedFirmwareTypes());
        }
        _supportedFirmwareTypes.append(MAV_AUTOPILOT_GENERIC);
    }
    return _supportedFirmwareTypes;
}

QList<MAV_TYPE> FirmwarePluginManager::supportedVehicleTypes(MAV_AUTOPILOT firmwareType)
{
    QList<MAV_TYPE> vehicleTypes;

    FirmwarePluginFactory* factory = _findPluginFactory(firmwareType);

    if (factory) {
        vehicleTypes = factory->supportedVehicleTypes();
    } else if (firmwareType == MAV_AUTOPILOT_GENERIC) {
        vehicleTypes << MAV_TYPE_FIXED_WING << MAV_TYPE_QUADROTOR << MAV_TYPE_VTOL_QUADROTOR << MAV_TYPE_GROUND_ROVER << MAV_TYPE_SUBMARINE;
    } else {
        qWarning() << "Request for unknown firmware plugin factory" << firmwareType;
    }

    return vehicleTypes;
}

FirmwarePlugin* FirmwarePluginManager::firmwarePluginForAutopilot(MAV_AUTOPILOT firmwareType, MAV_TYPE vehicleType)
{
    FirmwarePluginFactory*  factory = _findPluginFactory(firmwareType);
    FirmwarePlugin*         plugin = nullptr;

    if (factory) {
        plugin = factory->firmwarePluginForAutopilot(firmwareType, vehicleType);
    } else if (firmwareType != MAV_AUTOPILOT_GENERIC) {
        qWarning() << "Request for unknown firmware plugin factory" << firmwareType;
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

FirmwarePluginFactory* FirmwarePluginManager::_findPluginFactory(MAV_AUTOPILOT firmwareType)
{
    QList<FirmwarePluginFactory*> factoryList = FirmwarePluginFactoryRegister::instance()->pluginFactories();

    // Find the plugin which supports this vehicle
    for (int i=0; i<factoryList.count(); i++) {
        FirmwarePluginFactory* factory = factoryList[i];
        if (factory->supportedFirmwareTypes().contains(firmwareType)) {
            return factory;
        }
    }

    return nullptr;
}
