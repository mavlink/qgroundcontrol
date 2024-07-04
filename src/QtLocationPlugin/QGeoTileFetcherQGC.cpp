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

QGeoTileFetcherQGC::QGeoTileFetcherQGC(QNetworkAccessManager *networkManager, QGeoTiledMappingManagerEngineQGC *parent)
    : QGeoTileFetcher(parent)
    , m_networkManager(networkManager)
{
    // qCDebug(QGeoTileFetcherQGCLog) << Q_FUNC_INFO << this;
}

QGeoTileFetcherQGC::~QGeoTileFetcherQGC()
{
    // qCDebug(QGeoTileFetcherQGCLog) << Q_FUNC_INFO << this;
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

    const QNetworkRequest request = provider->getTileURL(spec.x(), spec.y(), spec.zoom());
    if (request.url().isEmpty()) {
        return nullptr;
    }

    return new QGeoTiledMapReplyQGC(m_networkManager, request, spec);
}

bool QGeoTileFetcherQGC::initialized() const
{
    return (m_networkManager != nullptr);
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
