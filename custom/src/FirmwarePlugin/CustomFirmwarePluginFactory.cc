/****************************************************************************
 *
 * (c) 2009-2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 * @file
 *   @brief Custom Firmware Plugin Factory (PX4)
 *   @author Gus Grubba <gus@auterion.com>
 *
 */

#include "CustomFirmwarePluginFactory.h"
#include "CustomFirmwarePlugin.h"

CustomFirmwarePluginFactory CustomFirmwarePluginFactoryImp;

CustomFirmwarePluginFactory::CustomFirmwarePluginFactory()
    : _pluginInstance(nullptr)
{
}

QList<MAV_AUTOPILOT> CustomFirmwarePluginFactory::supportedFirmwareTypes() const
{
    QList<MAV_AUTOPILOT> list;
    list.append(MAV_AUTOPILOT_PX4);
    return list;
}

FirmwarePlugin* CustomFirmwarePluginFactory::firmwarePluginForAutopilot(MAV_AUTOPILOT autopilotType, MAV_TYPE vehicleType)
{
    Q_UNUSED(vehicleType);
    if (autopilotType == MAV_AUTOPILOT_PX4) {
        if (!_pluginInstance) {
            _pluginInstance = new CustomFirmwarePlugin;
        }
        return _pluginInstance;
    }
    return nullptr;
}
