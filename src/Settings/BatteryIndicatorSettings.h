/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "SettingsGroup.h"

class BatteryIndicatorSettings : public SettingsGroup
{
    Q_OBJECT

public:
    BatteryIndicatorSettings(QObject* parent = nullptr);

    DEFINE_SETTING_NAME_GROUP()

    DEFINE_SETTINGFACT(valueDisplay)            // Battery value display mode
    DEFINE_SETTINGFACT(threshold1)              // First threshold for battery level
    DEFINE_SETTINGFACT(threshold2)              // Second threshold for battery level

    Q_INVOKABLE void setThreshold1(int value);  // Set threshold1 with validation
    Q_INVOKABLE void setThreshold2(int value);  // Set threshold2 with validation

private:
    void validateThreshold1();
    void validateThreshold2(); 
    void _threshold1Changed();
    void _threshold2Changed();
};
