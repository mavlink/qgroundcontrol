#include "QGeoTileFetcherQGC.h"

#include <QtLocation/private/qgeotiledmappingmanagerengine_p.h>
#include <QtLocation/private/qgeotilespec_p.h>

#include "MapProvider.h"
#include "QGCLoggingCategory.h"
#include "QGCMapUrlEngine.h"
#include "QGeoMapReplyQGC.h"
#include "QGeoTiledMappingManagerEngineQGC.h"

QGC_LOGGING_CATEGORY(QGeoTileFetcherQGCLog, "QtLocationPlugin.QGeoTileFetcherQGC")

QGeoTileFetcherQGC::QGeoTileFetcherQGC(QNetworkAccessManager* networkManager, const QVariantMap& parameters,
                                       QGeoTiledMappingManagerEngineQGC* parent)
    : QGeoTileFetcher(parent), m_networkManager(networkManager)
{
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

QGeoTiledMapReply* QGeoTileFetcherQGC::getTileImage(const QGeoTileSpec& spec)
{
    const SharedMapProvider provider = UrlFactory::getMapProviderFromQtMapId(spec.mapId());
    if (!provider) {
        return nullptr;
    }

    /*if (spec.zoom() > provider->maximumZoomLevel() || spec.zoom() < provider->minimumZoomLevel()) {
        return nullptr;
    }*/

    if (provider->getTileURL(spec.x(), spec.y(), spec.zoom()).isEmpty()) {
        return nullptr;
    }

    QGeoTiledMapReplyQGC* tileImage = new QGeoTiledMapReplyQGC(m_networkManager, spec);
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

void QGeoTileFetcherQGC::timerEvent(QTimerEvent* event)
{
    QGeoTileFetcher::timerEvent(event);
}

void QGeoTileFetcherQGC::handleReply(QGeoTiledMapReply* reply, const QGeoTileSpec& spec)
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
