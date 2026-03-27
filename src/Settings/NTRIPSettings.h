#pragma once

#include "SettingsGroup.h"

#include <QtQmlIntegration/QtQmlIntegration>

class NTRIPSettings : public SettingsGroup
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

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
    DEFINE_SETTINGFACT(ntripUseTls)
    DEFINE_SETTINGFACT(ntripAllowSelfSignedCerts)
    DEFINE_SETTINGFACT(ntripGgaPositionSource)
    DEFINE_SETTINGFACT(ntripUdpForwardEnabled)
    DEFINE_SETTINGFACT(ntripUdpTargetAddress)
    DEFINE_SETTINGFACT(ntripUdpTargetPort)
};
