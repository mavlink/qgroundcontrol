#include "QGeoTileFetcherQGC.h"

#include <QtLocation/private/qgeotilespec_p.h>

#include "QGCLoggingCategory.h"
#include "QGeoTiledMapReplyQGC.h"
#include "QGeoTiledMappingManagerEngineQGC.h"

QGC_LOGGING_CATEGORY(QGeoTileFetcherQGCLog, "QtLocationPlugin.QGeoTileFetcherQGC")

QGeoTileFetcherQGC::QGeoTileFetcherQGC(QNetworkAccessManager* networkManager, QGeoTiledMappingManagerEngineQGC* parent)
    : QGeoTileFetcher(parent)
    , m_networkManager(networkManager)
{
    qCDebug(QGeoTileFetcherQGCLog) << this;
}

QGeoTileFetcherQGC::~QGeoTileFetcherQGC()
{
    qCDebug(QGeoTileFetcherQGCLog) << this;
}

QGeoTiledMapReply* QGeoTileFetcherQGC::getTileImage(const QGeoTileSpec& spec)
{
    // A spec without a map provider is an error of the reply, which looks the provider up
    QGeoTiledMapReplyQGC* const tileImage = new QGeoTiledMapReplyQGC(m_networkManager, spec);
    tileImage->init();

    return tileImage;
}
