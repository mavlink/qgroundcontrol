#pragma once

#include <QtQmlIntegration/qqmlintegration.h>

#include "SettingsGroup.h"

class MapsSettings : public SettingsGroup
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
public:
    MapsSettings(QObject* parent = nullptr);

    DEFINE_SETTING_NAME_GROUP()

    DEFINE_SETTINGFACT(maxCacheDiskSize)
    DEFINE_SETTINGFACT(maxCacheMemorySize)
};
