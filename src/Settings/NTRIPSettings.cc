#include "NTRIPSettings.h"

DECLARE_SETTINGGROUP(NTRIP, "NTRIP") {}

DECLARE_SETTINGSFACT(NTRIPSettings, ntripServerConnectEnabled)
DECLARE_SETTINGSFACT(NTRIPSettings, ntripServerHostAddress)
DECLARE_SETTINGSFACT(NTRIPSettings, ntripServerPort)
DECLARE_SETTINGSFACT(NTRIPSettings, ntripUsername)
DECLARE_SETTINGSFACT(NTRIPSettings, ntripPassword)
DECLARE_SETTINGSFACT(NTRIPSettings, ntripMountpoint)
DECLARE_SETTINGSFACT(NTRIPSettings, ntripWhitelist)
DECLARE_SETTINGSFACT(NTRIPSettings, ntripUseTls)
DECLARE_SETTINGSFACT(NTRIPSettings, ntripUdpForwardEnabled)
DECLARE_SETTINGSFACT(NTRIPSettings, ntripUdpTargetAddress)
DECLARE_SETTINGSFACT(NTRIPSettings, ntripUdpTargetPort)
