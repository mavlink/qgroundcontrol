/*!
 * @file
 *   @brief Yuneec Firmware Plugin Factory (PX4)
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

#include "YuneecFirmwarePluginFactory.h"
#include "YuneecFirmwarePlugin.h"

YuneecFirmwarePluginFactory YuneecFirmwarePluginFactoryImp;

YuneecFirmwarePluginFactory::YuneecFirmwarePluginFactory(void)
    : _pluginInstance(NULL)
{
}

QList<MAV_AUTOPILOT> YuneecFirmwarePluginFactory::supportedFirmwareTypes(void) const
{
    QList<MAV_AUTOPILOT> list;
    list.append(MAV_AUTOPILOT_PX4);
    return list;
}

FirmwarePlugin* YuneecFirmwarePluginFactory::firmwarePluginForAutopilot(MAV_AUTOPILOT autopilotType, MAV_TYPE vehicleType)
{
    Q_UNUSED(vehicleType);
    if (autopilotType == MAV_AUTOPILOT_PX4) {
        if (!_pluginInstance) {
            _pluginInstance = new YuneecFirmwarePlugin;
        }
        return _pluginInstance;
    }
    return NULL;
}

QList<MAV_TYPE> YuneecFirmwarePluginFactory::supportedVehicleTypes(void) const
{
    QList<MAV_TYPE> mavTypes;
    mavTypes.append(MAV_TYPE_HEXAROTOR);
    return mavTypes;
}
