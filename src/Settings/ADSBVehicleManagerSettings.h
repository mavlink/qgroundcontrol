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

class ADSBVehicleManagerSettings : public SettingsGroup
{
    Q_OBJECT
public:
    ADSBVehicleManagerSettings(QObject* parent = nullptr);
    DEFINE_SETTING_NAME_GROUP()

    DEFINE_SETTINGFACT(adsbServerConnectEnabled)
    DEFINE_SETTINGFACT(adsbServerHostAddress)
    DEFINE_SETTINGFACT(adsbServerPort)
};
