/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "FirmwareUpgradeSettings.h"

DECLARE_SETTINGGROUP(FirmwareUpgrade, "FirmwareUpgrade")
{
}

DECLARE_SETTINGSFACT(FirmwareUpgradeSettings, defaultFirmwareType)
DECLARE_SETTINGSFACT(FirmwareUpgradeSettings, apmChibiOS)
DECLARE_SETTINGSFACT(FirmwareUpgradeSettings, apmVehicleType)
