#pragma once

#include <QtQmlIntegration/QtQmlIntegration>

#include "SettingsGroup.h"

class GPSPositionSettings : public SettingsGroup
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

public:
    explicit GPSPositionSettings(QObject* parent = nullptr);
    ~GPSPositionSettings() override;

    DEFINE_SETTING_NAME_GROUP()
    DEFINE_SETTINGFACT(sourceMode)
};
