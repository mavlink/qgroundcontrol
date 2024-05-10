#include "QGeoTileFetcherQGC.h"
#include "QGeoTiledMappingManagerEngineQGC.h"
#include "QGCMapEngine.h"
#include "QGeoMapReplyQGC.h"
#include "QGCMapUrlEngine.h"
#include <DeviceInfo.h>
#include <QGCLoggingCategory.h>

#include <QtNetwork/QNetworkRequest>
#include <QtLocation/private/qgeotilespec_p.h>
#include <QtLocation/private/qgeotiledmappingmanagerengine_p.h>

QGC_LOGGING_CATEGORY(QGeoTileFetcherQGCLog, "qgc.qtlocationplugin.qgeotilefetcherqgc")

QGeoTileFetcherQGC::QGeoTileFetcherQGC(QNetworkAccessManager* networkManager, QGeoTiledMappingManagerEngineQGC *parent)
    : QGeoTileFetcher(dynamic_cast<QGeoMappingManagerEngine *>(parent))
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
    QNetworkRequest request = getQGCMapEngine()->urlFactory()->getTileURL(spec.mapId(), spec.x(), spec.y(), spec.zoom(), m_networkManager);
    request.setOriginatingObject(this);
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
    request.setAttribute(QNetworkRequest::BackgroundRequestAttribute, true);
    request.setAttribute(QNetworkRequest::CacheSaveControlAttribute, true);
    request.setAttribute(QNetworkRequest::DoNotBufferUploadDataAttribute, false);
    request.setPriority(QNetworkRequest::NormalPriority);
    request.setTransferTimeout(10000);

    if (!request.url().isEmpty()) {
        return new QGeoTiledMapReplyQGC(m_networkManager, request, spec);
    }

    return nullptr;
}

bool QGeoTileFetcherQGC::initialized() const
{
    return !!m_networkManager;
}

bool QGeoTileFetcherQGC::fetchingEnabled() const
{
    return QGCDeviceInfo::isInternetAvailable();
}

void QGeoTileFetcherQGC::timerEvent(QTimerEvent *event)
{
    QGeoTileFetcher::timerEvent(event);
}

void QGeoTileFetcherQGC::handleReply(QGeoTiledMapReply *reply, const QGeoTileSpec &spec)
{
    if (!initialized()) {
        reply->deleteLater();
        return;
    }

    if (reply->error() == QGeoTiledMapReply::NoError) {
        emit tileFinished(spec, reply->mapImageData(), reply->mapImageFormat());
    } else {
        emit tileError(spec, reply->errorString());
    }

    reply->deleteLater();
}
