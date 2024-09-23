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

    DEFINE_SETTINGFACT(display)
    DEFINE_SETTINGFACT(threshold1)
    DEFINE_SETTINGFACT(threshold2)

    Q_PROPERTY(bool visible READ visible NOTIFY visibleChanged)
    Q_PROPERTY(bool thresholdEditable READ thresholdEditable NOTIFY thresholdEditableChanged)
    
    // Setters for thresholds
    // Expose setters to QML
    Q_INVOKABLE void setThreshold1(int value);
    Q_INVOKABLE void setThreshold2(int value);
    
    bool visible() const;  // Group visibility
    bool thresholdEditable() const;  // Single editable property for both thresholds

signals:
    void visibleChanged();
    void thresholdEditableChanged();
};