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
    AuterionFirmwarePluginFactory();
    QList<MAV_AUTOPILOT>    supportedFirmwareTypes      () const override;
    FirmwarePlugin*         firmwarePluginForAutopilot  (MAV_AUTOPILOT autopilotType, MAV_TYPE vehicleType) override;
private:
    AuterionFirmwarePlugin*   _pluginInstance;
};
