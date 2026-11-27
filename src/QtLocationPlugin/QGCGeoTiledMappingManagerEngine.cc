#include "QGCGeoTiledMappingManagerEngine.h"

#include <algorithm>
#include <cmath>

#include <QtCore/QPointF>
#include <QtCore/QtMath>
#include <QtLocation/private/qgeocameracapabilities_p.h>
#include <QtLocation/private/qgeocameradata_p.h>
#include <QtLocation/private/qgeomaptype_p.h>
#include <QtLocation/private/qgeotiledmap_p.h>
#include <QtNetwork/QNetworkInformation>
#include <QtPositioning/QGeoCoordinate>

#include "MapEngine.h"
#include "MapProvider.h"
#include "MapProviderRegistry.h"
#include "QGCGeoFileTileCache.h"
#include "QGCGeoTileFetcher.h"
#include "QGCGeoTiledMap.h"
#include "QGCLoggingCategory.h"
#include "QGCNetworkHelper.h"
#include "QtLocationMapId.h"
#include "TileCache.h"
#include "WebMercator.h"

QGC_LOGGING_CATEGORY(QGCGeoTiledMappingManagerEngineLog, "QtLocationPlugin.QGCGeoTiledMappingManagerEngine")

namespace {

/// Where \a coordinate is on the map of the whole world, from 0 to 1 eastward and southward
QPointF mappingEngineWorldPosition(const QGeoCoordinate& coordinate)
{
    return QPointF(WebMercator::xOfLongitude(coordinate.longitude()), WebMercator::yOfLatitude(coordinate.latitude()));
}

}  // namespace

// The constructor casts MapProvider::MapStyle to QGeoMapType::MapStyle, so the values must match.
static_assert(static_cast<int>(MapProvider::NoMap) == static_cast<int>(QGeoMapType::NoMap));
static_assert(static_cast<int>(MapProvider::StreetMap) == static_cast<int>(QGeoMapType::StreetMap));
static_assert(static_cast<int>(MapProvider::SatelliteMapDay) == static_cast<int>(QGeoMapType::SatelliteMapDay));
static_assert(static_cast<int>(MapProvider::SatelliteMapNight) == static_cast<int>(QGeoMapType::SatelliteMapNight));
static_assert(static_cast<int>(MapProvider::TerrainMap) == static_cast<int>(QGeoMapType::TerrainMap));
static_assert(static_cast<int>(MapProvider::HybridMap) == static_cast<int>(QGeoMapType::HybridMap));
static_assert(static_cast<int>(MapProvider::TransitMap) == static_cast<int>(QGeoMapType::TransitMap));
static_assert(static_cast<int>(MapProvider::GrayStreetMap) == static_cast<int>(QGeoMapType::GrayStreetMap));
static_assert(static_cast<int>(MapProvider::PedestrianMap) == static_cast<int>(QGeoMapType::PedestrianMap));
static_assert(static_cast<int>(MapProvider::CarNavigationMap) == static_cast<int>(QGeoMapType::CarNavigationMap));
static_assert(static_cast<int>(MapProvider::CycleMap) == static_cast<int>(QGeoMapType::CycleMap));
static_assert(static_cast<int>(MapProvider::CustomMap) == static_cast<int>(QGeoMapType::CustomMap));

