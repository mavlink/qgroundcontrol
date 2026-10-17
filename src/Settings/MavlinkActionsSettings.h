#pragma once

#include <QtQmlIntegration/qqmlintegration.h>

#include "SettingsGroup.h"

class MavlinkActionsSettings : public SettingsGroup
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
public:
    MavlinkActionsSettings(QObject* parent = nullptr);

    DEFINE_SETTING_NAME_GROUP()

    DEFINE_SETTINGFACT(flyViewActionsFile)
    DEFINE_SETTINGFACT(joystickActionsFile)
};
