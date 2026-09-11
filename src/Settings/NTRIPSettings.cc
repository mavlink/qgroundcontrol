#include "NTRIPSettings.h"

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(NTRIPSettingsLog, "GPS.Settings.NTRIPSettings")

const char* NTRIPSettings::name = "NTRIP";
const char* NTRIPSettings::settingsGroup = "NTRIP";

NTRIPSettings::NTRIPSettings(GPSCorrectionSettings& correctionSettings, QObject* parent)
    : SettingsGroup(name, settingsGroup, parent)
    , _correctionSettings(correctionSettings)
{
    qCDebug(NTRIPSettingsLog) << this;
}

NTRIPSettings::~NTRIPSettings()
{
    qCDebug(NTRIPSettingsLog) << this;
}

DECLARE_SETTINGSFACT(NTRIPSettings, ntripServerConnectEnabled)
DECLARE_SETTINGSFACT(NTRIPSettings, ntripServerHostAddress)
DECLARE_SETTINGSFACT(NTRIPSettings, ntripServerPort)
DECLARE_SETTINGSFACT(NTRIPSettings, ntripUsername)
DECLARE_SETTINGSFACT(NTRIPSettings, ntripPassword)
DECLARE_SETTINGSFACT(NTRIPSettings, ntripMountpoint)
DECLARE_SETTINGSFACT(NTRIPSettings, ntripWhitelist)
DECLARE_SETTINGSFACT(NTRIPSettings, ntripUseTls)
DECLARE_SETTINGSFACT(NTRIPSettings, ntripAllowSelfSignedCerts)
DECLARE_SETTINGSFACT(NTRIPSettings, ntripGgaPositionSource)
DECLARE_SETTINGSFACT(NTRIPSettings, ntripGgaIntervalSec)
DECLARE_SETTINGSFACT(NTRIPSettings, ntripUdpForwardEnabled)
DECLARE_SETTINGSFACT(NTRIPSettings, ntripUdpTargetAddress)
DECLARE_SETTINGSFACT(NTRIPSettings, ntripUdpTargetPort)
const char* NTRIPSettings::rtcmUdpInputEnabledName = "rtcmUdpInputEnabled";
const char* NTRIPSettings::rtcmUdpInputPortName = "rtcmUdpInputPort";
const char* NTRIPSettings::rtcmUdpValidateName = "rtcmUdpValidate";
const char* NTRIPSettings::correctionSourceName = "correctionSource";
const char* NTRIPSettings::correctionSourceInstanceName = "correctionSourceInstance";
const char* NTRIPSettings::injectLocalReceiverName = "injectLocalReceiver";
