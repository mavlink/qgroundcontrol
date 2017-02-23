/*!
 * @file
 *   @brief Yuneec Firmware Plugin Factory (PX4)
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

#pragma once

#include "FirmwarePlugin.h"

class YuneecFirmwarePlugin;

class YuneecFirmwarePluginFactory : public FirmwarePluginFactory
{
    Q_OBJECT
public:
    YuneecFirmwarePluginFactory(void);
    QList<MAV_AUTOPILOT>    supportedFirmwareTypes      (void) const final;
    FirmwarePlugin*         firmwarePluginForAutopilot  (MAV_AUTOPILOT autopilotType, MAV_TYPE vehicleType) final;
    QList<MAV_TYPE>         supportedVehicleTypes       (void) const final;
private:
    YuneecFirmwarePlugin*   _pluginInstance;
};
