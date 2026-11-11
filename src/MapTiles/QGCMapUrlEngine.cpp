#include "QGCMapUrlEngine.h"

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

QGC_LOGGING_CATEGORY(QGCMapUrlEngineLog, "MapTiles.QGCMapUrlEngine")

template <typename Provider>
SharedMapProvider UrlFactory::_provider(int mapId)
{
    const std::shared_ptr<Provider> provider = std::make_shared<Provider>();
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

QString UrlFactory::getImageFormat(int qtMapId, QByteArrayView image)
{
    const SharedMapProvider provider = getMapProviderFromQtMapId(qtMapId);
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

QUrl UrlFactory::getTileURL(int qtMapId, int x, int y, int zoom)
{
    const SharedMapProvider provider = getMapProviderFromQtMapId(qtMapId);
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

bool UrlFactory::isElevation(int qtMapId)
{
    const SharedMapProvider provider = getMapProviderFromQtMapId(qtMapId);
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
        // TODO: zoom = qBound(QGeoCameraCapabilities.minimumZoomLevel(), zoom,
        // QGeoCameraCapabilities.maximumZoomLevel());
        zoom = qBound(1, zoom, QGC_MAX_MAP_ZOOM);
        return provider->getTileCount(zoom, topleftLon, topleftLat, bottomRightLon, bottomRightLat);
    }

    return QGCTileSet();
}

QString UrlFactory::getProviderTypeFromQtMapId(int qtMapId)
{
    if (qtMapId == -1) {
        return QString();
    }

    for (const SharedMapProvider& provider : _providers) {
        if (provider->getMapId() == qtMapId) {
            return provider->getMapName();
        }
    }

    qCWarning(QGCMapUrlEngineLog) << "map id not found:" << qtMapId;
    return QString();
}

SharedMapProvider UrlFactory::getMapProviderFromQtMapId(int qtMapId)
{
    if (qtMapId == -1) {
        return nullptr;
    }

    for (const SharedMapProvider& provider : _providers) {
        if (provider->getMapId() == qtMapId) {
            return provider;
        }
    }

    qCWarning(QGCMapUrlEngineLog) << "provider not found from id:" << qtMapId;
    return nullptr;
}

SharedMapProvider UrlFactory::getMapProviderFromProviderType(QStringView type)
{
    if (type.isEmpty()) {
        return nullptr;
    }

    for (const SharedMapProvider& provider : _providers) {
        if (provider->getMapName() == type) {
            return provider;
        }
    }

    qCWarning(QGCMapUrlEngineLog) << "type not found:" << type;
    return nullptr;
}

int UrlFactory::getQtMapIdFromProviderType(QStringView type)
{
    if (type.isEmpty()) {
        return -1;
    }

    for (const SharedMapProvider& provider : _providers) {
        if (provider->getMapName() == type) {
            return provider->getMapId();
        }
    }

    qCWarning(QGCMapUrlEngineLog) << "type not found:" << type;
    return -1;
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

QString UrlFactory::providerTypeFromHash(int hash)
{
    for (const SharedMapProvider& provider : _providers) {
        const QString mapName = provider->getMapName();
        if (hashFromProviderType(mapName) == hash) {
            return mapName;
        }
    }

    qCWarning(QGCMapUrlEngineLog) << "provider not found from hash:" << hash;
    return QString("");
}

int UrlFactory::hashFromProviderType(QStringView type)
{
    for (const SharedMapProvider& provider : _providers) {
        if (provider->getMapName() == type) {
            return provider->getMapId();
        }
    }

    qCWarning(QGCMapUrlEngineLog) << "provider not found for type:" << type;
    return -1;
}

QString UrlFactory::tileHashToType(QStringView tileHash)
{
    const int providerHash = tileHash.mid(0, 10).toInt();
    return providerTypeFromHash(providerHash);
}

QString UrlFactory::getTileHash(QStringView type, int x, int y, int z)
{
    const int hash = hashFromProviderType(type);
    return QString::asprintf("%010d%08d%08d%03d", hash, x, y, z);
}
