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

static QObject* qgcMapEngineManagerSingletonFactory(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)

    QGCMapEngineManager* qgcMapEngineManager = new QGCMapEngineManager();
    return qgcMapEngineManager;
}

QGeoTiledMappingManagerEngineQGC::QGeoTiledMappingManagerEngineQGC(const QVariantMap &parameters, QGeoServiceProvider::Error *error, QString *errorString)
    : QGeoTiledMappingManagerEngine()
    , m_networkManager(new QNetworkAccessManager(this))
{
    qmlRegisterSingletonType<QGCMapEngineManager>("QGroundControl", 1, 0, "MapEngineManager", qgcMapEngineManagerSingletonFactory);

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
    for (const SharedMapProvider provider : providers) {
        const QGeoMapType map = QGeoMapType(
            provider->getMapStyle(),
            provider->getMapName(),
            provider->getMapName(),
            false,
            false,
            provider->getMapId(),
            QByteArrayLiteral("QGroundControl"),
            provider->getCameraCapabilities()
        );
        mapList.append(map);
    }
    setSupportedMapTypes(mapList);

    setCacheHint(QAbstractGeoTileCache::CacheArea::AllCaches);
    QGeoFileTileCacheQGC* const fileTileCache = new QGeoFileTileCacheQGC(m_networkManager, parameters);
    setTileCache(fileTileCache);

    m_prefetchStyle = QGeoTiledMap::PrefetchTwoNeighbourLayers;

    *error = QGeoServiceProvider::NoError;
    errorString->clear();

    QGeoTileFetcherQGC* const tileFetcher = new QGeoTileFetcherQGC(m_networkManager, this);
    // TODO: Allow useragent override again
    /*if (parameters.contains(QStringLiteral("useragent"))) {
        tileFetcher()->setUserAgent(parameters.value(QStringLiteral("useragent")).toString().toLatin1());
    }*/
    setTileFetcher(tileFetcher); /* Calls engineInitialized */

    qCDebug(QGeoTiledMappingManagerEngineQGCLog) << Q_FUNC_INFO << this;
}

QGeoTiledMappingManagerEngineQGC::~QGeoTiledMappingManagerEngineQGC()
{
    qCDebug(QGeoTiledMappingManagerEngineQGCLog) << Q_FUNC_INFO << this;
}

QGeoMap* QGeoTiledMappingManagerEngineQGC::createMap()
{
    QGeoTiledMapQGC* map = new QGeoTiledMapQGC(this, this);
    map->setPrefetchStyle(m_prefetchStyle);
    return map;
}
