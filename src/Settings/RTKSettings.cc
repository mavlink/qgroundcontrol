#include "RTKSettings.h"

DECLARE_SETTINGGROUP(RTK, "RTK")
{
    QSettings settings;
    if (!settings.contains(QStringLiteral("RTK/connectionType")) &&
        settings.value(QStringLiteral("AutoConnect/autoConnectNetworkRTKGPS"), false).toBool()) {
        settings.setValue(QStringLiteral("RTK/connectionType"), static_cast<int>(Tcp));
    }
}

DECLARE_SETTINGSFACT(RTKSettings, baseReceiverManufacturers)
DECLARE_SETTINGSFACT(RTKSettings, surveyInAccuracyLimit)
DECLARE_SETTINGSFACT(RTKSettings, surveyInMinObservationDuration)
DECLARE_SETTINGSFACT(RTKSettings, useFixedBasePosition)
DECLARE_SETTINGSFACT(RTKSettings, fixedBasePositionLatitude)
DECLARE_SETTINGSFACT(RTKSettings, fixedBasePositionLongitude)
DECLARE_SETTINGSFACT(RTKSettings, fixedBasePositionAltitude)
DECLARE_SETTINGSFACT(RTKSettings, fixedBasePositionAccuracy)
DECLARE_SETTINGSFACT(RTKSettings, networkBaseHost)
DECLARE_SETTINGSFACT(RTKSettings, networkBasePort)
DECLARE_SETTINGSFACT(RTKSettings, udpLocalPort)
DECLARE_SETTINGSFACT(RTKSettings, networkReceiverType)

DECLARE_SETTINGSFACT(RTKSettings, useReceiverPosition)
DECLARE_SETTINGSFACT(RTKSettings, connectionType)
DECLARE_SETTINGSFACT(RTKSettings, serialDevice)

DECLARE_SETTINGSFACT(RTKSettings, receiverRole)
