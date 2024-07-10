#include "QGeoTileFetcherQGC.h"
#include "QGeoTiledMappingManagerEngineQGC.h"
#include "QGeoMapReplyQGC.h"
#include "QGCMapUrlEngine.h"
#include "MapProvider.h"
#include <QGCLoggingCategory.h>

#include <QtNetwork/QNetworkRequest>
// #include <QtNetwork/QNetworkDiskCache>
#include <QtLocation/private/qgeotilespec_p.h>
#include <QtLocation/private/qgeotiledmappingmanagerengine_p.h>

QGC_LOGGING_CATEGORY(QGeoTileFetcherQGCLog, "qgc.qtlocationplugin.qgeotilefetcherqgc")

QGeoTileFetcherQGC::QGeoTileFetcherQGC(QNetworkAccessManager *networkManager, const QVariantMap &parameters, QGeoTiledMappingManagerEngineQGC *parent)
    : QGeoTileFetcher(parent)
    , m_networkManager(networkManager)
    // , m_diskCache(new QNetworkDiskCache(this))
{
    // qCDebug(QGeoTileFetcherQGCLog) << Q_FUNC_INFO << this;

    // TODO: Allow useragent override again
    /*if (parameters.contains(QStringLiteral("useragent"))) {
        setUserAgent(parameters.value(QStringLiteral("useragent")).toString().toLatin1());
    }*/

    /*if (networkManager) {
        m_diskCache->setCacheDirectory(directory() + "/Downloads");
        m_diskCache->setMaximumCacheSize(static_cast<qint64>(_getDefaultMaxDiskCache()));
        networkManager->setCache(m_diskCache);
    }*/
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
