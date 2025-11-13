#include "QGCMapUrlEngine.h"

#include <QtCore/QSet>
#include <QtCore/QtMinMax>

#include "BingMapProvider.h"
#include "ElevationMapProvider.h"
#include "EsriMapProvider.h"
#include "GenericMapProvider.h"
#include "GoogleMapProvider.h"
#include "MapboxMapProvider.h"
#include "TianDiTuProvider.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(QGCMapUrlEngineLog, "QtLocationPlugin.QGCMapUrlEngine")

const QList<SharedMapProvider> UrlFactory::_providers = {
#ifndef QGC_NO_GOOGLE_MAPS
    std::make_shared<GoogleStreetMapProvider>(),
    std::make_shared<GoogleSatelliteMapProvider>(),
    std::make_shared<GoogleTerrainMapProvider>(),
    std::make_shared<GoogleHybridMapProvider>(),
    std::make_shared<GoogleLabelsMapProvider>(),
#endif
    std::make_shared<BingRoadMapProvider>(),
    std::make_shared<BingSatelliteMapProvider>(),
    std::make_shared<BingHybridMapProvider>(),

    std::make_shared<TianDiTuRoadProvider>(),
    std::make_shared<TianDiTuSatelliteProvider>(),
    std::make_shared<StatkartTopoMapProvider>(),
    std::make_shared<StatkartBaseMapProvider>(),
    std::make_shared<SvalbardMapProvider>(),

    std::make_shared<EniroMapProvider>(),

    std::make_shared<EsriWorldStreetMapProvider>(),
    std::make_shared<EsriWorldSatelliteMapProvider>(),
    std::make_shared<EsriTerrainMapProvider>(),

    std::make_shared<MapboxStreetMapProvider>(),
    std::make_shared<MapboxLightMapProvider>(),
    std::make_shared<MapboxDarkMapProvider>(),
    std::make_shared<MapboxSatelliteMapProvider>(),
    std::make_shared<MapboxHybridMapProvider>(),
    std::make_shared<MapboxStreetsBasicMapProvider>(),
    std::make_shared<MapboxOutdoorsMapProvider>(),
    std::make_shared<MapboxBrightMapProvider>(),
    std::make_shared<MapboxCustomMapProvider>(),

    std::make_shared<MapQuestMapMapProvider>(),
    std::make_shared<MapQuestSatMapProvider>(),

    std::make_shared<VWorldStreetMapProvider>(),
    std::make_shared<VWorldSatMapProvider>(),

    std::make_shared<JapanStdMapProvider>(),
    std::make_shared<JapanSeamlessMapProvider>(),
    std::make_shared<JapanAnaglyphMapProvider>(),
    std::make_shared<JapanSlopeMapProvider>(),
    std::make_shared<JapanReliefMapProvider>(),

    std::make_shared<LINZBasemapMapProvider>(),

    std::make_shared<OpenStreetMapProvider>(),

    std::make_shared<OpenAIPMapProvider>(),

    std::make_shared<CustomURLMapProvider>(),

    std::make_shared<CopernicusElevationProvider>()
};

// Initialize static lookup tables
std::once_flag UrlFactory::_initFlag;
QHash<int, SharedMapProvider> UrlFactory::_providersByMapId;
QHash<QString, SharedMapProvider> UrlFactory::_providersByName;

void UrlFactory::_initializeLookupTables()
{
    _providersByMapId.reserve(_providers.size());
    _providersByName.reserve(_providers.size());

    for (const SharedMapProvider& provider : _providers) {
        _providersByMapId.insert(provider->getMapId(), provider);
        _providersByName.insert(provider->getMapName(), provider);
    }
}

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

QGCTileSet UrlFactory::getTileCount(int zoom, double topleftLon, double topleftLat, double bottomRightLon, double bottomRightLat, QStringView mapType)
{
    const SharedMapProvider provider = getMapProviderFromProviderType(mapType);
    if (provider) {
        // TODO: zoom = qBound(QGeoCameraCapabilities.minimumZoomLevel(), zoom, QGeoCameraCapabilities.maximumZoomLevel());
        zoom = qBound(1, zoom, QGC_MAX_MAP_ZOOM);
        return provider->getTileCount(zoom, topleftLon, topleftLat, bottomRightLon, bottomRightLat);
    }

    return QGCTileSet();
}

