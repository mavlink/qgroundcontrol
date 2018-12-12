/*!
 * @file
 *   @brief Auterion Firmware Plugin Factory (PX4)
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

#pragma once

#include "FirmwarePlugin.h"

class AuterionFirmwarePlugin;

class AuterionFirmwarePluginFactory : public FirmwarePluginFactory
{
    Q_OBJECT
public:
    AuterionFirmwarePluginFactory(void);
    QList<MAV_AUTOPILOT>    supportedFirmwareTypes      (void) const final;
    FirmwarePlugin*         firmwarePluginForAutopilot  (MAV_AUTOPILOT autopilotType, MAV_TYPE vehicleType) final;
    QList<MAV_TYPE>         supportedVehicleTypes       (void) const final;
private:
    AuterionFirmwarePlugin*   _pluginInstance;
};
