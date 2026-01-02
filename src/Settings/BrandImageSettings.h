/// @file
/// @brief Branding settings

#pragma once

#include <QtQmlIntegration/QtQmlIntegration>

#include "SettingsGroup.h"

/// Simple branding. Allows to define icon to use on main toolbar.
class BrandImageSettings : public SettingsGroup
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
public:
    BrandImageSettings(QObject* parent = nullptr);
    DEFINE_SETTING_NAME_GROUP()
    DEFINE_SETTINGFACT(userBrandImageIndoor)
    DEFINE_SETTINGFACT(userBrandImageOutdoor)
};
