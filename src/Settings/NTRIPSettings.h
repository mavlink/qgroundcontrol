#pragma once

#include "SettingsGroup.h"

class NTRIPSettings : public SettingsGroup
{
    Q_OBJECT

public:
    NTRIPSettings(QObject* parent = nullptr);

    DEFINE_SETTING_NAME_GROUP()

    DEFINE_SETTINGFACT(ntripServerConnectEnabled)
    DEFINE_SETTINGFACT(ntripServerHostAddress)
    DEFINE_SETTINGFACT(ntripServerPort)
    DEFINE_SETTINGFACT(ntripUsername)
    DEFINE_SETTINGFACT(ntripPassword)
    DEFINE_SETTINGFACT(ntripMountpoint)
    DEFINE_SETTINGFACT(ntripWhitelist)
    DEFINE_SETTINGFACT(ntripUseSpartn)
};
