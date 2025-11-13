#include "QGeoTiledMappingManagerEngineQGC.h"

#include <mutex>

#include <QtCore/QDir>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkDiskCache>
#include <QtNetwork/QNetworkInformation>
#include <QtNetwork/QNetworkProxy>
#include <QtLocation/private/qgeocameracapabilities_p.h>
#include <QtLocation/private/qgeomaptype_p.h>
#include <QtLocation/private/qgeotiledmap_p.h>
#include <QtLocation/private/qgeofiletilecache_p.h>

#include "MapProvider.h"
#include "QGCNetworkHelper.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "QGCMapEngine.h"
#include "QGCMapEngineManager.h"
#include "QGCMapUrlEngine.h"
#include "QGeoFileTileCacheQGC.h"
#include "QGeoTiledMapQGC.h"
#include "QGeoTileFetcherQGC.h"

QGC_LOGGING_CATEGORY(QGeoTiledMappingManagerEngineQGCLog, "QtLocationPlugin.QGeoTiledMappingManagerEngineQGC")

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
    cameraCaps.setTileSize(kTileSize);
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
    setTileSize(QSize(kTileSize, kTileSize));

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
        mapList.append(map);
    }
    setSupportedMapTypes(mapList);

    setCacheHint(QAbstractGeoTileCache::CacheArea::AllCaches);
    QGeoFileTileCacheQGC *fileTileCache = new QGeoFileTileCacheQGC(parameters, this);
    setTileCache(fileTileCache);

    // MapEngine must be init after fileTileCache
    static std::once_flag mapEngineInit;
    std::call_once(mapEngineInit, [fileTileCache]() {
        getQGCMapEngine()->init(fileTileCache->getDatabaseFilePath());
    });

    m_prefetchStyle = QGCNetworkHelper::isInternetAvailable() ? QGeoTiledMap::PrefetchTwoNeighbourLayers : QGeoTiledMap::NoPrefetching;

    QNetworkInformation *networkInfo = QNetworkInformation::instance();
    if (!networkInfo) {
        (void) QNetworkInformation::loadDefaultBackend();
        networkInfo = QNetworkInformation::instance();
    }
    if (networkInfo) {
        (void) connect(networkInfo, &QNetworkInformation::reachabilityChanged, this, [this](QNetworkInformation::Reachability newReachability) {
            const QGeoTiledMap::PrefetchStyle newStyle = (newReachability == QNetworkInformation::Reachability::Online)
                ? QGeoTiledMap::PrefetchTwoNeighbourLayers
                : QGeoTiledMap::NoPrefetching;
            if (newStyle == m_prefetchStyle) {
                return;
            }
            m_prefetchStyle = newStyle;
            _updatePrefetchStyles();
        });
    } else {
        qCWarning(QGeoTiledMappingManagerEngineQGCLog) << "QNetworkInformation backend not available";
    }

    if (!m_networkManager) {
        qCCritical(QGeoTiledMappingManagerEngineQGCLog) << "Network manager is null";
        if (error) {
            *error = QGeoServiceProvider::LoaderError;
        }
        if (errorString) {
            *errorString = QStringLiteral("Network manager is null");
        }
        return;
    }

    QGeoTileFetcherQGC *tileFetcher = new QGeoTileFetcherQGC(m_networkManager, parameters, this);

    if (error) {
        *error = QGeoServiceProvider::NoError;
    }
    if (errorString) {
        errorString->clear();
    }
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
    m_activeMaps.append(QPointer<QGeoTiledMapQGC>(map));
    (void) connect(map, &QObject::destroyed, this, [this, map]() {
        for (int i = m_activeMaps.count() - 1; i >= 0; --i) {
            if (!m_activeMaps[i] || m_activeMaps[i].data() == map) {
                m_activeMaps.removeAt(i);
            }
        }
    });
    return map;
}

void QGeoTiledMappingManagerEngineQGC::_updatePrefetchStyles()
{
    for (int i = m_activeMaps.count() - 1; i >= 0; --i) {
        QPointer<QGeoTiledMapQGC> &map = m_activeMaps[i];
        if (!map) {
            m_activeMaps.removeAt(i);
            continue;
        }
        map->setPrefetchStyle(m_prefetchStyle);
    }
}
