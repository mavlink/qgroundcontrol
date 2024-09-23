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

DECLARE_SETTINGSFACT(BatteryIndicatorSettings, display)
DECLARE_SETTINGSFACT(BatteryIndicatorSettings, threshold1)
DECLARE_SETTINGSFACT(BatteryIndicatorSettings, threshold2)

// Set threshold1 with validation
void BatteryIndicatorSettings::setThreshold1(int value) {
    // Ensure value is at least 16 and less than 100
    if (value < 16) {
        threshold1()->setRawValue(17);
    } else if (value > 99) {
        threshold1()->setRawValue(99);
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

bool BatteryIndicatorSettings::visible() const {
    QSettings settings;
    return settings.value("ShowBatterySettings", true).toBool();
}

bool BatteryIndicatorSettings::thresholdEditable() const {
    QSettings settings;
    return settings.value("ThresholdEditable", true).toBool();
}
