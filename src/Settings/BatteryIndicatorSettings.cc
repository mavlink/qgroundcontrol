/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "BatteryIndicatorSettings.h"

#include <QSettings>
#include <QtQml/QQmlEngine>

DECLARE_SETTINGGROUP(BatteryIndicator, "BatteryIndicator")
{
    qmlRegisterUncreatableType<BatteryIndicatorSettings>("QGroundControl.SettingsManager", 1, 0, "BatteryIndicatorSettings", "Reference only");
}

DECLARE_SETTINGSFACT(BatteryIndicatorSettings, valueDisplay)

DECLARE_SETTINGSFACT_NO_FUNC(BatteryIndicatorSettings, threshold1)
{
    if (!_threshold1Fact) {
        _threshold1Fact = _createSettingsFact(threshold1Name);
        connect(_threshold1Fact, &SettingsFact::rawValueChanged, this, &BatteryIndicatorSettings::_threshold1Changed);
    }
    return _threshold1Fact;
}

DECLARE_SETTINGSFACT_NO_FUNC(BatteryIndicatorSettings, threshold2)
{
    if (!_threshold2Fact) {
        _threshold2Fact = _createSettingsFact(threshold2Name);
        connect(_threshold2Fact, &SettingsFact::rawValueChanged, this, &BatteryIndicatorSettings::_threshold2Changed);
    }
    return _threshold2Fact;
}

// Change handlers for thresholds
void BatteryIndicatorSettings::_threshold1Changed() {
    validateThreshold1(); // Call validation when threshold1 value changes
}

void BatteryIndicatorSettings::_threshold2Changed() {
    validateThreshold2(); // Call validation when threshold2 value changes
}

// Validate threshold1 value
void BatteryIndicatorSettings::validateThreshold1() {
    int value = threshold1()->rawValue().toInt();
    setThreshold1(value); // Call the setter with the current value
}

// Validate threshold2 value
void BatteryIndicatorSettings::validateThreshold2() {
    int value = threshold2()->rawValue().toInt();
    setThreshold2(value); // Call the setter with the current value
}

// Set threshold1 with validation
void BatteryIndicatorSettings::setThreshold1(int value) {
    // Ensure value is at least 16 and less than 100
    if (value < 16) {
        threshold1()->setRawValue(17); // Adjust to minimum valid value
    } else if (value > 99) {
        threshold1()->setRawValue(99); // Cap at maximum valid value
    } else {
        // Check if value is greater than threshold2
        if (value > threshold2()->rawValue().toInt()) {
            threshold1()->setRawValue(value);
        } else {
            // Ensure threshold1 is greater than threshold2
            threshold1()->setRawValue(threshold2()->rawValue().toInt() + 1);
        }
    }
}

// Set threshold2 with validation
void BatteryIndicatorSettings::setThreshold2(int value) {
    // Ensure value is greater than 15
    if (value <= 15) {
        threshold2()->setRawValue(16); // Adjust to the minimum valid value
        return;
    }

    // Check if value is less than threshold1
    if (value < threshold1()->rawValue().toInt()) {
        threshold2()->setRawValue(value);
    } else {
        // Ensure threshold2 is less than threshold1
        threshold2()->setRawValue(threshold1()->rawValue().toInt() - 1);
    }
}
