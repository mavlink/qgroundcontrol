/*!
 * @file
 *   @brief Yuneec Firmware Plugin (PX4)
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

#include "YuneecFirmwarePlugin.h"

YuneecFirmwarePlugin::YuneecFirmwarePlugin(void)
{

}

QString
YuneecFirmwarePlugin::brandImage(const Vehicle* vehicle) const
{
    Q_UNUSED(vehicle);
    return QStringLiteral("/typhoonh/YuneecBrandImage.png");
}
