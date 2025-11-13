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
    if (!networkManager) {
        qCCritical(QGeoTileFetcherQGCLog) << "Network manager is null";
    }

    qCDebug(QGeoTileFetcherQGCLog) << this;
}

QGeoTileFetcherQGC::~QGeoTileFetcherQGC()
{
    qCDebug(QGeoTileFetcherQGCLog) << this;
}

QGeoTiledMapReply* QGeoTileFetcherQGC::getTileImage(const QGeoTileSpec &spec)
{
    const SharedMapProvider provider = UrlFactory::getMapProviderFromQtMapId(spec.mapId());
    if (!provider) {
        const QString error = tr("Unknown map provider (%1)").arg(spec.mapId());
        qCWarning(QGeoTileFetcherQGCLog) << error;
        emit tileError(spec, error);
        return nullptr;
    }

    // TODO: Re-enable zoom level check once all providers have correct min/max zoom levels set
    /*if (spec.zoom() > provider->maximumZoomLevel() || spec.zoom() < provider->minimumZoomLevel()) {
        return nullptr;
    }*/

    const QNetworkRequest request = getNetworkRequest(spec.mapId(), spec.x(), spec.y(), spec.zoom());
    if (!request.url().isValid() || request.url().isEmpty()) {
        const QString error = tr("Map provider returned an invalid URL");
        qCWarning(QGeoTileFetcherQGCLog) << error << "mapId:" << spec.mapId()
                                         << "url:" << request.url();
        emit tileError(spec, error);
        return nullptr;
    }

    QGeoTiledMapReplyQGC *tileImage = new QGeoTiledMapReplyQGC(m_networkManager, request, spec);
    if (!tileImage->init()) {
        tileImage->deleteLater();
        const QString error = tr("Failed to start tile request");
        qCWarning(QGeoTileFetcherQGCLog) << error << "mapId:" << spec.mapId()
                                         << "x:" << spec.x() << "y:" << spec.y()
                                         << "zoom:" << spec.zoom();
        emit tileError(spec, error);
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
        const QString error = tr("Invalid tile reply");
        qCWarning(QGeoTileFetcherQGCLog) << error << "mapId:" << spec.mapId()
                                         << "x:" << spec.x() << "y:" << spec.y()
                                         << "zoom:" << spec.zoom();
        emit tileError(spec, error);
        return;
    }

    reply->deleteLater();

    if (!initialized()) {
        const QString error = tr("Tile fetcher is not initialized");
        qCWarning(QGeoTileFetcherQGCLog) << error;
        emit tileError(spec, error);
        return;
    }

    if (reply->error() == QGeoTiledMapReply::NoError) {
        const QByteArray bytes = reply->mapImageData();
        const QString format = reply->mapImageFormat();
        emit tileFinished(spec, bytes, format);
    } else {
        const QString error = reply->errorString().isEmpty()
            ? tr("Unknown tile fetch error")
            : reply->errorString();
        emit tileError(spec, error);
    }
}

QNetworkRequest QGeoTileFetcherQGC::getNetworkRequest(int mapId, int x, int y, int zoom)
{
    const SharedMapProvider mapProvider = UrlFactory::getMapProviderFromQtMapId(mapId);
    if (!mapProvider) {
        return QNetworkRequest();
    }

    QNetworkRequest request;
    request.setUrl(mapProvider->getTileURL(x, y, zoom));
    request.setPriority(QNetworkRequest::NormalPriority);
    request.setTransferTimeout(kNetworkRequestTimeoutMs);

    // Headers
    request.setRawHeader(QByteArrayLiteral("Accept"), QByteArrayLiteral("*/*"));
    request.setHeader(QNetworkRequest::UserAgentHeader, s_userAgent);
    const QByteArray referrer = mapProvider->getReferrer().toUtf8();
    if (!referrer.isEmpty()) {
        request.setRawHeader(QByteArrayLiteral("Referer"), referrer);
    }
    const QByteArray token = mapProvider->getToken();
    if (!token.isEmpty()) {
        request.setRawHeader(QByteArrayLiteral("User-Token"), token);
    }
    request.setRawHeader(QByteArrayLiteral("Connection"), QByteArrayLiteral("keep-alive"));
    request.setRawHeader(QByteArrayLiteral("Accept-Encoding"), QByteArrayLiteral("gzip, deflate, br"));

    // Attributes
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
    request.setAttribute(QNetworkRequest::BackgroundRequestAttribute, true);
    request.setAttribute(QNetworkRequest::CacheSaveControlAttribute, true);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setAttribute(QNetworkRequest::Http2AllowedAttribute, true);
    request.setAttribute(QNetworkRequest::DoNotBufferUploadDataAttribute, false);
    request.setAttribute(QNetworkRequest::AutoDeleteReplyOnFinishAttribute, true);

    return request;
}
