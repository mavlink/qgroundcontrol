#include "QGeoTiledMappingManagerEngineQGC.h"
#include "QGCApplication.h"
#include "QGCMapEngine.h"
#include "QGeoTileFetcherQGC.h"
#include "QGeoFileTileCacheQGC.h"
#include "QGeoTiledMapQGC.h"
#include "QGCMapUrlEngine.h"
#include "MapProvider.h"
#include "QGCMapEngineManager.h"
#include <QGCLoggingCategory.h>

#include <QtCore/QDir>
#include <QtNetwork/QNetworkAccessManager>
#include <QtLocation/private/qgeocameracapabilities_p.h>
#include <QtLocation/private/qgeomaptype_p.h>
#include <QtLocation/private/qgeotiledmap_p.h>
#include <QtLocation/private/qgeofiletilecache_p.h>

#define TILE_VERSION 1

QGC_LOGGING_CATEGORY(QGeoTiledMappingManagerEngineQGCLog, "qgc.qtlocationplugin.qgeotiledmappingmanagerengineqgc")

QGeoTiledMappingManagerEngineQGC::QGeoTiledMappingManagerEngineQGC(const QVariantMap &parameters, QGeoServiceProvider::Error *error, QString *errorString, QObject *parent)
    : QGeoTiledMappingManagerEngine(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
    // qCDebug(QGeoTiledMappingManagerEngineQGCLog) << Q_FUNC_INFO << this;

    // TODO: Better way to get current language without qgcApp()?
    setLocale(qgcApp()->getCurrentLanguage());

    getQGCMapEngine()->init();

    QGeoCameraCapabilities cameraCaps;
    cameraCaps.setTileSize(256);
    cameraCaps.setMinimumZoomLevel(2.0);
    cameraCaps.setMaximumZoomLevel(MAX_MAP_ZOOM);
    cameraCaps.setSupportsBearing(true);
    cameraCaps.setSupportsRolling(false);
    cameraCaps.setSupportsTilting(false);
    cameraCaps.setMinimumTilt(0.0);
    cameraCaps.setMaximumTilt(0.0);
    cameraCaps.setMinimumFieldOfView(45.0);
    cameraCaps.setMaximumFieldOfView(45.0);
    cameraCaps.setOverzoomEnabled(true);
    setCameraCapabilities(cameraCaps);

    setTileVersion(TILE_VERSION);
    setTileSize(QSize(256, 256));

    QList<QGeoMapType> mapList;
    const QList<SharedMapProvider> providers = UrlFactory::getProviders();
    for (const SharedMapProvider &provider : providers) {
        const QGeoMapType map = QGeoMapType(
            provider->getMapStyle(),
            provider->getMapName(),
            provider->getMapName(),
            false,
            false,
            provider->getMapId(),
            QByteArrayLiteral("QGroundControl"),
            cameraCapabilities()
        );
        (void) mapList.append(map);
    }
    setSupportedMapTypes(mapList);

    setCacheHint(QAbstractGeoTileCache::CacheArea::AllCaches);
    QGeoFileTileCacheQGC* const fileTileCache = new QGeoFileTileCacheQGC(parameters);
    setTileCache(fileTileCache);

    m_prefetchStyle = QGeoTiledMap::PrefetchTwoNeighbourLayers;

    *error = QGeoServiceProvider::NoError;
    errorString->clear();

    QGeoTileFetcherQGC* const tileFetcher = new QGeoTileFetcherQGC(m_networkManager, parameters, this);
    setTileFetcher(tileFetcher); // Calls engineInitialized
}

QGeoTiledMappingManagerEngineQGC::~QGeoTiledMappingManagerEngineQGC()
{
    // qCDebug(QGeoTiledMappingManagerEngineQGCLog) << Q_FUNC_INFO << this;
}

QGeoMap* QGeoTiledMappingManagerEngineQGC::createMap()
{
    QGeoTiledMapQGC* const map = new QGeoTiledMapQGC(this, this);
    map->setPrefetchStyle(m_prefetchStyle);
    return map;
}
