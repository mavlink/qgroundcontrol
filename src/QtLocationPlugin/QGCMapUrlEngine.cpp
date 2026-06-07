#include "QGCMapUrlEngine.h"

#include <QtCore/QSet>
#include <QtCore/QUrl>
#include <QtCore/QtMinMax>
#include <QtGui/QGuiApplication>

#include "BingMapProvider.h"
#include "ElevationMapProvider.h"
#include "EsriMapProvider.h"
#include "GenericMapProvider.h"
#include "GoogleMapProvider.h"
#include "MapboxMapProvider.h"
#include "QGCLoggingCategory.h"
#include "QGCTileSet.h"
#include "TianDiTuProvider.h"

QGC_LOGGING_CATEGORY(QGCMapUrlEngineLog, "QtLocationPlugin.QGCMapUrlEngine")

const QList<SharedMapProvider> UrlFactory::_providers = {
#ifndef QGC_NO_GOOGLE_MAPS
    std::make_shared<GoogleMapProvider>(QStringLiteral("Google Street Map"), QStringLiteral("lyrs"),
                                        QStringLiteral("m"), QStringLiteral("png"), AVERAGE_GOOGLE_STREET_MAP,
                                        MapProvider::StreetMap),
    std::make_shared<GoogleMapProvider>(QStringLiteral("Google Satellite"), QStringLiteral("lyrs"), QStringLiteral("s"),
                                        QStringLiteral("jpg"), AVERAGE_GOOGLE_SAT_MAP, MapProvider::SatelliteMapDay),
    std::make_shared<GoogleMapProvider>(QStringLiteral("Google Terrain"), QStringLiteral("v"), QStringLiteral("t,r"),
                                        QStringLiteral("png"), AVERAGE_GOOGLE_TERRAIN_MAP, MapProvider::TerrainMap),
    std::make_shared<GoogleMapProvider>(QStringLiteral("Google Hybrid"), QStringLiteral("lyrs"), QStringLiteral("y"),
                                        QStringLiteral("png"), AVERAGE_GOOGLE_SAT_MAP, MapProvider::HybridMap),
    std::make_shared<GoogleMapProvider>(QStringLiteral("Google Labels"), QStringLiteral("lyrs"), QStringLiteral("h"),
                                        QStringLiteral("png"), QGC_AVERAGE_TILE_SIZE, MapProvider::CustomMap),
#endif
    std::make_shared<BingMapProvider>(QStringLiteral("Bing Road"), QStringLiteral("r"), QStringLiteral("png"),
                                      AVERAGE_BING_STREET_MAP, MapProvider::StreetMap),
    std::make_shared<BingMapProvider>(QStringLiteral("Bing Satellite"), QStringLiteral("a"), QStringLiteral("jpg"),
                                      AVERAGE_BING_SAT_MAP, MapProvider::SatelliteMapDay),
    std::make_shared<BingMapProvider>(QStringLiteral("Bing Hybrid"), QStringLiteral("h"), QStringLiteral("jpg"),
                                      AVERAGE_BING_SAT_MAP, MapProvider::HybridMap),

    std::make_shared<TianDiTuProvider>(QObject::tr("TianDiTu Road"), QStringLiteral("cia_w"), QStringLiteral("png"),
                                       AVERAGE_TIANDITU_STREET_MAP, MapProvider::StreetMap),
    std::make_shared<TianDiTuProvider>(QObject::tr("TianDiTu Satellite"), QStringLiteral("img_w"),
                                       QStringLiteral("jpg"), AVERAGE_TIANDITU_SAT_MAP, MapProvider::SatelliteMapDay),
    std::make_shared<TemplateMapProvider>(MapProviderConfig{
        .name = QStringLiteral("Statkart Topo"),
        .referrer = QStringLiteral("https://norgeskart.no/"),
        .urlTemplate = QStringLiteral("https://cache.kartverket.no/v1/wmts/1.0.0/%1/default/webmercator/%2/%3/%4.png"),
        .imageFormat = QStringLiteral("png"),
        .mapStyle = MapProvider::StreetMap,
        .mapTypeId = QStringLiteral("topo4"),
        .axisOrder = MapProviderConfig::ZYX}),
    std::make_shared<TemplateMapProvider>(MapProviderConfig{
        .name = QStringLiteral("Statkart Basemap"),
        .referrer = QStringLiteral("https://norgeskart.no/"),
        .urlTemplate = QStringLiteral("https://cache.kartverket.no/v1/wmts/1.0.0/%1/default/webmercator/%2/%3/%4.png"),
        .imageFormat = QStringLiteral("png"),
        .mapStyle = MapProvider::StreetMap,
        .mapTypeId = QStringLiteral("norgeskart_bakgrunn"),
        .axisOrder = MapProviderConfig::ZYX}),
    std::make_shared<TemplateMapProvider>(MapProviderConfig{
        .name = QStringLiteral("Svalbard Topo"),
        .referrer = QStringLiteral("https://www.npolar.no/"),
        .urlTemplate = QStringLiteral(
            "https://geodata.npolar.no/arcgis/rest/services/Basisdata/NP_Basiskart_Svalbard_WMTS_3857/MapServer/WMTS/"
            "tile/1.0.0/Basisdata_NP_Basiskart_Svalbard_WMTS_3857/default/default028mm/%1/%2/%3"),
        .imageFormat = QStringLiteral("png"),
        .mapStyle = MapProvider::StreetMap,
        .axisOrder = MapProviderConfig::ZYX}),

    std::make_shared<TemplateMapProvider>(MapProviderConfig{
        .name = QStringLiteral("Eniro Topo"),
        .referrer = QStringLiteral("https://www.eniro.se/"),
        .urlTemplate = QStringLiteral("https://map.eniro.com/geowebcache/service/tms1.0.0/map/%1/%2/%3.%4"),
        .imageFormat = QStringLiteral("png"),
        .mapStyle = MapProvider::StreetMap,
        .flipY = true,
        .appendImageFormat = true}),

    std::make_shared<EsriMapProvider>(QStringLiteral("Esri World Street"), QStringLiteral("World_Street_Map"),
                                      QGC_AVERAGE_TILE_SIZE, MapProvider::StreetMap),
    std::make_shared<EsriMapProvider>(QStringLiteral("Esri World Satellite"), QStringLiteral("World_Imagery"),
                                      QGC_AVERAGE_TILE_SIZE, MapProvider::SatelliteMapDay),
    std::make_shared<EsriMapProvider>(QStringLiteral("Esri Terrain"), QStringLiteral("World_Terrain_Base"),
                                      QGC_AVERAGE_TILE_SIZE, MapProvider::TerrainMap),

    std::make_shared<MapboxMapProvider>(QStringLiteral("Mapbox Streets"), QStringLiteral("streets-v10"),
                                        AVERAGE_MAPBOX_STREET_MAP, MapProvider::StreetMap),
    std::make_shared<MapboxMapProvider>(QStringLiteral("Mapbox Light"), QStringLiteral("light-v9"),
                                        QGC_AVERAGE_TILE_SIZE, MapProvider::CustomMap),
    std::make_shared<MapboxMapProvider>(QStringLiteral("Mapbox Dark"), QStringLiteral("dark-v9"), QGC_AVERAGE_TILE_SIZE,
                                        MapProvider::CustomMap),
    std::make_shared<MapboxMapProvider>(QStringLiteral("Mapbox Satellite"), QStringLiteral("satellite-v9"),
                                        AVERAGE_MAPBOX_SAT_MAP, MapProvider::SatelliteMapDay),
    std::make_shared<MapboxMapProvider>(QStringLiteral("Mapbox Hybrid"), QStringLiteral("satellite-streets-v10"),
                                        AVERAGE_MAPBOX_SAT_MAP, MapProvider::HybridMap),
    std::make_shared<MapboxMapProvider>(QStringLiteral("Mapbox StreetsBasic"), QStringLiteral("basic-v9"),
                                        QGC_AVERAGE_TILE_SIZE, MapProvider::StreetMap),
    std::make_shared<MapboxMapProvider>(QStringLiteral("Mapbox Outdoors"), QStringLiteral("outdoors-v10"),
                                        QGC_AVERAGE_TILE_SIZE, MapProvider::CustomMap),
    std::make_shared<MapboxMapProvider>(QStringLiteral("Mapbox Bright"), QStringLiteral("bright-v9"),
                                        QGC_AVERAGE_TILE_SIZE, MapProvider::CustomMap),
    std::make_shared<MapboxMapProvider>(QStringLiteral("Mapbox Custom"), QStringLiteral("mapbox.custom"),
                                        QGC_AVERAGE_TILE_SIZE, MapProvider::CustomMap),

    std::make_shared<TemplateMapProvider>(
        MapProviderConfig{.name = QStringLiteral("MapQuest Map"),
                          .referrer = QStringLiteral("https://mapquest.com"),
                          .urlTemplate = QStringLiteral("https://otile%1.mqcdn.com/tiles/1.0.0/%2/%3/%4/%5.%6"),
                          .imageFormat = QStringLiteral("jpg"),
                          .mapStyle = MapProvider::StreetMap,
                          .mapTypeId = QStringLiteral("map"),
                          .appendImageFormat = true,
                          .serverCount = 4}),
    std::make_shared<TemplateMapProvider>(
        MapProviderConfig{.name = QStringLiteral("MapQuest Sat"),
                          .referrer = QStringLiteral("https://mapquest.com"),
                          .urlTemplate = QStringLiteral("https://otile%1.mqcdn.com/tiles/1.0.0/%2/%3/%4/%5.%6"),
                          .imageFormat = QStringLiteral("jpg"),
                          .mapStyle = MapProvider::SatelliteMapDay,
                          .mapTypeId = QStringLiteral("sat"),
                          .appendImageFormat = true,
                          .serverCount = 4}),

    std::make_shared<VWorldStreetMapProvider>(),
    std::make_shared<VWorldSatMapProvider>(),

    std::make_shared<TemplateMapProvider>(
        MapProviderConfig{.name = QStringLiteral("Japan-GSI Contour"),
                          .referrer = QStringLiteral("https://cyberjapandata.gsi.go.jp/xyz/std"),
                          .urlTemplate = QStringLiteral("https://cyberjapandata.gsi.go.jp/xyz/%1/%2/%3/%4.%5"),
                          .imageFormat = QStringLiteral("png"),
                          .mapStyle = MapProvider::StreetMap,
                          .mapTypeId = QStringLiteral("std"),
                          .appendImageFormat = true}),
    std::make_shared<TemplateMapProvider>(
        MapProviderConfig{.name = QStringLiteral("Japan-GSI Seamless"),
                          .referrer = QStringLiteral("https://cyberjapandata.gsi.go.jp/xyz/std"),
                          .urlTemplate = QStringLiteral("https://cyberjapandata.gsi.go.jp/xyz/%1/%2/%3/%4.%5"),
                          .imageFormat = QStringLiteral("jpg"),
                          .mapStyle = MapProvider::StreetMap,
                          .mapTypeId = QStringLiteral("seamlessphoto"),
                          .appendImageFormat = true}),
    std::make_shared<TemplateMapProvider>(
        MapProviderConfig{.name = QStringLiteral("Japan-GSI Anaglyph"),
                          .referrer = QStringLiteral("https://cyberjapandata.gsi.go.jp/xyz/std"),
                          .urlTemplate = QStringLiteral("https://cyberjapandata.gsi.go.jp/xyz/%1/%2/%3/%4.%5"),
                          .imageFormat = QStringLiteral("png"),
                          .mapStyle = MapProvider::StreetMap,
                          .mapTypeId = QStringLiteral("anaglyphmap_color"),
                          .appendImageFormat = true}),
    std::make_shared<TemplateMapProvider>(
        MapProviderConfig{.name = QStringLiteral("Japan-GSI Slope"),
                          .referrer = QStringLiteral("https://cyberjapandata.gsi.go.jp/xyz/std"),
                          .urlTemplate = QStringLiteral("https://cyberjapandata.gsi.go.jp/xyz/%1/%2/%3/%4.%5"),
                          .imageFormat = QStringLiteral("png"),
                          .mapStyle = MapProvider::StreetMap,
                          .mapTypeId = QStringLiteral("slopemap"),
                          .appendImageFormat = true}),
    std::make_shared<TemplateMapProvider>(
        MapProviderConfig{.name = QStringLiteral("Japan-GSI Relief"),
                          .referrer = QStringLiteral("https://cyberjapandata.gsi.go.jp/xyz/std"),
                          .urlTemplate = QStringLiteral("https://cyberjapandata.gsi.go.jp/xyz/%1/%2/%3/%4.%5"),
                          .imageFormat = QStringLiteral("png"),
                          .mapStyle = MapProvider::StreetMap,
                          .mapTypeId = QStringLiteral("relief"),
                          .appendImageFormat = true}),

    std::make_shared<LINZBasemapMapProvider>(),

    std::make_shared<TemplateMapProvider>(
        MapProviderConfig{.name = QStringLiteral("Street Map"),
                          .referrer = QStringLiteral("https://www.openstreetmap.org"),
                          .urlTemplate = QStringLiteral("https://tile.openstreetmap.org/%1/%2/%3.png"),
                          .imageFormat = QStringLiteral("png"),
                          .mapStyle = MapProvider::StreetMap,
                          .isOSM = true}),

    std::make_shared<OpenAIPMapProvider>(),

    std::make_shared<CustomURLMapProvider>(),

    std::make_shared<CopernicusElevationProvider>()};

