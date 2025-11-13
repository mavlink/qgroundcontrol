#include "MapsSettings.h"

#include <QtCore/QSettings>

DECLARE_SETTINGGROUP(Maps, "Maps")
{
    // Move deprecated keys to new location

    static constexpr const char* kMaxDiskCacheKey = "MaxDiskCache";
    static constexpr const char* kMaxMemCacheKey  = "MaxMemoryCache";

    QSettings deprecatedSettings;
    QSettings newSettings;

    newSettings.beginGroup(_settingsGroup);
    if (deprecatedSettings.contains(kMaxDiskCacheKey)) {
        uint32_t maxDiskCache = deprecatedSettings.value(kMaxDiskCacheKey, 1024).toUInt();
        deprecatedSettings.remove(kMaxDiskCacheKey);
        newSettings.setValue("maxCacheDiskSize", maxDiskCache);
   }
    if (deprecatedSettings.contains(kMaxMemCacheKey)) {
        uint32_t maxMemCache = deprecatedSettings.value(kMaxMemCacheKey, 1024).toUInt();
        deprecatedSettings.remove(kMaxMemCacheKey);
        newSettings.setValue("maxCacheMemorySize", maxMemCache);
    }

}

DECLARE_SETTINGSFACT(MapsSettings, maxCacheDiskSize)
DECLARE_SETTINGSFACT(MapsSettings, maxCacheMemorySize)
DECLARE_SETTINGSFACT(MapsSettings, disableDefaultCache)
