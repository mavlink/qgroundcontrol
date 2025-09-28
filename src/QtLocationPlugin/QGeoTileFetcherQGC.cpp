/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGeoTileFetcherQGC.h"

#include <QtLocation/private/qgeotiledmappingmanagerengine_p.h>
#include <QtLocation/private/qgeotilespec_p.h>
#include <QtNetwork/QNetworkRequest>

#include "MapProvider.h"
#include "QGCLoggingCategory.h"
#include "QGCMapUrlEngine.h"
#include "QGeoMapReplyQGC.h"
#include "QGeoTiledMappingManagerEngineQGC.h"

QGC_LOGGING_CATEGORY(QGeoTileFetcherQGCLog, "QtLocationPlugin.QGeoTileFetcherQGC")

QGeoTileFetcherQGC::QGeoTileFetcherQGC(QNetworkAccessManager *networkManager, const QVariantMap &parameters, QGeoTiledMappingManagerEngineQGC *parent)
    : QGeoTileFetcher(parent)
    , m_networkManager(networkManager)
{
    Q_ASSERT(networkManager);

    qCDebug(QGeoTileFetcherQGCLog) << this;

    // TODO: Allow useragent override again
    /*if (parameters.contains(QStringLiteral("useragent"))) {
        setUserAgent(parameters.value(QStringLiteral("useragent")).toString().toLatin1());
    }*/
}

QGeoTileFetcherQGC::~QGeoTileFetcherQGC()
{
    qCDebug(QGeoTileFetcherQGCLog) << this;
}

QGeoTiledMapReply* QGeoTileFetcherQGC::getTileImage(const QGeoTileSpec &spec)
{
    const SharedMapProvider provider = UrlFactory::getMapProviderFromQtMapId(spec.mapId());
    if (!provider) {
        return nullptr;
    }

    /*if (spec.zoom() > provider->maximumZoomLevel() || spec.zoom() < provider->minimumZoomLevel()) {
        return nullptr;
    }*/

    const QNetworkRequest request = getNetworkRequest(spec.mapId(), spec.x(), spec.y(), spec.zoom());
    if (request.url().isEmpty()) {
        return nullptr;
    }

    QGeoTiledMapReplyQGC *tileImage = new QGeoTiledMapReplyQGC(m_networkManager, request, spec);
    if (!tileImage->init()) {
        tileImage->deleteLater();
        return nullptr;
    }

    return tileImage;
}

bool QGeoTileFetcherQGC::initialized() const
{
    return (m_networkManager != nullptr);
}

bool QGeoTileFetcherQGC::fetchingEnabled() const
{
    return initialized();
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
    const SharedMapProvider mapProvider = UrlFactory::getMapProviderFromQtMapId(mapId);

    QNetworkRequest request;
    request.setUrl(mapProvider->getTileURL(x, y, zoom));
    request.setPriority(QNetworkRequest::NormalPriority);
    request.setTransferTimeout(10000);
    // request.setOriginatingObject(this);

    // Headers
    request.setRawHeader(QByteArrayLiteral("Accept"), QByteArrayLiteral("*/*"));
    request.setHeader(QNetworkRequest::UserAgentHeader, s_userAgent);
    const QByteArray referrer = mapProvider->getReferrer().toUtf8();
    if (!referrer.isEmpty()) {
        request.setRawHeader(QByteArrayLiteral("Referrer"), referrer);
    }
    const QByteArray token = mapProvider->getToken();
    if (!token.isEmpty()) {
        request.setRawHeader(QByteArrayLiteral("User-Token"), token);
    }
    request.setRawHeader(QByteArrayLiteral("Connection"), QByteArrayLiteral("keep-alive"));
    // request.setRawHeader(QByteArrayLiteral("Accept-Encoding"), QByteArrayLiteral("gzip, deflate, br"));

    // Attributes
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
    request.setAttribute(QNetworkRequest::BackgroundRequestAttribute, true);
    request.setAttribute(QNetworkRequest::CacheSaveControlAttribute, true);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setAttribute(QNetworkRequest::Http2AllowedAttribute, true);
    request.setAttribute(QNetworkRequest::DoNotBufferUploadDataAttribute, false);
    // request.setAttribute(QNetworkRequest::AutoDeleteReplyOnFinishAttribute, true);

    return request;
}
