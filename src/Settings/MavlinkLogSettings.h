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

class MavlinkLogSettings : public SettingsGroup
{
    Q_OBJECT
public:
    MavlinkLogSettings(QObject* parent = nullptr);

    DEFINE_SETTING_NAME_GROUP()

    DEFINE_SETTINGFACT(emailAddress)
    DEFINE_SETTINGFACT(description)
    DEFINE_SETTINGFACT(uploadURL)
    DEFINE_SETTINGFACT(videoURL)
    DEFINE_SETTINGFACT(autoUploadEnabled)
    DEFINE_SETTINGFACT(autoStartEnabled)
    DEFINE_SETTINGFACT(deleteAferUpload)
    DEFINE_SETTINGFACT(windSpeed)
    DEFINE_SETTINGFACT(rating)
    DEFINE_SETTINGFACT(isPulicLog)
};
