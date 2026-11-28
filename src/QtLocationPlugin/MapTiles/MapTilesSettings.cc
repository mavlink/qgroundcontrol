#include "MapTilesSettings.h"

#include "AppSettings.h"
#include "FlightMapSettings.h"
#include "FlyViewSettings.h"
#include "MapsSettings.h"
#include "PlanViewSettings.h"
#include "SettingsManager.h"

namespace MapTilesSettings {

Fact* mapProvider()
{
    FlightMapSettings* const settings = SettingsManager::instance()->flightMapSettings();
    return settings ? settings->mapProvider() : nullptr;
}

Fact* mapType()
{
    FlightMapSettings* const settings = SettingsManager::instance()->flightMapSettings();
    return settings ? settings->mapType() : nullptr;
}

QString elevationMapProvider()
{
    return SettingsManager::instance()->flightMapSettings()->elevationMapProvider()->rawValue().toString();
}

bool usesGeoMapEngine()
{
    const SettingsManager* const settings = SettingsManager::instance();
    return settings->flyViewSettings()->useGeoMapEngine()->rawValue().toBool() ||
           settings->planViewSettings()->useGeoMapEngine()->rawValue().toBool();
}

bool persistenceDisabled()
{
    return SettingsManager::instance()->appSettings()->disableAllPersistence()->rawValue().toBool();
}

quint32 maxCacheDiskSizeMB()
{
    return SettingsManager::instance()->mapsSettings()->maxCacheDiskSize()->rawValue().toUInt();
}

quint32 maxCacheMemorySizeMB()
{
    return SettingsManager::instance()->mapsSettings()->maxCacheMemorySize()->rawValue().toUInt();
}

QString esriToken()
{
    return SettingsManager::instance()->appSettings()->esriToken()->rawValue().toString();
}

QString mapboxToken()
{
    return SettingsManager::instance()->appSettings()->mapboxToken()->rawValue().toString();
}

QString tiandituToken()
{
    return SettingsManager::instance()->appSettings()->tiandituToken()->rawValue().toString();
}

QString vworldToken()
{
    return SettingsManager::instance()->appSettings()->vworldToken()->rawValue().toString();
}

QString openaipToken()
{
    return SettingsManager::instance()->appSettings()->openaipToken()->rawValue().toString();
}

const Fact* mapboxAccount()
{
    return SettingsManager::instance()->appSettings()->mapboxAccount();
}

const Fact* mapboxStyle()
{
    return SettingsManager::instance()->appSettings()->mapboxStyle();
}

const Fact* customUrl()
{
    return SettingsManager::instance()->appSettings()->customURL();
}

const Fact* language()
{
    return SettingsManager::instance()->appSettings()->qLocaleLanguage();
}

}  // namespace MapTilesSettings
