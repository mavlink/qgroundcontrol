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

class FlightMapSettings : public SettingsGroup
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
public:
    FlightMapSettings(QObject* parent = nullptr);


    DEFINE_SETTING_NAME_GROUP()
    DEFINE_SETTINGFACT(mapProvider)
    DEFINE_SETTINGFACT(mapType)
    DEFINE_SETTINGFACT(elevationMapProvider)
};
