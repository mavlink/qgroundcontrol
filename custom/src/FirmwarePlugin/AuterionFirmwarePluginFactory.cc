/*!
 * @file
 *   @brief Auterion Firmware Plugin Factory (PX4)
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

#include "AuterionFirmwarePluginFactory.h"
#include "AuterionFirmwarePlugin.h"

AuterionFirmwarePluginFactory AuterionFirmwarePluginFactoryImp;

AuterionFirmwarePluginFactory::AuterionFirmwarePluginFactory()
    : _pluginInstance(nullptr)
{
}

QList<MAV_AUTOPILOT> AuterionFirmwarePluginFactory::supportedFirmwareTypes() const
{
    QList<MAV_AUTOPILOT> list;
    list.append(MAV_AUTOPILOT_PX4);
    return list;
}

FirmwarePlugin* AuterionFirmwarePluginFactory::firmwarePluginForAutopilot(MAV_AUTOPILOT autopilotType, MAV_TYPE vehicleType)
{
    Q_UNUSED(vehicleType);
    if (autopilotType == MAV_AUTOPILOT_PX4) {
        if (!_pluginInstance) {
            _pluginInstance = new AuterionFirmwarePlugin;
        }
        return _pluginInstance;
    }
    return nullptr;
}
