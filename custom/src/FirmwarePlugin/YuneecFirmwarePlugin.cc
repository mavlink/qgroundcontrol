/*!
 * @file
 *   @brief Yuneec Firmware Plugin (PX4)
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

#include "YuneecFirmwarePlugin.h"

const char* YuneecFirmwarePlugin::_simpleFlightMode =   "Smart";
const char* YuneecFirmwarePlugin::_posCtlFlightMode =   "Angle";

YuneecFirmwarePlugin::YuneecFirmwarePlugin(void)
{
    //  The following flight modes are renamed:
    //      Simple -> Smart
    //      POSCTL -> Angle

    for (int i=0; i<_flightModeInfoList.count(); i++) {
        FlightModeInfo_t& info = _flightModeInfoList[i];

        if (info.name == PX4FirmwarePlugin::_simpleFlightMode) {
            info.name = YuneecFirmwarePlugin::_simpleFlightMode;
        } else if (info.name == PX4FirmwarePlugin::_posCtlFlightMode) {
            info.name = YuneecFirmwarePlugin::_posCtlFlightMode;
        }
    }
}

QString
YuneecFirmwarePlugin::brandImage(const Vehicle* vehicle) const
{
    Q_UNUSED(vehicle);
    return QStringLiteral("/typhoonh/YuneecBrandImage.png");
}

QString YuneecFirmwarePlugin::vehicleImageOpaque(const Vehicle* vehicle) const
{
    Q_UNUSED(vehicle);
    return QStringLiteral("/typhoonh/VehicleIndicator.svg");
}

QString YuneecFirmwarePlugin::vehicleImageOutline(const Vehicle* vehicle) const
{
    Q_UNUSED(vehicle);
    return QStringLiteral("/typhoonh/VehicleIndicator.svg");
}

QString YuneecFirmwarePlugin::vehicleImageCompass(const Vehicle* vehicle) const
{
    Q_UNUSED(vehicle);
    return QStringLiteral("/typhoonh/CompassIndicator.svg");
}
