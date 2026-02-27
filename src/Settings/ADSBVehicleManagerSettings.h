#pragma once

#include <QtQmlIntegration/QtQmlIntegration>

#include "SettingsGroup.h"

class ADSBVehicleManagerSettings : public SettingsGroup
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
public:
    ADSBVehicleManagerSettings(QObject* parent = nullptr);
    DEFINE_SETTING_NAME_GROUP()

    DEFINE_SETTINGFACT(adsbServerConnectEnabled)
    DEFINE_SETTINGFACT(adsbServerHostAddress)
    DEFINE_SETTINGFACT(adsbServerPort)
};
