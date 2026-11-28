#pragma once

#include <memory>

#include <QtCore/QPointer>
#include <QtLocation/private/qgeotilefetcher_p.h>

class QGCGeoFileTileCache;
class GeoNoTiles;
class GeoTileFailures;
class GeoTileRequests;
class QGCGeoTiledMappingManagerEngine;
class QNetworkInformation;

class QGCGeoTileFetcher : public QGeoTileFetcher
{
    Q_OBJECT

public:
    explicit QGCGeoTileFetcher(QGCGeoTiledMappingManagerEngine* parent = nullptr);
    ~QGCGeoTileFetcher();

    /// Whether the servers of the maps are asked for tiles with \a networkInformation, which is null on a system
    /// without a backend that reports reachability. Such a system, as one that cannot tell whether it reaches the
    /// internet, is taken to be online: the replies then ask a server that did not answer again after a delay, and
    /// do not wait for news of the network that would never come.
    static bool isOnline(const QNetworkInformation* networkInformation);

private:
    QGeoTiledMapReply* getTileImage(const QGeoTileSpec& spec) final;

    /// Shared with the replies, which are destroyed after these when the fetcher is destroyed before they finish
    const std::shared_ptr<GeoNoTiles> _noTiles;
    const std::shared_ptr<GeoTileFailures> _failures;
    const std::shared_ptr<GeoTileRequests> _requests;
    const QPointer<QGCGeoFileTileCache> _tileCache;  ///< That of the engine, which shows the tiles from above
};
