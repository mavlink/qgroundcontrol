/*!
 * @file
 *   @brief Yuneec Firmware Plugin (PX4)
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

#pragma once

#include "FirmwarePlugin.h"
#include "PX4FirmwarePlugin.h"

class YuneecFirmwarePlugin : public PX4FirmwarePlugin
{
    Q_OBJECT
public:
    YuneecFirmwarePlugin(void);
    QString     brandImage          (const Vehicle* vehicle) const;
    QString     vehicleImageOpaque  (const Vehicle* vehicle) const;
    QString     vehicleImageOutline (const Vehicle* vehicle) const;
    QString     vehicleImageCompass (const Vehicle* vehicle) const;
};
