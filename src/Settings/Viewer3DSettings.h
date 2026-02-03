#pragma once

#include <QtQmlIntegration/QtQmlIntegration>

#include "SettingsGroup.h"

class Viewer3DSettings : public SettingsGroup
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
public:
    Viewer3DSettings(QObject* parent = nullptr);

    enum MapProvider {
        MapProviderOSM = 0,
    };
    Q_ENUM(MapProvider)

    DEFINE_SETTING_NAME_GROUP()

    DEFINE_SETTINGFACT(enabled)
    DEFINE_SETTINGFACT(mapProvider)
    DEFINE_SETTINGFACT(osmFilePath)
    DEFINE_SETTINGFACT(buildingLevelHeight)
    DEFINE_SETTINGFACT(altitudeBias)
};
