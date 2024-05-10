#include "QGeoTiledMappingManagerEngineQGC.h"
#include "QGCApplication.h"
#include "QGCMapEngine.h"
#include "QGeoTileFetcherQGC.h"
#include "QGeoFileTileCacheQGC.h"
#include "QGeoTiledMapQGC.h"
#include "QGCMapUrlEngine.h"
#include "MapProvider.h"
#include <QGCLoggingCategory.h>

#include <QtCore/QDir>
#include <QtNetwork/QNetworkAccessManager>
#include <QtLocation/private/qgeocameracapabilities_p.h>
#include <QtLocation/private/qgeomaptype_p.h>
#include <QtLocation/private/qgeotiledmap_p.h>
#include <QtLocation/private/qgeofiletilecache_p.h>

QGC_LOGGING_CATEGORY(QGeoTiledMappingManagerEngineQGCLog, "qgc.qtlocationplugin.qgeotiledmappingmanagerengineqgc")

QGeoTiledMappingManagerEngineQGC::QGeoTiledMappingManagerEngineQGC(const QVariantMap &parameters, QGeoServiceProvider::Error *error, QString *errorString)
    : QGeoTiledMappingManagerEngine()
    , m_networkManager(new QNetworkAccessManager(this))
{
    setLocale(qgcApp()->getCurrentLanguage());

    getQGCMapEngine()->init();
    if (parameters.contains(QStringLiteral("useragent"))) {
        getQGCMapEngine()->setUserAgent(parameters.value(QStringLiteral("useragent")).toString().toLatin1());
    }

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

    setTileSize(QSize(256, 256));

    _createSupportedMapTypesList();

    setTileCache(new QGeoFileTileCacheQGC(m_networkManager, parameters));

    m_prefetchStyle = QGeoTiledMap::PrefetchTwoNeighbourLayers;

    *error = QGeoServiceProvider::NoError;
    errorString->clear();

    setTileFetcher(new QGeoTileFetcherQGC(m_networkManager, this)); /* Calls engineInitialized */

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

void QGeoTiledMappingManagerEngineQGC::_createSupportedMapTypesList()
{
    QList<QGeoMapType> mapList;

    int qtMapId = 1; // Qt Map Ids must start at 1 and are sequential.
    MapProvider* provider = getQGCMapEngine()->urlFactory()->getMapProviderFromQtMapId(qtMapId);

    while (provider) {
        const QString providerType = getQGCMapEngine()->urlFactory()->getProviderTypeFromQtMapId(qtMapId);
        mapList.append(QGeoMapType(provider->getMapStyle(), providerType, providerType, false, false, qtMapId++, QByteArray("QGroundControl"), cameraCapabilities()));
        provider = getQGCMapEngine()->urlFactory()->getMapProviderFromQtMapId(qtMapId);
    }

    setSupportedMapTypes(mapList);
}
