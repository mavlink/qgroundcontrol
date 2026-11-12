#include "UrlFactory.h"

#include <QtCore/QCryptographicHash>
#include <QtCore/QUrl>
#include <QtCore/QtMinMax>

#include "BingMapProvider.h"
#include "ElevationMapProvider.h"
#include "EsriMapProvider.h"
#include "GenericMapProvider.h"
#include "GoogleMapProvider.h"
#include "MapboxMapProvider.h"
#include "QGCLoggingCategory.h"
#include "QGCTileSet.h"
#include "TianDiTuProvider.h"

QGC_LOGGING_CATEGORY(UrlFactoryLog, "MapTiles.UrlFactory")

template <typename Provider>
SharedMapProvider UrlFactory::_provider(int mapId)
{
    std::shared_ptr<Provider> provider = std::make_shared<Provider>();
    provider->_mapId = mapId;
    return provider;
}

// The number is the permanent id of the provider (MapProvider::getMapId()), a key of the tile cache:
// - never change or reuse one
// - a new provider takes the next unused number
// - the number of a removed provider stays retired
const QList<SharedMapProvider> UrlFactory::_providers = {
#ifndef QGC_NO_GOOGLE_MAPS
    _provider<GoogleStreetMapProvider>(1),
    _provider<GoogleSatelliteMapProvider>(2),
    _provider<GoogleTerrainMapProvider>(3),
    _provider<GoogleHybridMapProvider>(4),
    _provider<GoogleLabelsMapProvider>(5),
#endif
    _provider<BingRoadMapProvider>(6),
    _provider<BingSatelliteMapProvider>(7),
    _provider<BingHybridMapProvider>(8),

    _provider<TianDiTuRoadProvider>(9),
    _provider<TianDiTuSatelliteProvider>(10),
    _provider<StatkartTopoMapProvider>(11),
    _provider<StatkartBaseMapProvider>(12),
    _provider<SvalbardMapProvider>(13),

    _provider<EniroMapProvider>(14),

    _provider<EsriWorldStreetMapProvider>(15),
    _provider<EsriWorldSatelliteMapProvider>(16),
    _provider<EsriTerrainMapProvider>(17),

    _provider<MapboxStreetMapProvider>(18),
    _provider<MapboxLightMapProvider>(19),
    _provider<MapboxDarkMapProvider>(20),
    _provider<MapboxSatelliteMapProvider>(21),
    _provider<MapboxHybridMapProvider>(22),
    _provider<MapboxStreetsBasicMapProvider>(23),
    _provider<MapboxOutdoorsMapProvider>(24),
    _provider<MapboxBrightMapProvider>(25),
    _provider<MapboxCustomMapProvider>(26),

    _provider<MapQuestMapMapProvider>(27),
    _provider<MapQuestSatMapProvider>(28),

    _provider<VWorldStreetMapProvider>(29),
    _provider<VWorldSatMapProvider>(30),

    _provider<JapanStdMapProvider>(31),
    _provider<JapanSeamlessMapProvider>(32),
    _provider<JapanAnaglyphMapProvider>(33),
    _provider<JapanSlopeMapProvider>(34),
    _provider<JapanReliefMapProvider>(35),

    _provider<LINZBasemapMapProvider>(36),

    _provider<OpenStreetMapProvider>(37),

    _provider<OpenAIPMapProvider>(38),

    _provider<CustomURLMapProvider>(39),

    _provider<CopernicusElevationProvider>(40),
    _provider<TerrariumElevationProvider>(41)};

QString UrlFactory::getImageFormat(int mapId, QByteArrayView image)
{
    const SharedMapProvider provider = getMapProviderFromMapId(mapId);
    if (provider) {
        return provider->getImageFormat(image);
    }

    return QString("");
}

QString UrlFactory::getImageFormat(QStringView type, QByteArrayView image)
{
    const SharedMapProvider provider = getMapProviderFromProviderType(type);
    if (provider) {
        return provider->getImageFormat(image);
    }

    return QString("");
}

