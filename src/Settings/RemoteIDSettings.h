/****************************************************************************
 *
 * (c) 2009-2022 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "SettingsGroup.h"

class RemoteIDSettings : public SettingsGroup
{
    Q_OBJECT
public:
    RemoteIDSettings(QObject* parent = nullptr);
    DEFINE_SETTING_NAME_GROUP()

    DEFINE_SETTINGFACT(enable)
    DEFINE_SETTINGFACT(operatorID)
    DEFINE_SETTINGFACT(operatorIDType)
    DEFINE_SETTINGFACT(sendOperatorID)
    DEFINE_SETTINGFACT(selfIDFree)
    DEFINE_SETTINGFACT(selfIDEmergency)
    DEFINE_SETTINGFACT(selfIDExtended)
    DEFINE_SETTINGFACT(selfIDType)
    DEFINE_SETTINGFACT(sendSelfID)
    DEFINE_SETTINGFACT(basicID)
    DEFINE_SETTINGFACT(basicIDType)
    DEFINE_SETTINGFACT(basicIDUaType)
    DEFINE_SETTINGFACT(sendBasicID)
    DEFINE_SETTINGFACT(region)
    DEFINE_SETTINGFACT(locationType)
    DEFINE_SETTINGFACT(latitudeFixed)
    DEFINE_SETTINGFACT(longitudeFixed)
    DEFINE_SETTINGFACT(altitudeFixed)
    DEFINE_SETTINGFACT(classificationType)
    DEFINE_SETTINGFACT(categoryEU)
    DEFINE_SETTINGFACT(classEU)
};