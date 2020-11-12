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

class AirMapSettings : public SettingsGroup
{
    Q_OBJECT
public:
    AirMapSettings(QObject* parent = nullptr);

    DEFINE_SETTING_NAME_GROUP()

    DEFINE_SETTINGFACT(usePersonalApiKey)
    DEFINE_SETTINGFACT(apiKey)
    DEFINE_SETTINGFACT(clientID)
    DEFINE_SETTINGFACT(userName)
    DEFINE_SETTINGFACT(password)
    DEFINE_SETTINGFACT(enableAirMap)
    DEFINE_SETTINGFACT(enableAirspace)
    DEFINE_SETTINGFACT(enableTelemetry)

};
