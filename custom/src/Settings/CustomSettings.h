#pragma once

#include <QtQmlIntegration/QtQmlIntegration>

#include "SettingsGroup.h"

/// Example custom-build settings group. Registered at runtime via
/// CustomPlugin::registerCustomSettings so generated settings pages can reference
/// facts as QGroundControl.settingsManager.customSettings.<factName>.
class CustomSettings : public SettingsGroup
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
public:
    CustomSettings(QObject* parent = nullptr);

    DEFINE_SETTING_NAME_GROUP()

    DEFINE_SETTINGFACT(showAttitudeWidget)
    DEFINE_SETTINGFACT(updateInterval)
    DEFINE_SETTINGFACT(operatorName)
};
