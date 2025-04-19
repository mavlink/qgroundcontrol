/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
#include "GenericMapProvider.h"
#include "EsriMapProvider.h"
#include "MapboxMapProvider.h"
#include "ElevationMapProvider.h"
#include <QGCLoggingCategory.h>

QGC_LOGGING_CATEGORY(QGCMapUrlEngineLog, "qgc.qtlocationplugin.qgcmapurlengine")

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

    std::make_shared<CustomURLMapProvider>(),

    std::make_shared<CopernicusElevationProvider>()
};

QString UrlFactory::getImageFormat(int qtMapId, QByteArrayView image)
{
    const SharedMapProvider provider = getMapProviderFromQtMapId(qtMapId);
    if (provider) {
        return provider->getImageFormat(image);
    }

    return QStringLiteral("");
}

QString UrlFactory::getImageFormat(QStringView type, QByteArrayView image)
{
    const SharedMapProvider provider =  getMapProviderFromProviderType(type);
    if (provider) {
        return provider->getImageFormat(image);
    }

    return QStringLiteral("");
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

    return AVERAGE_TILE_SIZE;
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
        // TODO: Check QGeoCameraCapabilities.maximumZoomLevel() and QGeoCameraCapabilities.minimumZoomLevel()
        if(zoom < 1) {
            zoom = 1;
        } else if(zoom > MAX_MAP_ZOOM) {
            zoom = MAX_MAP_ZOOM;
        }
        return provider->getTileCount(zoom, topleftLon, topleftLat, bottomRightLon, bottomRightLat);
    }

    return QGCTileSet();
}

QString UrlFactory::getProviderTypeFromQtMapId(int qtMapId)
{
    // Default Set
    if(qtMapId == -1) {
        return nullptr;
    }

    for (const SharedMapProvider &provider : _providers) {
        if (provider->getMapId() == qtMapId) {
            return provider->getMapName();
        }
    }

    qCWarning(QGCMapUrlEngineLog) << Q_FUNC_INFO << "map id not found:" << qtMapId;
    return QStringLiteral("");
}

SharedMapProvider UrlFactory::getMapProviderFromQtMapId(int qtMapId)
{
    // Default Set
    if(qtMapId == -1) {
        return nullptr;
    }

    for (const SharedMapProvider &provider : _providers) {
        if (provider->getMapId() == qtMapId) {
            return provider;
        }
    }

    qCWarning(QGCMapUrlEngineLog) << Q_FUNC_INFO << "provider not found from id:" << qtMapId;
    return nullptr;
}

SharedMapProvider UrlFactory::getMapProviderFromProviderType(QStringView type)
{
    for (const SharedMapProvider &provider : _providers) {
        if (provider->getMapName() == type) {
            return provider;
        }
    }

    qCWarning(QGCMapUrlEngineLog) << Q_FUNC_INFO << "type not found:" << type;
    return nullptr;
}

int UrlFactory::getQtMapIdFromProviderType(QStringView type)
{
    for (const SharedMapProvider &provider : _providers) {
        if (provider->getMapName() == type) {
            return provider->getMapId();
        }
    }

    qCWarning(QGCMapUrlEngineLog) << Q_FUNC_INFO << "type not found:" << type;
    return -1;
}

QStringList UrlFactory::getElevationProviderTypes()
{
    QStringList types;
    for (const SharedMapProvider &provider : _providers) {
        if (provider->isElevationProvider()) {
            (void) types.append(provider->getMapName());
        }
    }

    return types;
}

QStringList UrlFactory::getProviderTypes()
{
    QStringList types;
    for (const SharedMapProvider &provider : _providers) {
        (void) types.append(provider->getMapName());
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

    qCWarning(QGCMapUrlEngineLog) << Q_FUNC_INFO << "provider not found from hash:" << hash;
    return QStringLiteral("");
}

// This seems to limit provider name length to less than ~25 chars due to downcasting to int
int UrlFactory::hashFromProviderType(QStringView type)
{
    const auto hash = qHash(type) >> 1;
    return static_cast<int>(hash);
}

QString UrlFactory::tileHashToType(QStringView tileHash)
{
    const int providerHash = tileHash.mid(0,10).toInt();
    return providerTypeFromHash(providerHash);
}

QString UrlFactory::getTileHash(QStringView type, int x, int y, int z)
{
    const int hash = hashFromProviderType(type);
    return QString::asprintf("%010d%08d%08d%03d", hash, x, y, z);
}
