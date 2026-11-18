#include "QGCMapCatalog.h"

#include <QtCore/QApplicationStatic>
#include <QtCore/QRegularExpression>

#include "FlightMapSettings.h"
#include "MapProvider.h"
#include "SettingsManager.h"
#include "UrlFactory.h"

Q_APPLICATION_STATIC(QGCMapCatalog, _mapCatalog);

QGCMapCatalog* QGCMapCatalog::instance()
{
    return _mapCatalog();
}

QGCMapCatalog::QGCMapCatalog(QObject* parent)
    : QObject(parent)
{
    // Saved settings that name no map are repaired: there is no map chosen before them to keep
    _repairMapOfSettings();
    _chosenMap = _mapOfSettings();
    if (FlightMapSettings* const settings = SettingsManager::instance()->flightMapSettings()) {
        (void) connect(settings->mapProvider(), &Fact::rawValueChanged, this, &QGCMapCatalog::_updateChosenMap);
        (void) connect(settings->mapType(), &Fact::rawValueChanged, this, &QGCMapCatalog::_updateChosenMap);
    }
}

QStringList QGCMapCatalog::mapList()
{
    return UrlFactory::getProviderTypes();
}

QStringList QGCMapCatalog::_imageryMapList()
{
    QStringList mapStringList = mapList();
    // Subtract ALL elevation providers (not just user-selectable ones) so
    // hidden elevation providers don't leak into the imagery maps
    const QStringList elevationStringList = UrlFactory::getElevationProviderTypes();
    for (const QString& elevationProviderName : elevationStringList) {
        (void) mapStringList.removeAll(elevationProviderName);
    }

    return mapStringList;
}

QStringList QGCMapCatalog::mapProviderList()
{
    QStringList mapStringList = _imageryMapList();

    static const QRegularExpression providerType(QStringLiteral(R"(^([^\ ]*) (.*)$)"));
    (void) mapStringList.replaceInStrings(providerType, "\\1");
    (void) mapStringList.removeDuplicates();

    return mapStringList;
}

QStringList QGCMapCatalog::mapTypeList(const QString& provider)
{
    // A map is named "<provider> <type>"
    const QString providerPrefix = provider + QLatin1Char(' ');
    const QStringList maps = mapList();
    QStringList types;
    for (const QString& map : maps) {
        if (map.startsWith(providerPrefix)) {
            types.append(map.sliced(providerPrefix.size()));
        }
    }

    return types;
}

QString QGCMapCatalog::mapName(const QString& provider, const QString& type)
{
    if (mapTypeList(provider).isEmpty()) {
        return provider;
    }

    return provider + QLatin1Char(' ') + type;
}

QStringList QGCMapCatalog::elevationProviderList()
{
    // User-facing list only: hidden providers (e.g. Terrarium, GeoMap-only) are
    // still registered for cache/fetch but must not be selectable as the app's
    // elevation provider
    QStringList types;
    for (const SharedMapProvider& provider : UrlFactory::getProviders()) {
        if (provider->isElevationProvider() && provider->isUserSelectable()) {
            types.append(provider->getMapName());
        }
    }
    return types;
}

bool QGCMapCatalog::isImageryMap(const QString& mapType)
{
    return _imageryMapList().contains(mapType);
}

bool QGCMapCatalog::isSatelliteMap(const QString& mapType)
{
    const SharedMapProvider provider = UrlFactory::findProvider(mapType);
    if (!provider) {
        return false;
    }

    switch (provider->getMapStyle()) {
        case MapProvider::SatelliteMapDay:
        case MapProvider::SatelliteMapNight:
        case MapProvider::HybridMap:
            return true;
        default:
            return false;
    }
}

QString QGCMapCatalog::_mapOfSettings()
{
    FlightMapSettings* const settings = SettingsManager::instance()->flightMapSettings();
    if (!settings) {
        return QString();
    }

    return mapName(settings->mapProvider()->rawValue().toString(), settings->mapType()->rawValue().toString());
}

void QGCMapCatalog::_repairMapOfSettings()
{
    FlightMapSettings* const settings = SettingsManager::instance()->flightMapSettings();
    if (!settings) {
        return;
    }

    Fact* const providerFact = settings->mapProvider();
    Fact* const typeFact = settings->mapType();
    const QString provider = providerFact->rawValue().toString();
    const QStringList maps = _imageryMapList();
    if (maps.contains(mapName(provider, typeFact->rawValue().toString()))) {
        return;
    }

    const QStringList types = mapTypeList(provider);
    if (!types.isEmpty()) {
        typeFact->setRawValue(types.constFirst());
        return;
    }

    // No map has the provider
    const QVariant defaultProvider = providerFact->rawDefaultValue();
    const QVariant defaultType = typeFact->rawDefaultValue();
    if (maps.contains(mapName(defaultProvider.toString(), defaultType.toString()))) {
        providerFact->setRawValue(defaultProvider);
        typeFact->setRawValue(defaultType);
    }
}

void QGCMapCatalog::_updateChosenMap()
{
    const QString map = _mapOfSettings();
    if ((map != _chosenMap) && _imageryMapList().contains(map)) {
        _chosenMap = map;
        emit chosenMapChanged();
    }
}
