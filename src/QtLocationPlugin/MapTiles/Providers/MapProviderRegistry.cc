#include "MapProviderRegistry.h"

#include "ElevationMapProvider.h"
#include "MapProviderTable.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(MapProviderRegistryLog, "MapTiles.MapProviderRegistry")

const QList<SharedMapProvider>& MapProviderRegistry::providers()
{
    // Made at the first call: before main(), what the providers are made of need not exist yet
    static const QList<SharedMapProvider> providers = [] {
        QList<SharedMapProvider> all = MapProviderTable::providers();
        all.append(std::make_shared<CopernicusElevationProvider>());
        all.append(std::make_shared<TerrariumElevationProvider>());
        return all;
    }();
    return providers;
}

QString MapProviderRegistry::getProviderTypeFromMapId(int mapId)
{
    const SharedMapProvider provider = getMapProviderFromMapId(mapId);
    return provider ? provider->mapName() : QString();
}

SharedMapProvider MapProviderRegistry::getMapProviderFromMapId(int mapId)
{
    if (mapId == -1) {
        return nullptr;
    }

    for (const SharedMapProvider& provider : providers()) {
        if (provider->mapId() == mapId) {
            return provider;
        }
    }

    qCWarning(MapProviderRegistryLog) << "map id not found:" << mapId;
    return nullptr;
}

SharedMapProvider MapProviderRegistry::findProvider(QStringView type)
{
    for (const SharedMapProvider& provider : providers()) {
        if (provider->mapName() == type) {
            return provider;
        }
    }

    return nullptr;
}

SharedMapProvider MapProviderRegistry::getMapProviderFromProviderType(QStringView type)
{
    if (type.isEmpty()) {
        return nullptr;
    }

    SharedMapProvider provider = findProvider(type);
    if (!provider) {
        qCWarning(MapProviderRegistryLog) << "type not found:" << type;
    }

    return provider;
}

int MapProviderRegistry::getMapIdFromProviderType(QStringView type)
{
    const SharedMapProvider provider = getMapProviderFromProviderType(type);
    return provider ? provider->mapId() : -1;
}

QStringList MapProviderRegistry::elevationProviderTypes()
{
    QStringList types;
    for (const SharedMapProvider& provider : providers()) {
        if (provider->isElevationProvider()) {
            types.append(provider->mapName());
        }
    }

    return types;
}

QStringList MapProviderRegistry::providerTypes()
{
    QStringList types;
    for (const SharedMapProvider& provider : providers()) {
        types.append(provider->mapName());
    }

    return types;
}

QMap<int, int> MapProviderRegistry::legacyMapIds()
{
    // Version 1 numbered the providers of its list from 1. The list was the providers 1 to 41 in the order of their
    // ids, without the 5 Google providers on a build that lacks them. Stated here and not derived from the providers,
    // so that changing that list cannot change how an old cache is read.
#ifdef QGC_NO_GOOGLE_MAPS
    constexpr int firstMapId = 6;
#else
    constexpr int firstMapId = 1;
#endif
    constexpr int lastMapId = 41;

    QMap<int, int> ids;
    for (int mapId = firstMapId; mapId <= lastMapId; mapId++) {
        ids.insert(mapId - firstMapId + 1, mapId);
    }
    return ids;
}

QHash<int, QString> MapProviderRegistry::cacheVariants()
{
    QHash<int, QString> variants;
    for (const SharedMapProvider& provider : providers()) {
        const QString variant = provider->cacheVariant();
        if (!variant.isEmpty()) {
            variants.insert(provider->mapId(), variant);
        }
    }

    return variants;
}
