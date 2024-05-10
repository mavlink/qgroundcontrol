/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

/**
 *  @file
 *  @author Gus Grubba <gus@auterion.com>
 *  Original work: The OpenPilot Team, http://www.openpilot.org Copyright (C)
 * 2012.
 */

#include "QGCMapUrlEngine.h"
#include "GoogleMapProvider.h"
#include "BingMapProvider.h"
#include "EsriMapProvider.h"
#include "GenericMapProvider.h"
#include "MapboxMapProvider.h"
#include "ElevationMapProvider.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(QGCMapUrlEngineLog, "QGCMapUrlEngineLog")

const QList<SharedMapProvider> UrlFactory::m_providers = {
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

    std::make_shared<StatkartMapProvider>(),
    std::make_shared<StatkartBaseMapProvider>(),

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

    std::make_shared<CopernicusElevationProvider>(),

    std::make_shared<JapanStdMapProvider>(),
    std::make_shared<JapanSeamlessMapProvider>(),
    std::make_shared<JapanAnaglyphMapProvider>(),
    std::make_shared<JapanSlopeMapProvider>(),
    std::make_shared<JapanReliefMapProvider>(),

    std::make_shared<LINZBasemapMapProvider>(),

    std::make_shared<CustomURLMapProvider>()
};

QString UrlFactory::getImageFormat(int qtMapId, QByteArrayView image)
{
    const SharedMapProvider provider = getProviderFromQtMapId(qtMapId);
    if (provider) {
        return provider->getImageFormat(image);
    }

    qCWarning(QGCMapUrlEngineLog) << Q_FUNC_INFO << "map id not found:" << qtMapId;
    return QString();
}

QString UrlFactory::getImageFormat(QStringView type, QByteArrayView image)
{
    const SharedMapProvider provider =  getProviderFromProviderType(type);
    if (provider) {
        return provider->getImageFormat(image);
    }

    qCWarning(QGCMapUrlEngineLog) << Q_FUNC_INFO << "type not found:" << type;
    return QString();
}

QString UrlFactory::getTileURL(int qtMapId, int x, int y, int zoom)
{
    const SharedMapProvider provider = getProviderFromQtMapId(qtMapId);
    if (provider) {
        return provider->getTileURL(x, y, zoom);
    }

    qCWarning(QGCMapUrlEngineLog) << Q_FUNC_INFO << "map id not found:" << qtMapId;
    return QString();
}

QString UrlFactory::getTileURL(QStringView type, int x, int y, int zoom)
{
    const SharedMapProvider provider = getProviderFromProviderType(type);
    if (provider) {
        return provider->getTileURL(x, y, zoom);
    }

    qCWarning(QGCMapUrlEngineLog) << Q_FUNC_INFO << "type not found:" << type;
    return QString();
}

quint32 UrlFactory::averageSizeForType(QStringView type)
{
    const SharedMapProvider provider = getProviderFromProviderType(type);
    if (provider) {
        return provider->getAverageSize();
    }

    qCWarning(QGCMapUrlEngineLog) << Q_FUNC_INFO << "type not found:" << type;
    return AVERAGE_TILE_SIZE;
}

QString UrlFactory::getProviderTypeFromQtMapId(int qtMapId)
{
    const SharedMapProvider provider = getProviderFromQtMapId(qtMapId);
    if (provider) {
        return provider->getMapName();
    }

    // Default Set
    if(qtMapId == -1) {
        return nullptr;
    }

    qCWarning(QGCMapUrlEngineLog) << Q_FUNC_INFO << "map id not found:" << qtMapId;
    return m_providers.first()->getMapName();
}

SharedMapProvider UrlFactory::getProviderFromProviderType(QStringView type)
{
    for (SharedMapProvider provider : m_providers) {
        if (provider->getMapName() == type) {
            return provider;
        }
    }

    qCWarning(QGCMapUrlEngineLog) << Q_FUNC_INFO << "type not found:" << type;
    return nullptr;
}

SharedMapProvider UrlFactory::getProviderFromQtMapId(int qtMapId)
{
    for (SharedMapProvider provider : m_providers) {
        if (provider->getMapId() == qtMapId) {
            return provider;
        }
    }

    // Default Set
    if(qtMapId == -1) {
        return nullptr;
    }

    qCWarning(QGCMapUrlEngineLog) << Q_FUNC_INFO << "map id not found:" << qtMapId;
    return nullptr;
}

int UrlFactory::getQtMapIdFromProviderType(QStringView type)
{
    for (const SharedMapProvider provider : m_providers) {
        if (provider->getMapName() == type) {
            return provider->getMapId();
        }
    }

    qCWarning(QGCMapUrlEngineLog) << Q_FUNC_INFO << "type not found:" << type;
    return 1;
}

int UrlFactory::long2tileX(QStringView mapType, double lon, int z)
{
    const SharedMapProvider provider = getProviderFromProviderType(mapType);
    if (provider) {
        return provider->long2tileX(lon, z);
    }

    qCWarning(QGCMapUrlEngineLog) << Q_FUNC_INFO << "type not found:" << mapType;
    return 0;
}

