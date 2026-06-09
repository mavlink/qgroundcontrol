#include "LogManagerSettings.h"

DECLARE_SETTINGGROUP(LogManager, "LogManager")
{
}

DECLARE_SETTINGSFACT(LogManagerSettings, diskLoggingEnabled)
DECLARE_SETTINGSFACT(LogManagerSettings, diskLoggingMaxFileSizeMB)
DECLARE_SETTINGSFACT(LogManagerSettings, diskLoggingMaxBackupFiles)
DECLARE_SETTINGSFACT(LogManagerSettings, saveFormat)
