#include "QGeoTileFetcherQGC.h"

#include <QtLocation/private/qgeotiledmappingmanagerengine_p.h>
#include <QtLocation/private/qgeotilespec_p.h>

#include "QGCLoggingCategory.h"
#include "QGCMapUrlEngine.h"
#include "QGeoMapIdQGC.h"
#include "QGeoMapReplyQGC.h"
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
    const SharedMapProvider provider = UrlFactory::getMapProviderFromMapId(QGeoMapIdQGC::toProviderId(spec.mapId()));
    if (!provider) {
        return nullptr;
    }

    QGeoTiledMapReplyQGC* tileImage = new QGeoTiledMapReplyQGC(m_networkManager, spec);
    tileImage->init();

    return tileImage;
}