QString UrlFactory::getProviderTypeFromQtMapId(int qtMapId)
{
    if (qtMapId == defaultSetMapId()) {
        return QString();
    }

    std::call_once(_initFlag, &UrlFactory::_initializeLookupTables);

    const SharedMapProvider provider = _providersByMapId.value(qtMapId, nullptr);
    if (provider) {
        return provider->getMapName();
    }

    static QSet<int> warnedIds;
    if (!warnedIds.contains(qtMapId)) {
        warnedIds.insert(qtMapId);
        qCWarning(QGCMapUrlEngineLog) << "map id not found:" << qtMapId;
    }
    return QString::number(qtMapId);
}

SharedMapProvider UrlFactory::getMapProviderFromQtMapId(int qtMapId)
{
    if (qtMapId == defaultSetMapId()) {
        return nullptr;
    }

    std::call_once(_initFlag, &UrlFactory::_initializeLookupTables);

    const SharedMapProvider provider = _providersByMapId.value(qtMapId, nullptr);
    if (!provider) {
        static QSet<int> warnedIds;
        if (!warnedIds.contains(qtMapId)) {
            warnedIds.insert(qtMapId);
            qCWarning(QGCMapUrlEngineLog) << "provider not found from id:" << qtMapId;
        }
    }
    return provider;
}

SharedMapProvider UrlFactory::getMapProviderFromProviderType(QStringView type)
{
    if (type.isEmpty()) {
        return nullptr;
    }

    bool ok = false;
    const int numericId = type.toInt(&ok);
    if (ok) {
        return getMapProviderFromQtMapId(numericId);
    }

    std::call_once(_initFlag, &UrlFactory::_initializeLookupTables);

    const QString typeStr = type.toString();
    const SharedMapProvider provider = _providersByName.value(typeStr, nullptr);
    if (!provider) {
        qCWarning(QGCMapUrlEngineLog) << "type not found:" << type;
    }
    return provider;
}

int UrlFactory::getQtMapIdFromProviderType(QStringView type)
{
    if (type.isEmpty()) {
        return defaultSetMapId();
    }

    bool ok = false;
    const int numericId = type.toInt(&ok);
    if (ok) {
        return numericId;
    }

    std::call_once(_initFlag, &UrlFactory::_initializeLookupTables);

    const QString typeStr = type.toString();
    const SharedMapProvider provider = _providersByName.value(typeStr, nullptr);
    if (provider) {
        return provider->getMapId();
    }

    qCWarning(QGCMapUrlEngineLog) << "type not found:" << type;
    return -1;
}

QStringList UrlFactory::getElevationProviderTypes()
{
    QStringList types;
    for (const SharedMapProvider &provider : _providers) {
        if (provider->isElevationProvider()) {
            types.append(provider->getMapName());
        }
    }

    return types;
}

QStringList UrlFactory::getProviderTypes()
{
    QStringList types;
    for (const SharedMapProvider &provider : _providers) {
        types.append(provider->getMapName());
    }

    return types;
}

QString UrlFactory::providerTypeFromHash(int hash)
{
    for (const SharedMapProvider &provider : _providers) {
        const QString mapName = provider->getMapName();
        if (hashFromProviderType(mapName) == hash) {
            return mapName;
        }
    }

    qCWarning(QGCMapUrlEngineLog) << "provider not found from hash:" << hash;
    return QString("");
}

// This seems to limit provider name length to less than ~25 chars due to downcasting to int
int UrlFactory::hashFromProviderType(QStringView type)
{
    const quint32 hash = qHash(type);
    return static_cast<int>(hash & 0x7fffffff);
}

QString UrlFactory::tileHashToType(QStringView tileHash)
{
    if (tileHash.isEmpty()) {
        return QString();
    }

    bool ok = false;
    const int providerHash = tileHash.mid(0, 10).toInt(&ok);
    if (!ok) {
        qCWarning(QGCMapUrlEngineLog) << "Invalid tile hash" << tileHash;
        return QString();
    }

    if (providerHash <= 0) {
        return getProviderTypeFromQtMapId(providerHash);
    }

    const QString type = providerTypeFromHash(providerHash);
    if (!type.isEmpty()) {
        return type;
    }

    return getProviderTypeFromQtMapId(providerHash);
}

QString UrlFactory::getTileHash(QStringView type, int x, int y, int z)
{
    const int hash = hashFromProviderType(type);
    return QString::asprintf("%010d%08d%08d%03d", hash, x, y, z);
}
