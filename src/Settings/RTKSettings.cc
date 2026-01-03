#include "RTKSettings.h"

DECLARE_SETTINGGROUP(RTK, "RTK") {}

DECLARE_SETTINGSFACT(RTKSettings, baseReceiverManufacturers)
DECLARE_SETTINGSFACT(RTKSettings, surveyInAccuracyLimit)
DECLARE_SETTINGSFACT(RTKSettings, surveyInMinObservationDuration)
DECLARE_SETTINGSFACT(RTKSettings, useFixedBasePosition)
DECLARE_SETTINGSFACT(RTKSettings, fixedBasePositionLatitude)
DECLARE_SETTINGSFACT(RTKSettings, fixedBasePositionLongitude)
DECLARE_SETTINGSFACT(RTKSettings, fixedBasePositionAltitude)
DECLARE_SETTINGSFACT(RTKSettings, fixedBasePositionAccuracy)
