#include "QGeoTileFetcherQGC.h"
#include "QGeoTiledMappingManagerEngineQGC.h"
#include "QGCMapEngine.h"
#include "QGeoMapReplyQGC.h"
#include "QGCMapUrlEngine.h"
#include "MapProvider.h"
#include <DeviceInfo.h>
#include <QGCLoggingCategory.h>

#include <QtNetwork/QNetworkRequest>
#include <QtLocation/private/qgeotilespec_p.h>
#include <QtLocation/private/qgeotiledmappingmanagerengine_p.h>

QGC_LOGGING_CATEGORY(QGeoTileFetcherQGCLog, "qgc.qtlocationplugin.qgeotilefetcherqgc")

QGeoTileFetcherQGC::QGeoTileFetcherQGC(QNetworkAccessManager* networkManager, QGeoTiledMappingManagerEngineQGC *parent)
    : QGeoTileFetcher(parent)
    , m_networkManager(networkManager)
{
    qCDebug(QGeoTileFetcherQGCLog) << Q_FUNC_INFO << this;
}

QGeoTileFetcherQGC::~QGeoTileFetcherQGC()
{
    qCDebug(QGeoTileFetcherQGCLog) << Q_FUNC_INFO << this;
}

QGeoTiledMapReply* QGeoTileFetcherQGC::getTileImage(const QGeoTileSpec &spec)
{
    const QList<SharedMapProvider> providers = UrlFactory::getProviders();
    int id = spec.mapId();
    if ((id < 1) || (id > providers.size())) {
        qCWarning(QGeoTileFetcherQGCLog) << QString("Unknown map id %1").arg(id);
        if (providers.isEmpty()) {
            return nullptr;
        } else {
            id = 1;
        }
    }

    /*const MapProvider* const provider = getProviderFromQtMapId(id);
    if (spec.zoom() > provider->maximumZoomLevel() || spec.zoom() < provider->minimumZoomLevel()) {
        return nullptr;
    }*/

    return new QGeoTiledMapReplyQGC(m_networkManager, spec, this);
}

bool QGeoTileFetcherQGC::initialized() const
{
    return !!m_networkManager;
}

bool QGeoTileFetcherQGC::fetchingEnabled() const
{
    return initialized(); // QGCDeviceInfo::isInternetAvailable();
}

void QGeoTileFetcherQGC::timerEvent(QTimerEvent *event)
{
    QGeoTileFetcher::timerEvent(event);
}

void QGeoTileFetcherQGC::handleReply(QGeoTiledMapReply *reply, const QGeoTileSpec &spec)
{
    if (!reply) {
        return;
    }

    reply->deleteLater();

    if (!initialized()) {
        return;
    }

    if (reply->error() == QGeoTiledMapReply::NoError) {
        emit tileFinished(spec, reply->mapImageData(), reply->mapImageFormat());
    } else {
        emit tileError(spec, reply->errorString());
    }
}

QNetworkRequest QGeoTileFetcherQGC::getNetworkRequest(int mapId, int x, int y, int zoom)
{
    const SharedMapProvider mapProvider = UrlFactory::getProviderFromQtMapId(mapId);

    QNetworkRequest request;
    request.setUrl(mapProvider->getTileURL(x, y, zoom));
    request.setRawHeader(QByteArrayLiteral("Accept"), QByteArrayLiteral("*/*"));
    request.setRawHeader(QByteArrayLiteral("Referrer"), mapProvider->getReferrer().toUtf8());
    request.setHeader(QNetworkRequest::UserAgentHeader, s_userAgent);
    request.setRawHeader(QByteArrayLiteral("User-Token"), mapProvider->getToken());
    // request.setOriginatingObject(this);
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
    request.setAttribute(QNetworkRequest::BackgroundRequestAttribute, true);
    request.setAttribute(QNetworkRequest::CacheSaveControlAttribute, true);
    request.setAttribute(QNetworkRequest::DoNotBufferUploadDataAttribute, false);
    request.setPriority(QNetworkRequest::NormalPriority);
    request.setTransferTimeout(10000);
    return request;
}
