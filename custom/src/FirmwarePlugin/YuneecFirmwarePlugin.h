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
    QString             brandImage      (const Vehicle* vehicle) const;
};
