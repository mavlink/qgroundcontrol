#include "UrlFactory.h"

#include "ElevationMapProvider.h"
#include "MapProviderTable.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(UrlFactoryLog, "MapTiles.UrlFactory")

const QList<SharedMapProvider> UrlFactory::_providers = [] {
    QList<SharedMapProvider> providers = MapProviderTable::providers();
    providers.append(std::make_shared<CopernicusElevationProvider>());
    providers.append(std::make_shared<TerrariumElevationProvider>());
    return providers;
}();

QString UrlFactory::getProviderTypeFromMapId(int mapId)
{
    const SharedMapProvider provider = getMapProviderFromMapId(mapId);
    return provider ? provider->getMapName() : QString();
}

SharedMapProvider UrlFactory::getMapProviderFromMapId(int mapId)
{
    if (mapId == -1) {
        return nullptr;
    }

    for (const SharedMapProvider& provider : _providers) {
        if (provider->getMapId() == mapId) {
            return provider;
        }
    }

    qCWarning(UrlFactoryLog) << "map id not found:" << mapId;
    return nullptr;
}

SharedMapProvider UrlFactory::findProvider(QStringView type)
{
    for (const SharedMapProvider& provider : _providers) {
        if (provider->getMapName() == type) {
            return provider;
        }
    }

    return nullptr;
}

SharedMapProvider UrlFactory::getMapProviderFromProviderType(QStringView type)
{
    if (type.isEmpty()) {
        return nullptr;
    }

    SharedMapProvider provider = findProvider(type);
    if (!provider) {
        qCWarning(UrlFactoryLog) << "type not found:" << type;
    }

    return provider;
}

int UrlFactory::getMapIdFromProviderType(QStringView type)
{
    const SharedMapProvider provider = getMapProviderFromProviderType(type);
    return provider ? provider->getMapId() : -1;
}

QStringList UrlFactory::getElevationProviderTypes()
{
    QStringList types;
    for (const SharedMapProvider& provider : _providers) {
        if (provider->isElevationProvider()) {
            types.append(provider->getMapName());
        }
    }

    return types;
}

QStringList UrlFactory::getProviderTypes()
{
    QStringList types;
    for (const SharedMapProvider& provider : _providers) {
        types.append(provider->getMapName());
    }

    return types;
}

QMap<int, int> UrlFactory::legacyMapIds()
{
    // Version 1 numbered the providers of its list from 1. The list was the providers 1 to 41 in the order of their
    // ids, without the 5 Google providers on a build that lacks them. Stated here and not derived from _providers, so
    // that changing that list cannot change how an old cache is read.
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

QHash<int, QString> UrlFactory::cacheVariants()
{
    QHash<int, QString> variants;
    for (const SharedMapProvider& provider : _providers) {
        const QString variant = provider->cacheVariant();
        if (!variant.isEmpty()) {
            variants.insert(provider->getMapId(), variant);
        }
    }

    return variants;
}
