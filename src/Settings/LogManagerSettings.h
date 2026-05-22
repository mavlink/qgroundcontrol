#pragma once

#include <QtQmlIntegration/QtQmlIntegration>

#include "SettingsGroup.h"

class LogManagerSettings : public SettingsGroup
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
public:
    LogManagerSettings(QObject* parent = nullptr);

    DEFINE_SETTING_NAME_GROUP()

    DEFINE_SETTINGFACT(diskLoggingEnabled)
    DEFINE_SETTINGFACT(diskLoggingMaxFileSizeMB)
    DEFINE_SETTINGFACT(diskLoggingMaxBackupFiles)
    DEFINE_SETTINGFACT(saveFormat)
};
