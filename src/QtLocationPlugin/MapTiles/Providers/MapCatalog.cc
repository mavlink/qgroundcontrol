#include "MapCatalog.h"

#include <QtCore/QApplicationStatic>
#include <QtCore/QRegularExpression>

#include "Fact.h"
#include "MapProvider.h"
#include "MapProviderRegistry.h"
#include "MapTilesSettings.h"

Q_APPLICATION_STATIC(MapCatalog, _mapCatalog);

MapCatalog* MapCatalog::instance()
{
    return _mapCatalog();
}

MapCatalog::MapCatalog(QObject* parent)
    : QObject(parent)
{
    // Saved settings that name no map are repaired: there is no map chosen before them to keep
    _repairMapOfSettings();
    _chosenMap = _mapOfSettings();
    for (const Fact* const setting : {MapTilesSettings::mapProvider(), MapTilesSettings::mapType()}) {
        if (setting) {
            (void) connect(setting, &Fact::rawValueChanged, this, &MapCatalog::_updateChosenMap);
        }
    }
}

QStringList MapCatalog::mapList()
{
    QStringList maps;
    for (const SharedMapProvider& provider : MapProviderRegistry::providers()) {
        // Every elevation provider, as before: elevationProviderList() has those that the user selects
        if (provider->isElevationProvider() || provider->isUserSelectable()) {
            maps.append(provider->mapName());
        }
    }
    return maps;
}

QStringList MapCatalog::downloadableMapList()
{
    QStringList maps = mapList();
    (void) maps.removeIf([](const QString& map) {
        const SharedMapProvider provider = MapProviderRegistry::findProvider(map);
        return provider && !provider->hasServer();
    });
    return maps;
}

QStringList MapCatalog::_imageryMapList()
{
    QStringList mapStringList = mapList();
    // Subtract ALL elevation providers (not just user-selectable ones) so
    // hidden elevation providers don't leak into the imagery maps
    const QStringList elevationStringList = MapProviderRegistry::elevationProviderTypes();
    for (const QString& elevationProviderName : elevationStringList) {
        (void) mapStringList.removeAll(elevationProviderName);
    }

    return mapStringList;
}

QStringList MapCatalog::mapProviderList()
{
    QStringList mapStringList = _imageryMapList();

    static const QRegularExpression providerType(QStringLiteral(R"(^([^\ ]*) (.*)$)"));
    (void) mapStringList.replaceInStrings(providerType, "\\1");
    (void) mapStringList.removeDuplicates();

    return mapStringList;
}

QStringList MapCatalog::mapTypeList(const QString& provider)
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

QString MapCatalog::mapName(const QString& provider, const QString& type)
{
    if (mapTypeList(provider).isEmpty()) {
        return provider;
    }

    return provider + QLatin1Char(' ') + type;
}

QStringList MapCatalog::elevationProviderList()
{
    // User-facing list only: hidden providers (e.g. Terrarium, GeoMap-only) are
    // still registered for cache/fetch but must not be selectable as the app's
    // elevation provider
    QStringList types;
    for (const SharedMapProvider& provider : MapProviderRegistry::providers()) {
        if (provider->isElevationProvider() && provider->isUserSelectable()) {
            types.append(provider->mapName());
        }
    }
    return types;
}

bool MapCatalog::isImageryMap(const QString& mapType)
{
    return _imageryMapList().contains(mapType);
}

bool MapCatalog::isSatelliteMap(const QString& mapType)
{
    const SharedMapProvider provider = MapProviderRegistry::findProvider(mapType);
    if (!provider) {
        return false;
    }

    switch (provider->mapStyle()) {
        case MapProvider::SatelliteMapDay:
        case MapProvider::SatelliteMapNight:
        case MapProvider::HybridMap:
            return true;
        case MapProvider::NoMap:
        case MapProvider::StreetMap:
        case MapProvider::TerrainMap:
        case MapProvider::TransitMap:
        case MapProvider::GrayStreetMap:
        case MapProvider::PedestrianMap:
        case MapProvider::CarNavigationMap:
        case MapProvider::CycleMap:
        case MapProvider::CustomMap:
            return false;
    }
    return false;
}

bool MapCatalog::isDarkMap(const QString& mapType)
{
    const SharedMapProvider provider = MapProviderRegistry::findProvider(mapType);
    return provider && (provider->isNight() || isSatelliteMap(mapType));
}

int MapCatalog::maxZoom(const QString& mapType)
{
    const SharedMapProvider provider = MapProviderRegistry::findProvider(mapType);
    return provider ? provider->maxZoom() : QGC_MAX_MAP_ZOOM;
}

QString MapCatalog::_mapOfSettings()
{
    const Fact* const providerFact = MapTilesSettings::mapProvider();
    const Fact* const typeFact = MapTilesSettings::mapType();
    if (!providerFact || !typeFact) {
        return QString();
    }

    return mapName(providerFact->rawValue().toString(), typeFact->rawValue().toString());
}

void MapCatalog::_repairMapOfSettings()
{
    Fact* const providerFact = MapTilesSettings::mapProvider();
    Fact* const typeFact = MapTilesSettings::mapType();
    if (!providerFact || !typeFact) {
        return;
    }

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

void MapCatalog::_updateChosenMap()
{
    const QString map = _mapOfSettings();
    if ((map != _chosenMap) && _imageryMapList().contains(map)) {
        _chosenMap = map;
        emit chosenMapChanged();
    }
}