QHash<int, SharedMapProvider> UrlFactory::_providersByMapId;
QHash<QString, SharedMapProvider> UrlFactory::_providersByName;
std::once_flag UrlFactory::_initFlag;

void UrlFactory::_ensureInitialized()
{
    std::call_once(_initFlag, [] {
        for (const SharedMapProvider& provider : _providers) {
            _providersByMapId.insert(provider->getMapId(), provider);
            _providersByName.insert(provider->getMapName(), provider);
        }
    });
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
    return provider && std::dynamic_pointer_cast<const ElevationProvider>(provider) != nullptr;
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
    if (qtMapId == defaultSetMapId()) {
        return QString();
    }

    const SharedMapProvider provider = getMapProviderFromQtMapId(qtMapId);
    if (provider) {
        return provider->getMapName();
    }

    return QString();
}

SharedMapProvider UrlFactory::getMapProviderFromQtMapId(int qtMapId)
{
    if (qtMapId == defaultSetMapId()) {
        return nullptr;
    }

    _ensureInitialized();

    const auto it = _providersByMapId.constFind(qtMapId);
    if (it != _providersByMapId.constEnd()) {
        return it.value();
    }

    thread_local QSet<int> s_warnedIds;
    if (!s_warnedIds.contains(qtMapId)) {
        s_warnedIds.insert(qtMapId);
        qCWarning(QGCMapUrlEngineLog) << "provider not found from id:" << qtMapId;
    }
    return nullptr;
}

