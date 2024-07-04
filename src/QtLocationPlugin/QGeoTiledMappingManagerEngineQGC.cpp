#include "QGeoTiledMappingManagerEngineQGC.h"
#include "QGCApplication.h"
#include "QGCMapEngine.h"
#include "QGeoTileFetcherQGC.h"
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
    _setCache(parameters);

    m_prefetchStyle = QGeoTiledMap::PrefetchTwoNeighbourLayers;

    *error = QGeoServiceProvider::NoError;
    errorString->clear();

     QGeoTileFetcherQGC* const tileFetcher = new QGeoTileFetcherQGC(m_networkManager, this);
    // TODO: Allow useragent override again
    /*if (parameters.contains(QStringLiteral("useragent"))) {
        tileFetcher()->setUserAgent(parameters.value(QStringLiteral("useragent")).toString().toLatin1());
    }*/
    setTileFetcher(tileFetcher); /* Calls engineInitialized */
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

void QGeoTiledMappingManagerEngineQGC::_setCache(const QVariantMap &parameters)
{
    if (getQGCMapEngine()->wasCacheReset()) {
        qgcApp()->showAppMessage(tr("The Offline Map Cache database has been upgraded. "
                    "Your old map cache sets have been reset."));
    }

    QString cacheDir;
    if (parameters.contains(QStringLiteral("mapping.cache.directory"))) {
        cacheDir = parameters.value(QStringLiteral("mapping.cache.directory")).toString();
    } else {
        cacheDir = getQGCMapEngine()->getCachePath();
        if(!QFileInfo::exists(cacheDir)) {
            if(!QDir::root().mkpath(cacheDir)) {
                qWarning() << "Could not create mapping disk cache directory: " << cacheDir;
                cacheDir = QDir::homePath() + QStringLiteral("/.qgcmapscache/");
            }
        }
    }
    if(!QFileInfo::exists(cacheDir)) {
        if(!QDir::root().mkpath(cacheDir)) {
            qWarning() << "Could not create mapping disk cache directory: " << cacheDir;
            cacheDir.clear();
        }
    }
    //-- Memory Cache
    uint32_t memLimit = 0;
    if (parameters.contains(QStringLiteral("mapping.cache.memory.size"))) {
      bool ok = false;
      memLimit = parameters.value(QStringLiteral("mapping.cache.memory.size")).toString().toUInt(&ok);
      if (!ok) {
          memLimit = 0;
      }
    }
    if(!memLimit)
    {
        //-- Value saved in MB
        memLimit = QGCMapEngine::getMaxMemCache() * (1024 * 1024);
    }
    //-- It won't work with less than 1M of memory cache
    if(memLimit < 1024 * 1024) {
        memLimit = 1024 * 1024;
    }
    //-- On the other hand, Qt uses signed 32-bit integers. Limit to 1G to round it down (you don't need more than that).
    if(memLimit > 1024 * 1024 * 1024) {
        memLimit = 1024 * 1024 * 1024;
    }
    //-- Disable Qt's disk cache (sort of)
    QAbstractGeoTileCache *pTileCache = new QGeoFileTileCache(cacheDir);
    setTileCache(pTileCache);
    if(pTileCache) {
        //-- We're basically telling it to use 100k of disk for cache. It doesn't like
        //   values smaller than that and I could not find a way to make it NOT cache.
        //   We handle our own disk caching elsewhere.
        pTileCache->setMaxDiskUsage(1024 * 100);
        pTileCache->setMaxMemoryUsage(memLimit);
    }
}
