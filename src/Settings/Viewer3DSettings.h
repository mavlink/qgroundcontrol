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

///     @author Omid Esrafilian <esrafilian.omid@gmail.com>

class Viewer3DSettings : public SettingsGroup
{
    Q_OBJECT
public:
    Viewer3DSettings(QObject* parent = nullptr);

    DEFINE_SETTING_NAME_GROUP()

    DEFINE_SETTINGFACT(enabled)
    DEFINE_SETTINGFACT(osmFilePath)
    DEFINE_SETTINGFACT(buildingLevelHeight)
    DEFINE_SETTINGFACT(altitudeBias)
};
