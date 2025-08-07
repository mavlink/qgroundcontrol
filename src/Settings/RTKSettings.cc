/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "RTKSettings.h"
// #include "BaseMode.h" // Removed, definition now in RTKSettings.h

DECLARE_SETTINGGROUP(RTK, "RTK")
{
    qmlRegisterUncreatableType<RTKSettings>("QGroundControl.SettingsManager", 1, 0, "RTKSettings", "Reference only"); \
    qRegisterMetaType<BaseModeDefinition::Mode>("BaseModeDefinition::Mode"); \
    qmlRegisterUncreatableType<BaseModeDefinition>("QGroundControl.SettingsManager", 1, 0, "BaseMode", "Reference to BaseModeDefinition enum holding class");
}

DECLARE_SETTINGSFACT(RTKSettings, baseReceiverManufacturers)
DECLARE_SETTINGSFACT(RTKSettings, surveyInAccuracyLimit)
DECLARE_SETTINGSFACT(RTKSettings, surveyInMinObservationDuration)
DECLARE_SETTINGSFACT(RTKSettings, useFixedBasePosition)
DECLARE_SETTINGSFACT(RTKSettings, fixedBasePositionLatitude)
DECLARE_SETTINGSFACT(RTKSettings, fixedBasePositionLongitude)
DECLARE_SETTINGSFACT(RTKSettings, fixedBasePositionAltitude)
DECLARE_SETTINGSFACT(RTKSettings, fixedBasePositionAccuracy)