QGCGeoTiledMappingManagerEngine::QGCGeoTiledMappingManagerEngine(QGeoServiceProvider::Error* error,
                                                                 QString* errorString, QObject* parent)
    : QGeoTiledMappingManagerEngine(parent)
{
    qCDebug(QGCGeoTiledMappingManagerEngineLog) << this;

    QGeoCameraCapabilities cameraCaps{};
    cameraCaps.setTileSize(256);
    cameraCaps.setMinimumZoomLevel(2.0);
    cameraCaps.setMaximumZoomLevel(QGC_MAX_MAP_ZOOM);
    cameraCaps.setSupportsBearing(true);
    cameraCaps.setSupportsRolling(false);
    cameraCaps.setSupportsTilting(false);
    cameraCaps.setMinimumTilt(0.0);
    cameraCaps.setMaximumTilt(0.0);
    cameraCaps.setMinimumFieldOfView(45.0);
    cameraCaps.setMaximumFieldOfView(45.0);
    cameraCaps.setOverzoomEnabled(true);
    setCameraCapabilities(cameraCaps);

    setTileVersion(kTileVersion);
    // A tile of the source the user configures now has the spec of the tile of the previous source that the maps and
    // the tile cache hold. Under another version it is another tile, so every map requests its tiles again.
    (void) connect(MapEngine::instance(), &MapEngine::tileSourceChanged, this,
                   [this] { setTileVersion(tileVersion() + 1); });
    setTileSize(QSize(256, 256));

    QList<QGeoMapType> mapList;
    const QList<int>& providerIds = QtLocationMapId::mapTypeProviderIds();
    for (const int providerId : providerIds) {
        const SharedMapProvider provider = MapProviderRegistry::getMapProviderFromMapId(providerId);
        if (!provider) {
            continue;
        }
        const QGeoMapType map = QGeoMapType(static_cast<QGeoMapType::MapStyle>(provider->mapStyle()),
                                            provider->mapName(), provider->mapName(), false, provider->isNight(),
                                            QtLocationMapId::fromProviderId(providerId, providerIds),
                                            QByteArrayLiteral("QGroundControl"), cameraCapabilities());
        mapList.append(map);
    }
    setSupportedMapTypes(mapList);

    setCacheHint(QAbstractGeoTileCache::MemoryCache);
    setTileCache(new QGCGeoFileTileCache(this));

    TileCache::instance()->ensureInitialized();

    // Without a connection to the internet, a tile that is not cached only fails. What counts as one is what the
    // fetcher asks the servers with.
    const auto prefetchStyle = [](const QNetworkInformation* networkInformation) {
        return QGCGeoTileFetcher::isOnline(networkInformation) ? QGeoTiledMap::PrefetchTwoNeighbourLayers
                                                               : QGeoTiledMap::NoPrefetching;
    };
    // Null on a system without a backend that reports reachability
    const QNetworkInformation* const networkInformation = QGCNetworkHelper::networkInformation();
    setPrefetchStyle(prefetchStyle(networkInformation));
    if (networkInformation) {
        (void) connect(
            networkInformation, &QNetworkInformation::reachabilityChanged, this,
            [this, prefetchStyle, networkInformation] { setPrefetchStyle(prefetchStyle(networkInformation)); });
    }

    QGCGeoTileFetcher* tileFetcher = new QGCGeoTileFetcher(this);

    *error = QGeoServiceProvider::NoError;
    errorString->clear();
    setTileFetcher(tileFetcher);  // Calls engineInitialized()
}

QGCGeoTiledMappingManagerEngine::~QGCGeoTiledMappingManagerEngine()
{
    qCDebug(QGCGeoTiledMappingManagerEngineLog) << this;
}

QGeoMap* QGCGeoTiledMappingManagerEngine::createMap()
{
    QGCGeoTiledMap* map = new QGCGeoTiledMap(this, this);
    map->setPrefetchStyle(m_prefetchStyle);
    return map;
}

void QGCGeoTiledMappingManagerEngine::setPrefetchStyle(QGeoTiledMap::PrefetchStyle style)
{
    m_prefetchStyle = style;
    // The engine is the parent of the maps it creates
    const QList<QGeoTiledMap*> maps = findChildren<QGeoTiledMap*>(Qt::FindDirectChildrenOnly);
    for (QGeoTiledMap* const map : maps) {
        map->setPrefetchStyle(style);
    }
}

void QGCGeoTiledMappingManagerEngine::updateTileRequests(QGeoTiledMap* map, const QSet<QGeoTileSpec>& tilesAdded,
                                                         const QSet<QGeoTileSpec>& tilesRemoved)
{
    if (!map || (tilesAdded.size() < 2)) {
        QGeoTiledMappingManagerEngine::updateTileRequests(map, tilesAdded, tilesRemoved);
        return;
    }

    const QGeoCameraData& camera = map->cameraData();
    // As QGeoTiledMap shows the tiles of a whole zoom level for a zoom level within 0.01 below it
    const int viewZoom = static_cast<int>(std::floor(camera.zoomLevel() + 0.01));
    const QPointF centre = mappingEngineWorldPosition(camera.center());
    const auto distanceToCentre = [&centre](const QGeoTileSpec& spec) {
        const double tilesPerEdge = std::ldexp(1.0, spec.zoom());
        double dx = std::abs(((spec.x() + 0.5) / tilesPerEdge) - centre.x());
        dx = std::min(dx, 1.0 - dx);  // Also across the antimeridian
        const double dy = ((spec.y() + 0.5) / tilesPerEdge) - centre.y();
        return (dx * dx) + (dy * dy);
    };

    QList<QGeoTileSpec> viewTiles;
    QSet<QGeoTileSpec> otherTiles;
    for (const QGeoTileSpec& spec : tilesAdded) {
        if (spec.zoom() == viewZoom) {
            viewTiles.append(spec);
        } else {
            (void) otherTiles.insert(spec);
        }
    }
    std::sort(viewTiles.begin(), viewTiles.end(), [&distanceToCentre](const QGeoTileSpec& a, const QGeoTileSpec& b) {
        return distanceToCentre(a) < distanceToCentre(b);
    });

    // The fetcher keeps the order of the calls, not that of the tiles of one call
    QGeoTiledMappingManagerEngine::updateTileRequests(map, {}, tilesRemoved);
    for (const QGeoTileSpec& spec : std::as_const(viewTiles)) {
        QGeoTiledMappingManagerEngine::updateTileRequests(map, {spec}, {});
    }
    if (!otherTiles.isEmpty()) {
        QGeoTiledMappingManagerEngine::updateTileRequests(map, otherTiles, {});
    }
}
