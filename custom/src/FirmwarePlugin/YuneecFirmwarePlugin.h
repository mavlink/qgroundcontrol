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

    // FirmwarePlugin overrides

    QString         brandImage          (const Vehicle* vehicle) const final;
    QString         vehicleImageOpaque  (const Vehicle* vehicle) const final;
    QString         vehicleImageOutline (const Vehicle* vehicle) const final;
    QString         vehicleImageCompass (const Vehicle* vehicle) const final;
    QVariantList&   toolBarIndicators   (const Vehicle* vehicle) final;

private:
    static const char* _simpleFlightMode;
    static const char* _posCtlFlightMode;

};
