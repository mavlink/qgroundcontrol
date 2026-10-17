#pragma once

#include <QtQmlIntegration/qqmlintegration.h>

#include "SettingsGroup.h"

class LogViewerSettings : public SettingsGroup
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
public:
    LogViewerSettings(QObject* parent = nullptr);
    DEFINE_SETTING_NAME_GROUP()

    DEFINE_SETTINGFACT(xAxisShowLocalTime)
};
