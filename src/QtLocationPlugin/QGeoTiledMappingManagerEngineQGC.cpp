#include "QGeoTiledMappingManagerEngineQGC.h"

#include <QtCore/QDir>
#include <QtLocation/private/qgeocameracapabilities_p.h>
#include <QtLocation/private/qgeofiletilecache_p.h>
#include <QtLocation/private/qgeomaptype_p.h>
#include <QtLocation/private/qgeotiledmap_p.h>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkDiskCache>
#include <QtNetwork/QNetworkInformation>
#include <QtNetwork/QNetworkProxy>

#include "MapProvider.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "QGCMapEngine.h"
#include "QGCMapEngineManager.h"
#include "QGCMapUrlEngine.h"
#include "QGCNetworkHelper.h"
#include "QGeoFileTileCacheQGC.h"
#include "QGeoMapIdQGC.h"
#include "QGeoTileFetcherQGC.h"
#include "QGeoTiledMapQGC.h"

QGC_LOGGING_CATEGORY(QGeoTiledMappingManagerEngineQGCLog, "QtLocationPlugin.QGeoTiledMappingManagerEngineQGC")

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

QGeoTiledMappingManagerEngineQGC::QGeoTiledMappingManagerEngineQGC(const QVariantMap &parameters, QGeoServiceProvider::Error *error, QString *errorString, QNetworkAccessManager *networkManager, QObject *parent)
    : QGeoTiledMappingManagerEngine(parent)
    , m_networkManager(networkManager)
{
    qCDebug(QGeoTiledMappingManagerEngineQGCLog) << this;

    setLocale(qgcApp()->getCurrentLanguage());
    (void) connect(qgcApp(), &QGCApplication::languageChanged, this, [this](const QLocale &locale) {
        setLocale(locale);
    });

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
    (void) connect(getQGCMapEngine(), &QGCMapEngine::tileSourceChanged, this,
                   [this] { setTileVersion(tileVersion() + 1); });
    setTileSize(QSize(256, 256));

    QList<QGeoMapType> mapList;
    const QList<int>& providerIds = QGeoMapIdQGC::mapTypeProviderIds();
    for (const int providerId : providerIds) {
        const SharedMapProvider provider = UrlFactory::getMapProviderFromMapId(providerId);
        if (!provider) {
            continue;
        }
        const QGeoMapType map =
            QGeoMapType(static_cast<QGeoMapType::MapStyle>(provider->getMapStyle()), provider->getMapName(),
                        provider->getMapName(), false, false, QGeoMapIdQGC::fromProviderId(providerId, providerIds),
                        QByteArrayLiteral("QGroundControl"), cameraCapabilities());
        mapList.append(map);
    }
    setSupportedMapTypes(mapList);

    setCacheHint(QAbstractGeoTileCache::CacheArea::AllCaches);
    setTileCache(new QGeoFileTileCacheQGC(parameters, this));

    getQGCMapEngine()->ensureInitialized();

    m_prefetchStyle = QGCNetworkHelper::isInternetAvailable() ? QGeoTiledMap::PrefetchTwoNeighbourLayers : QGeoTiledMap::NoPrefetching;
    (void) connect(QNetworkInformation::instance(), &QNetworkInformation::reachabilityChanged, this, [this](QNetworkInformation::Reachability newReachability) {
        if (newReachability == QNetworkInformation::Reachability::Online) {
            m_prefetchStyle = QGeoTiledMap::PrefetchTwoNeighbourLayers;
        } else {
            m_prefetchStyle = QGeoTiledMap::NoPrefetching;
        }
    });

    QGeoTileFetcherQGC *tileFetcher = new QGeoTileFetcherQGC(m_networkManager, parameters, this);

    *error = QGeoServiceProvider::NoError;
    errorString->clear();
    setTileFetcher(tileFetcher); // Calls engineInitialized()
}

QGeoTiledMappingManagerEngineQGC::~QGeoTiledMappingManagerEngineQGC()
{
    qCDebug(QGeoTiledMappingManagerEngineQGCLog) << this;
}

QGeoMap *QGeoTiledMappingManagerEngineQGC::createMap()
{
    QGeoTiledMapQGC *map = new QGeoTiledMapQGC(this, this);
    map->setPrefetchStyle(m_prefetchStyle);
    return map;
}