SharedMapProvider UrlFactory::getMapProviderFromProviderType(QStringView type)
{
    if (type.isEmpty()) {
        return nullptr;
    }

    _ensureInitialized();

    const QString typeStr = type.toString();
    const auto it = _providersByName.constFind(typeStr);
    if (it != _providersByName.constEnd()) {
        return it.value();
    }

    thread_local QSet<QString> s_warnedTypes;
    if (!s_warnedTypes.contains(typeStr)) {
        s_warnedTypes.insert(typeStr);
        qCWarning(QGCMapUrlEngineLog) << "type not found:" << type;
    }
    return nullptr;
}

int UrlFactory::getQtMapIdFromProviderType(QStringView type)
{
    if (type.isEmpty()) {
        return -1;
    }

    const SharedMapProvider provider = getMapProviderFromProviderType(type);
    if (provider) {
        return provider->getMapId();
    }

    return -1;
}

QStringList UrlFactory::getElevationProviderTypes()
{
    QStringList types;
    for (const SharedMapProvider& provider : _providers) {
        if (std::dynamic_pointer_cast<const ElevationProvider>(provider)) {
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

namespace {
// Retina tiles share the SQLite cache with 1x tiles, so their hashes must not
// collide. The 10-digit provider-hash field has ample headroom above the small
// sequential mapIds, so @2x tiles reserve a high offset added to the mapId.
// providerTypeFromHash() strips it to recover the provider, keeping the mapping
// reversible for the importer / tileHashToType.
constexpr int kRetinaHashOffset = 1000000000;
}  // namespace

bool UrlFactory::useRetinaTiles()
{
    const QGuiApplication* const app = qobject_cast<QGuiApplication*>(QCoreApplication::instance());
    return app && (app->devicePixelRatio() > 1.0);
}

int UrlFactory::tilePixelScale()
{
    return useRetinaTiles() ? 2 : 1;
}

QString UrlFactory::providerTypeFromHash(int hash)
{
    _ensureInitialized();

    if (hash >= kRetinaHashOffset) {
        hash -= kRetinaHashOffset;
    }
    const auto it = _providersByMapId.constFind(hash);
    if (it != _providersByMapId.constEnd()) {
        return it.value()->getMapName();
    }

    qCWarning(QGCMapUrlEngineLog) << "provider not found from hash:" << hash;
    return QString("");
}

int UrlFactory::hashFromProviderType(QStringView type)
{
    const SharedMapProvider provider = getMapProviderFromProviderType(type);
    if (provider) {
        return provider->getMapId();
    }

    return -1;
}

QString UrlFactory::tileHashToType(QStringView tileHash)
{
    if (tileHash.size() < 10) {
        return QString();
    }
    const int providerHash = tileHash.mid(0, 10).toInt();
    return providerTypeFromHash(providerHash);
}

QString UrlFactory::getTileHash(QStringView type, int x, int y, int z)
{
    int hash = hashFromProviderType(type);
    // Keep @2x tiles in a disjoint hash space so they never collide with 1x tiles.
    if ((hash >= 0) && useRetinaTiles()) {
        hash += kRetinaHashOffset;
    }
    return QString::asprintf("%010d%08d%08d%03d", hash, x, y, z);
}
