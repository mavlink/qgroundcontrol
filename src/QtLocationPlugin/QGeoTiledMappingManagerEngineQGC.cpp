#include "QGeoTiledMappingManagerEngineQGC.h"

#include <QtCore/QDir>
#include <QtLocation/private/qgeocameracapabilities_p.h>
#include <QtLocation/private/qgeofiletilecache_p.h>
#include <QtLocation/private/qgeomaptype_p.h>
#include <QtLocation/private/qgeotiledmap_p.h>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkInformation>
#include <QtNetwork/QNetworkProxy>
#include <mutex>

#include "MapProvider.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "QGCMapEngine.h"
#include "QGCMapEngineManager.h"
#include "QGCMapUrlEngine.h"
#include "QGCNetworkHelper.h"
#include "QGCTileCache.h"
#include "QGeoFileTileCacheQGC.h"
#include "QGeoTileFetcherQGC.h"
#include "QGeoTiledMapQGC.h"

QGC_LOGGING_CATEGORY(QGeoTiledMappingManagerEngineQGCLog, "QtLocationPlugin.QGeoTiledMappingManagerEngineQGC")

QGeoTiledMappingManagerEngineQGC::QGeoTiledMappingManagerEngineQGC(const QVariantMap& parameters,
                                                                   QGeoServiceProvider::Error* error,
                                                                   QString* errorString,
                                                                   QNetworkAccessManager* networkManager,
                                                                   QObject* parent)
    : QGeoTiledMappingManagerEngine(parent), m_networkManager(networkManager)
{
    qCDebug(QGeoTiledMappingManagerEngineQGCLog) << this;

    setLocale(qgcApp()->getCurrentLanguage());
    (void) connect(qgcApp(), &QGCApplication::languageChanged, this,
                   [this](const QLocale& locale) { setLocale(locale); });

    // R4: request 512px (@2x) tiles on HiDPI displays so the map isn't a blurry
    // upscale of 256px tiles. UrlFactory::useRetinaTiles() reads the display DPR
    // (false without a QGuiApplication, e.g. unit tests).
    const int tilePx = UrlFactory::useRetinaTiles() ? 512 : 256;

    QGeoCameraCapabilities cameraCaps{};
    cameraCaps.setTileSize(tilePx);
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
    setTileSize(QSize(tilePx, tilePx));

    QList<QGeoMapType> mapList;
    const QList<SharedMapProvider> providers = UrlFactory::getProviders();
    for (const SharedMapProvider& provider : providers) {
        const QGeoMapType map = QGeoMapType(
            static_cast<QGeoMapType::MapStyle>(provider->getMapStyle()), provider->getMapName(), provider->getMapName(),
            false, false, provider->getMapId(), QByteArrayLiteral("QGroundControl"), cameraCapabilities());
        mapList.append(map);
    }
    setSupportedMapTypes(mapList);

    setCacheHint(QAbstractGeoTileCache::CacheArea::AllCaches);
    QGeoFileTileCacheQGC* fileTileCache = new QGeoFileTileCacheQGC(parameters, this);
    setTileCache(fileTileCache);

    // MapEngine must be init after fileTileCache
    static std::once_flag mapEngineInit;
    std::call_once(mapEngineInit, []() { getQGCMapEngine()->init(QGCTileCache::getDatabaseFilePath()); });

    m_prefetchStyle = QGCNetworkHelper::isInternetAvailable() ? QGeoTiledMap::PrefetchTwoNeighbourLayers
                                                              : QGeoTiledMap::NoPrefetching;
    (void) connect(QNetworkInformation::instance(), &QNetworkInformation::reachabilityChanged, this,
                   [this](QNetworkInformation::Reachability newReachability) {
                       const QGeoTiledMap::PrefetchStyle newStyle =
                           (newReachability == QNetworkInformation::Reachability::Online)
                               ? QGeoTiledMap::PrefetchTwoNeighbourLayers
                               : QGeoTiledMap::NoPrefetching;
                       if (newStyle == m_prefetchStyle) {
                           return;
                       }
                       m_prefetchStyle = newStyle;
                       _updatePrefetchStyles();
                   });

    if (!m_networkManager) {
        qCCritical(QGeoTiledMappingManagerEngineQGCLog) << "No network manager provided";
        if (error) {
            *error = QGeoServiceProvider::ConnectionError;
        }
        if (errorString) {
            *errorString = QStringLiteral("No network manager available");
        }
        return;
    }

    QGeoTileFetcherQGC* tileFetcher = new QGeoTileFetcherQGC(m_networkManager, parameters, this);

    if (error) {
        *error = QGeoServiceProvider::NoError;
    }
    if (errorString) {
        errorString->clear();
    }
    setTileFetcher(tileFetcher);  // Calls engineInitialized()
}

QGeoTiledMappingManagerEngineQGC::~QGeoTiledMappingManagerEngineQGC()
{
    qCDebug(QGeoTiledMappingManagerEngineQGCLog) << this;
}

QGeoMap* QGeoTiledMappingManagerEngineQGC::createMap()
{
    QGeoTiledMapQGC* map = new QGeoTiledMapQGC(this, this);
    map->setPrefetchStyle(m_prefetchStyle);

    m_activeMaps.append(QPointer<QGeoTiledMapQGC>(map));
    // QPointer auto-nulls before destroyed() fires, so prune by null on any death.
    (void) connect(map, &QObject::destroyed, this, [this]() {
        for (int i = m_activeMaps.count() - 1; i >= 0; --i) {
            if (!m_activeMaps[i]) {
                m_activeMaps.removeAt(i);
            }
        }
    });

    return map;
}

void QGeoTiledMappingManagerEngineQGC::_updatePrefetchStyles()
{
    for (int i = m_activeMaps.count() - 1; i >= 0; --i) {
        const QPointer<QGeoTiledMapQGC>& mapPtr = m_activeMaps[i];
        if (!mapPtr) {
            m_activeMaps.removeAt(i);
            continue;
        }
        mapPtr->setPrefetchStyle(m_prefetchStyle);
    }
}
