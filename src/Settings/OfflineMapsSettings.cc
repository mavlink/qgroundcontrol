/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "OfflineMapsSettings.h"

#include <QQmlEngine>
#include <QtQml>

DECLARE_SETTINGGROUP(OfflineMaps, "OfflineMaps")
{
    qmlRegisterUncreatableType<OfflineMapsSettings>("QGroundControl.SettingsManager", 1, 0, "OfflineMapsSettings", "Reference only");
}

DECLARE_SETTINGSFACT(OfflineMapsSettings, minZoomLevelDownload)
DECLARE_SETTINGSFACT(OfflineMapsSettings, maxZoomLevelDownload)
DECLARE_SETTINGSFACT(OfflineMapsSettings, maxTilesForDownload)
