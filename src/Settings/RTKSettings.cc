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
DECLARE_SETTINGSFACT(RTKSettings, gpsOutputMode)
DECLARE_SETTINGSFACT(RTKSettings, ubxMode)
DECLARE_SETTINGSFACT(RTKSettings, ubxDynamicModel)
DECLARE_SETTINGSFACT(RTKSettings, ubxUart2Baudrate)
DECLARE_SETTINGSFACT(RTKSettings, ubxPpkOutput)
DECLARE_SETTINGSFACT(RTKSettings, ubxDgnssTimeout)
DECLARE_SETTINGSFACT(RTKSettings, ubxMinCno)
DECLARE_SETTINGSFACT(RTKSettings, ubxMinElevation)
DECLARE_SETTINGSFACT(RTKSettings, ubxOutputRate)
DECLARE_SETTINGSFACT(RTKSettings, ubxJamDetSensitivityHi)
