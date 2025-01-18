/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGeoTiledMappingManagerEngineQGC.h"
#include "QGCApplication.h"
#include "QGCMapEngine.h"
#include "QGeoTileFetcherQGC.h"
#include "QGeoFileTileCacheQGC.h"
#include "QGeoTiledMapQGC.h"
#include "QGCMapUrlEngine.h"
#include "MapProvider.h"
#include "QGCMapEngineManager.h"
#include "QGCLoggingCategory.h"

#include <mutex>

#include <QtCore/QDir>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkDiskCache>
#include <QtNetwork/QNetworkProxy>
#include <QtLocation/private/qgeocameracapabilities_p.h>
#include <QtLocation/private/qgeomaptype_p.h>
#include <QtLocation/private/qgeotiledmap_p.h>
#include <QtLocation/private/qgeofiletilecache_p.h>

QGC_LOGGING_CATEGORY(QGeoTiledMappingManagerEngineQGCLog, "qgc.qtlocationplugin.qgeotiledmappingmanagerengineqgc")

QGeoTiledMappingManagerEngineQGC::QGeoTiledMappingManagerEngineQGC(const QVariantMap &parameters, QGeoServiceProvider::Error *error, QString *errorString, QNetworkAccessManager *networkManager, QObject *parent)
    : QGeoTiledMappingManagerEngine(parent)
    , m_networkManager(networkManager)
{
    // qCDebug(QGeoTiledMappingManagerEngineQGCLog) << Q_FUNC_INFO << this;

    // TODO: Better way to get current language without qgcApp()?
    setLocale(qgcApp()->getCurrentLanguage());

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

    setTileVersion(kTileVersion);
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

    // MapEngine must be init after fileTileCache
    static std::once_flag mapEngineInit;
    std::call_once(mapEngineInit, [fileTileCache]() {
        getQGCMapEngine()->init(fileTileCache->getDatabaseFilePath());
    });

    m_prefetchStyle = QGeoTiledMap::PrefetchTwoNeighbourLayers;

    if (!m_networkManager) {
        m_networkManager = new QNetworkAccessManager(this);
        #if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
            QNetworkProxy proxy = m_networkManager->proxy();
            proxy.setType(QNetworkProxy::DefaultProxy);
            m_networkManager->setProxy(proxy);
        #endif
        m_networkManager->setTransferTimeout(10000);
        // m_networkManager->setAutoDeleteReplies(true);
        QNetworkDiskCache *const diskCache = new QNetworkDiskCache(this);
        diskCache->setCacheDirectory(fileTileCache->getCachePath() + "/Downloads");
        const qint64 maxCacheSize = (50 * pow(1024, 2)); // fileTileCache->getMaxDiskCache()
        diskCache->setMaximumCacheSize(maxCacheSize);
        m_networkManager->setCache(diskCache);
    }

    QGeoTileFetcherQGC* const tileFetcher = new QGeoTileFetcherQGC(m_networkManager, parameters, this);

    *error = QGeoServiceProvider::NoError;
    errorString->clear();
    setTileFetcher(tileFetcher); // Calls engineInitialized()
}

QGeoTiledMappingManagerEngineQGC::~QGeoTiledMappingManagerEngineQGC()
{
    // qCDebug(QGeoTiledMappingManagerEngineQGCLog) << Q_FUNC_INFO << this;
}

QGeoMap *QGeoTiledMappingManagerEngineQGC::createMap()
{
    QGeoTiledMapQGC* const map = new QGeoTiledMapQGC(this, this);
    map->setPrefetchStyle(m_prefetchStyle);
    return map;
}