QUrl UrlFactory::getTileURL(int mapId, int x, int y, int zoom)
{
    const SharedMapProvider provider = getMapProviderFromMapId(mapId);
    if (provider) {
        return provider->getTileURL(x, y, zoom);
    }

    return QUrl();
}

QUrl UrlFactory::getTileURL(QStringView type, int x, int y, int zoom)
{
    const SharedMapProvider provider = getMapProviderFromProviderType(type);
    if (provider) {
        return provider->getTileURL(x, y, zoom);
    }

    return QUrl();
}

quint32 UrlFactory::averageSizeForType(QStringView type)
{
    const SharedMapProvider provider = getMapProviderFromProviderType(type);
    if (provider) {
        return provider->getAverageSize();
    }

    return QGC_AVERAGE_TILE_SIZE;
}

bool UrlFactory::isElevation(int mapId)
{
    const SharedMapProvider provider = getMapProviderFromMapId(mapId);
    if (provider) {
        return provider->isElevationProvider();
    }

    return false;
}

int UrlFactory::long2tileX(QStringView mapType, double lon, int z)
{
    const SharedMapProvider provider = getMapProviderFromProviderType(mapType);
    if (provider) {
        return provider->long2tileX(lon, z);
    }

    return 0;
}

int UrlFactory::lat2tileY(QStringView mapType, double lat, int z)
{
    const SharedMapProvider provider = getMapProviderFromProviderType(mapType);
    if (provider) {
        return provider->lat2tileY(lat, z);
    }

    return 0;
}

QGCTileSet UrlFactory::getTileCount(int zoom, double topleftLon, double topleftLat, double bottomRightLon,
                                    double bottomRightLat, QStringView mapType)
{
    const SharedMapProvider provider = getMapProviderFromProviderType(mapType);
    if (provider) {
        zoom = qBound(1, zoom, QGC_MAX_MAP_ZOOM);
        return provider->getTileCount(zoom, topleftLon, topleftLat, bottomRightLon, bottomRightLat);
    }

    return QGCTileSet();
}

QString UrlFactory::getProviderTypeFromMapId(int mapId)
{
    if (mapId == -1) {
        return QString();
    }

    for (const SharedMapProvider& provider : _providers) {
        if (provider->getMapId() == mapId) {
            return provider->getMapName();
        }
    }

    qCWarning(UrlFactoryLog) << "map id not found:" << mapId;
    return QString();
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

    qCWarning(UrlFactoryLog) << "provider not found from id:" << mapId;
    return nullptr;
}

SharedMapProvider UrlFactory::_findProvider(QStringView type)
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

    SharedMapProvider provider = _findProvider(type);
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

QString UrlFactory::getTileHash(QStringView type, int x, int y, int z)
{
    const SharedMapProvider provider = _findProvider(type);
    return _tileHash(provider, type, x, y, z, provider ? provider->cacheVariant() : QString());
}

QString UrlFactory::getTileHash(QStringView type, int x, int y, int z, QStringView cacheVariant)
{
    return _tileHash(_findProvider(type), type, x, y, z, cacheVariant);
}

QString UrlFactory::_tileHash(const SharedMapProvider& provider, QStringView type, int x, int y, int z,
                              QStringView cacheVariant)
{
    if (!provider) {
        // The key is all the caller gets: only this tells that it is the key of no provider
        qCWarning(UrlFactoryLog) << "provider not found for type:" << type;
    }

    const int mapId = provider ? provider->getMapId() : -1;
    return QString::asprintf("%010d%08d%08d%03d", mapId, x, y, z) + cacheVariantSuffix(cacheVariant);
}

QString UrlFactory::cacheVariantSuffix(QStringView cacheVariant)
{
    if (cacheVariant.isEmpty()) {
        return QString();
    }

    // Not qHash(): the keys are stored, and its values differ between Qt versions and runs
    const QByteArray digest = QCryptographicHash::hash(cacheVariant.toUtf8(), QCryptographicHash::Sha1);
    return QLatin1Char('-') + QString::fromLatin1(digest.toHex().left(8));
}

QString UrlFactory::cacheVariant(QStringView type)
{
    const SharedMapProvider provider = _findProvider(type);
    return provider ? provider->cacheVariant() : QString();
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
