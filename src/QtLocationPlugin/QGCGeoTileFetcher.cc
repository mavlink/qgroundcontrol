#include "QGCGeoTileFetcher.h"

#include <QtNetwork/QNetworkInformation>

#include "QGCGeoFileTileCache.h"
#include "QGCGeoTiledMapReply.h"
#include "QGCGeoTiledMappingManagerEngine.h"
#include "QGCLoggingCategory.h"
#include "QGCNetworkHelper.h"

QGC_LOGGING_CATEGORY(QGCGeoTileFetcherLog, "QtLocationPlugin.QGCGeoTileFetcher")

QGCGeoTileFetcher::QGCGeoTileFetcher(QGCGeoTiledMappingManagerEngine* parent)
    : QGeoTileFetcher(parent)
    , _noTiles(std::make_shared<GeoNoTiles>())
    , _failures(std::make_shared<GeoTileFailures>())
    , _requests(std::make_shared<GeoTileRequests>())
    , _tileCache(parent ? qobject_cast<QGCGeoFileTileCache*>(parent->tileCache()) : nullptr)
{
    qCDebug(QGCGeoTileFetcherLog) << this;

    if (parent) {
        // The tiles of another version are other tiles: those of another source
        (void) connect(parent, &QGeoTiledMappingManagerEngine::tileVersionChanged, this, [this] {
            _noTiles->clear();
            _failures->clear();
            _requests->clear();
        });
    }

    // Null on a system without a backend that reports reachability
    if (const QNetworkInformation* const networkInformation = QGCNetworkHelper::networkInformation()) {
        _failures->setOnline(isOnline(networkInformation));
        (void) connect(networkInformation, &QNetworkInformation::reachabilityChanged, this,
                       [this, networkInformation] { _failures->setOnline(isOnline(networkInformation)); });
    }
}

bool QGCGeoTileFetcher::isOnline(const QNetworkInformation* networkInformation)
{
    if (!networkInformation) {
        return true;
    }
    const QNetworkInformation::Reachability reachability = networkInformation->reachability();
    return (reachability == QNetworkInformation::Reachability::Online) ||
           (reachability == QNetworkInformation::Reachability::Unknown);
}

QGCGeoTileFetcher::~QGCGeoTileFetcher()
{
    qCDebug(QGCGeoTileFetcherLog) << this;
}

QGeoTiledMapReply* QGCGeoTileFetcher::getTileImage(const QGeoTileSpec& spec)
{
    // A spec without a map provider fails in the reply, which looks the provider up. A child of the fetcher, as a reply
    // that waits to ask again can be unfinished when the fetcher is destroyed.
    QGCGeoTiledMapReply* const tileImage =
        new QGCGeoTiledMapReply(spec, _noTiles, _failures, _requests, _tileCache, this);
    tileImage->init();

    return tileImage;
}
