/****************************************************************************
 *
 *   (c) 2009-2017 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

/// @file
/// @brief Branding settings

#pragma once

#include "SettingsGroup.h"

/// Simple branding. Allows to define icon to use on main toolbar.
class BrandImageSettings : public SettingsGroup
{
    Q_OBJECT
public:
    BrandImageSettings(QObject* parent = nullptr);
    DEFINE_SETTING_NAME_GROUP()
    DEFINE_SETTINGFACT(userBrandImageIndoor)
    DEFINE_SETTINGFACT(userBrandImageOutdoor)
};
