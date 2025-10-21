/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtQmlIntegration/QtQmlIntegration>

#include "SettingsGroup.h"

/// Settings for a single stored vehicle profile entry.
class VehicleProfileSettings : public SettingsGroup
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Created by VehicleProfile manager")

public:
    explicit VehicleProfileSettings(QObject* parent = nullptr);

    DEFINE_SETTING_NAME_GROUP()
    DEFINE_SETTINGFACT(vehicleName)
    DEFINE_SETTINGFACT(mavlinkId)
};
