/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "SettingsGroup.h"

class OfflineMapsSettings : public SettingsGroup
{
    Q_OBJECT

public:
    OfflineMapsSettings(QObject* parent = nullptr);

    DEFINE_SETTING_NAME_GROUP()
    DEFINE_SETTINGFACT(minZoomLevelDownload)
    DEFINE_SETTINGFACT(maxZoomLevelDownload)
    DEFINE_SETTINGFACT(maxTilesForDownload)

private:
};
