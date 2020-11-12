/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "FirmwareUpgradeSettings.h"

#include <QQmlEngine>
#include <QtQml>

DECLARE_SETTINGGROUP(FirmwareUpgrade, "FirmwareUpgrade")
{
    qmlRegisterUncreatableType<FirmwareUpgradeSettings>("QGroundControl.SettingsManager", 1, 0, "FirmwareUpgradeSettings", "Reference only");
}

DECLARE_SETTINGSFACT(FirmwareUpgradeSettings, defaultFirmwareType)
DECLARE_SETTINGSFACT(FirmwareUpgradeSettings, apmChibiOS)
DECLARE_SETTINGSFACT(FirmwareUpgradeSettings, apmVehicleType)
