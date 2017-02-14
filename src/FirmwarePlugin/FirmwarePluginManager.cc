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

#include "FirmwarePluginManager.h"
#include "FirmwarePlugin.h"

FirmwarePluginManager::FirmwarePluginManager(QGCApplication* app)
    : QGCTool(app)
    , _genericFirmwarePlugin(NULL)
{

}

FirmwarePluginManager::~FirmwarePluginManager()
{
    delete _genericFirmwarePlugin;
}

QList<MAV_AUTOPILOT> FirmwarePluginManager::knownFirmwareTypes(void)
{
    if (_knownFirmwareTypes.isEmpty()) {
        QList<FirmwarePluginFactory*> factoryList = FirmwarePluginFactoryRegister::instance()->pluginFactories();
        for (int i = 0; i < factoryList.count(); i++) {
            _knownFirmwareTypes.append(factoryList[i]->knownFirmwareTypes());
        }
        _knownFirmwareTypes.append(MAV_AUTOPILOT_GENERIC);
    }
    return _knownFirmwareTypes;
}

FirmwarePlugin* FirmwarePluginManager::firmwarePluginForAutopilot(MAV_AUTOPILOT autopilotType, MAV_TYPE vehicleType)
{
    FirmwarePlugin* _plugin = NULL;
    QList<FirmwarePluginFactory*> factoryList = FirmwarePluginFactoryRegister::instance()->pluginFactories();

    // Find the plugin which supports this vehicle
    for (int i=0; i<factoryList.count(); i++) {
        if ((_plugin = factoryList[i]->firmwarePluginForAutopilot(autopilotType, vehicleType))) {
            return _plugin;
        }
    }

    // Default plugin fallback
    if (!_genericFirmwarePlugin) {
        _genericFirmwarePlugin = new FirmwarePlugin;
    }
    return _genericFirmwarePlugin;
}
