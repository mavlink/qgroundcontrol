#pragma once

#include <QtQmlIntegration/QtQmlIntegration>

#include "SettingsGroup.h"

class FirmwareUpgradeSettings : public SettingsGroup
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
public:
    FirmwareUpgradeSettings(QObject* parent = nullptr);

    DEFINE_SETTING_NAME_GROUP()

    DEFINE_SETTINGFACT(defaultFirmwareType)
    DEFINE_SETTINGFACT(apmChibiOS)
    DEFINE_SETTINGFACT(apmVehicleType)
};
