/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "ADSBVehicleManagerSettings.h"

#include <QQmlEngine>
#include <QtQml>

DECLARE_SETTINGGROUP(ADSBVehicleManager, "ADSBVehicleManager")
{
    qmlRegisterUncreatableType<ADSBVehicleManagerSettings>("QGroundControl.SettingsManager", 1, 0, "ADSBVehicleManagerSettings", "Reference only");
}

DECLARE_SETTINGSFACT(ADSBVehicleManagerSettings, adsbServerConnectEnabled)
DECLARE_SETTINGSFACT(ADSBVehicleManagerSettings, adsbServerHostAddress)
DECLARE_SETTINGSFACT(ADSBVehicleManagerSettings, adsbServerPort)
