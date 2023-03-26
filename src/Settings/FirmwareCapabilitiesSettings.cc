/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "FirmwareCapabilitiesSettings.h"

#include <QQmlEngine>
#include <QtQml>

DECLARE_SETTINGGROUP(FirmwareCapabilities, "FirmwareCapabilities")
{
    qmlRegisterUncreatableType<FirmwareCapabilitiesSettings>("QGroundControl.SettingsManager", 1, 0, "FirmwareCapabilitiesSettings", "Reference only"); \
}

DECLARE_SETTINGSFACT(FirmwareCapabilitiesSettings, enableROI)