int UrlFactory::lat2tileY(QStringView mapType, double lat, int z)
{
    const SharedMapProvider provider = getProviderFromProviderType(mapType);
    if (provider) {
        return provider->lat2tileY(lat, z);
    }

    qCWarning(QGCMapUrlEngineLog) << Q_FUNC_INFO << "type not found:" << mapType;
    return 0;
}

QGCTileSet UrlFactory::getTileCount(int zoom, double topleftLon, double topleftLat, double bottomRightLon, double bottomRightLat, QStringView mapType)
{
    const SharedMapProvider provider = getProviderFromProviderType(mapType);
    if (provider) {
        // TODO: Check QGeoCameraCapabilities
        if(zoom < 1) {
            zoom = 1;
        } else if(zoom > MAX_MAP_ZOOM) {
            zoom = MAX_MAP_ZOOM;
        }
        return provider->getTileCount(zoom, topleftLon, topleftLat, bottomRightLon, bottomRightLat);
    }

    qCWarning(QGCMapUrlEngineLog) << Q_FUNC_INFO << "type not found:" << mapType;
    return QGCTileSet();
}

bool UrlFactory::isElevation(int qtMapId)
{
    const SharedMapProvider provider = getProviderFromQtMapId(qtMapId);
    if (provider) {
        return provider->isElevationProvider();
    }

    qCWarning(QGCMapUrlEngineLog) << Q_FUNC_INFO << "map id not found:" << qtMapId;
    return false;
}

const SharedMapProvider UrlFactory::getElevationProvider()
{
    for (const SharedMapProvider provider : m_providers) {
        if (provider->isElevationProvider()) {
            return provider;
        }
    }

    qCWarning(QGCMapUrlEngineLog) << Q_FUNC_INFO << "elevation provider not found";
    return nullptr;
}

QStringList UrlFactory::getProviderTypes()
{
    QStringList types;
    for (const SharedMapProvider provider : m_providers) {
        types.append(provider->getMapName());
    }

    return types;
}

int UrlFactory::hashFromProviderType(QStringView type)
{
    const int result = static_cast<int>(qHash(type) >> 1);
    return result;
}

QString UrlFactory::getProviderTypeFromHash(int hash)
{
    for (const SharedMapProvider provider : m_providers) {
        const QString mapName = provider->getMapName();
        if (hashFromProviderType(mapName) == hash) {
            return mapName;
        }
    }

    qCWarning(QGCMapUrlEngineLog) << Q_FUNC_INFO << "provider not found from hash" << hash;
    return QStringLiteral("");
}

QString UrlFactory::tileHashToType(QStringView tileHash)
{
    const int providerHash = tileHash.mid(0,10).toInt();
    return UrlFactory::getProviderTypeFromHash(providerHash);
}

QString UrlFactory::getTileHash(QStringView type, int x, int y, int z)
{
    const int hash = UrlFactory::hashFromProviderType(type);
    return QString::asprintf("%010d%08d%08d%03d", hash, x, y, z);
}

// TODO
/*bool UrlFactory::initializeCustomMapSources(QGeoServiceProvider::Error *error,
                                              QString *errorString,
                                              const QGeoCameraCapabilities &cameraCaps)
{
    QFile mapsFile(":/MapProviders/maps.json");

    if (!mapsFile.open(QIODevice::ReadOnly)) {
        *error = QGeoServiceProvider::NotSupportedError;
        *errorString = Q_FUNC_INFO + QStringLiteral("Unable to open: ") + mapsFile.fileName();

        return false;
    }

    const QByteArray mapsData = mapsFile.readAll();
    mapsFile.close();

    QJsonParseError parseError;
    const QJsonDocument mapsDocument = QJsonDocument::fromJson(mapsData, &parseError);

    if (!mapsDocument.isObject()) {
        *error = QGeoServiceProvider::NotSupportedError;
        *errorString = QString("%1JSON error: %2, offset: %3, details: %4")
                            .arg(Q_FUNC_INFO)
                            .arg((int)parseError.error)
                            .arg(parseError.offset)
                            .arg(parseError.errorString());
        return false;
    }

    const QVariantMap maps = mapsDocument.object().toVariantMap();
    const QVariantList mapSources = maps["mapSources"].toList();

    for (const QVariant &mapSourceElement : mapSources) {
        const QVariantMap mapSource = mapSourceElement.toMap();
        const int mapId = m_providers.count() + 1;

        MapProvider* provider = new MapProvider(
            GeoMapSource::mapStyle(mapSource[kPropStyle].toString()),
            mapSource[kPropName].toString(),
            mapSource[kPropDescription].toString(),
            mapSource[kPropMobile].toBool(),
            mapSource[kPropMapId].toBool(),
            mapId,
            GeoMapSource::toFormat(mapSource[kPropUrl].toString()),
            mapSource[kPropCopyright].toString(),
            cameraCaps
        );
        m_providers << provider;
    }

    return true;
}*/
