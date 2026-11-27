#include "Viewer3DTileQuery.h"

#include <cmath>
#include <memory>
#include <utility>

#include <QtGui/QPainter>
#include <QtGui/QPixmap>

#include "MapProvider.h"
#include "MapProviderRegistry.h"
#include "QGCLoggingCategory.h"
#include "TileReply.h"

QGC_LOGGING_CATEGORY(Viewer3DTileQueryLog, "Viewer3d.Viewer3DTileQuery")

static constexpr int kMaxTileCounts = 200;
static constexpr int kMaxZoomLevel  = 23;

Viewer3DTileQuery::Viewer3DTileQuery(QObject* parent, RuntimeScheduler* scheduler)
    : QObject{parent}
    , _scheduler(RuntimeScheduler::orDefault(scheduler, this))
{
}

void Viewer3DTileQuery::MapTileContainer_t::init()
{
    mapWidth = (tileMaxIndex.x() - tileMinIndex.x() + 1) * tileSize;
    mapHeight = (tileMaxIndex.y() - tileMinIndex.y() + 1) * tileSize;
    mapTextureImage = QImage(mapWidth, mapHeight, QImage::Format_RGBA32FPx4);
    mapTextureImage.fill(Qt::gray);
}

void Viewer3DTileQuery::MapTileContainer_t::setMapTile()
{
    QPixmap tmpPixmap;
    tmpPixmap.loadFromData(currentTileData);
    QImage tmpImage = tmpPixmap.toImage().convertToFormat(QImage::Format_RGBA32FPx4);

    QPainter painter(&mapTextureImage);
    int idxX = (currentTileIndex.x() - tileMinIndex.x()) * tileSize;
    int idxY = (currentTileIndex.y() - tileMinIndex.y()) * tileSize;
    painter.drawImage(idxX, idxY, tmpImage);
}

QByteArray Viewer3DTileQuery::MapTileContainer_t::mapData() const
{
    return QByteArray(reinterpret_cast<const char *>(mapTextureImage.constBits()), mapTextureImage.sizeInBytes());
}

void Viewer3DTileQuery::MapTileContainer_t::clear()
{
    tileList.clear();
}

void Viewer3DTileQuery::_loadMapTiles(int zoomLevel, QPoint tileMinIndex, QPoint tileMaxIndex)
{
    _mapToBeLoaded.clear();
    _mapToBeLoaded.zoomLevel = zoomLevel;
    _mapToBeLoaded.tileMinIndex = tileMinIndex;
    _mapToBeLoaded.tileMaxIndex = tileMaxIndex;
    _mapToBeLoaded.init();

    // The tiles of an earlier load are not shown: they are not asked for any more
    _abortTileReplies();
    for (int x = tileMinIndex.x(); x <= tileMaxIndex.x(); x++) {
        for (int y = tileMinIndex.y(); y <= tileMaxIndex.y(); y++) {
            _mapToBeLoaded.tileList.append(_tileKey(_mapId, x, y, zoomLevel));

            Viewer3DTileInfo tile;
            tile.x = x;
            tile.y = y;
            tile.zoomLevel = zoomLevel;
            tile.mapId = _mapId;
            _requestTile(tile, 0);
        }
    }

    _totalTilesCount = _mapToBeLoaded.tileList.size();
    qCDebug(Viewer3DTileQueryLog) << "Requesting" << _totalTilesCount << "tiles at zoom" << zoomLevel
                                  << "x:[" << tileMinIndex.x() << ".." << tileMaxIndex.x() << "]"
                                  << "y:[" << tileMinIndex.y() << ".." << tileMaxIndex.y() << "]";
}

Viewer3DTileQuery::TileStatistics_t Viewer3DTileQuery::_findAndLoadMapTiles(int zoomLevel, const QGeoCoordinate &coordinateMin, const QGeoCoordinate &coordinateMax)
{
    const SharedMapProvider provider = MapProviderRegistry::getMapProviderFromMapId(_mapId);
    if (!provider) {
        return {};
    }

    const double lat1 = coordinateMin.latitude();
    const double lon1 = coordinateMin.longitude();
    const double lat2 = coordinateMax.latitude();
    const double lon2 = coordinateMax.longitude();

    const int minTileX = provider->long2tileX(std::fmin(lon1, lon2), zoomLevel);
    const int maxTileX = provider->long2tileX(std::fmax(lon1, lon2), zoomLevel);
    const int minTileY = provider->lat2tileY(std::fmax(lat1, lat2), zoomLevel);
    const int maxTileY = provider->lat2tileY(std::fmin(lat1, lat2), zoomLevel);

    const QPoint minTile(minTileX, minTileY);
    const QPoint maxTile(maxTileX, maxTileY);

    const QGeoCoordinate nwCoord(provider->tileY2lat(minTileY, zoomLevel),
                                 provider->tileX2long(minTileX, zoomLevel), 0);
    const QGeoCoordinate seCoord(provider->tileY2lat(maxTileY + 1, zoomLevel),
                                 provider->tileX2long(maxTileX + 1, zoomLevel), 0);

    const QGeoCoordinate finalMin = QGeoCoordinate(seCoord.latitude(), nwCoord.longitude(), 0);
    const QGeoCoordinate finalMax = QGeoCoordinate(nwCoord.latitude(), seCoord.longitude(), 0);

    _loadMapTiles(zoomLevel, minTile, maxTile);

    TileStatistics_t output;
    output.coordinateMin = finalMin;
    output.coordinateMax = finalMax;
    output.tileCounts = QSize(maxTile.x() - minTile.x() + 1, maxTile.y() - minTile.y() + 1);
    output.zoomLevel = zoomLevel;

    return output;
}

void Viewer3DTileQuery::adaptiveMapTilesLoader(int mapId, const QGeoCoordinate& coordinateMin,
                                               const QGeoCoordinate& coordinateMax)
{
    _mapId = mapId;

    for (_zoomLevel = kMaxZoomLevel; _zoomLevel > 0; _zoomLevel--) {
        if (maxTileCount(_zoomLevel, coordinateMin, coordinateMax) < kMaxTileCounts) {
            break;
        }
    }

    _textureCoordinateMin = coordinateMin;
    _textureCoordinateMax = coordinateMax;
    emit textureGeometryReady(_findAndLoadMapTiles(_zoomLevel, coordinateMin, coordinateMax));
}

int Viewer3DTileQuery::maxTileCount(int zoomLevel, const QGeoCoordinate &coordinateMin, const QGeoCoordinate &coordinateMax)
{
    const SharedMapProvider provider = MapProviderRegistry::getMapProviderFromMapId(_mapId);
    if (!provider) {
        return 0;
    }

    const double minLon = std::fmin(coordinateMin.longitude(), coordinateMax.longitude());
    const double maxLon = std::fmax(coordinateMin.longitude(), coordinateMax.longitude());
    const double minLat = std::fmin(coordinateMin.latitude(), coordinateMax.latitude());
    const double maxLat = std::fmax(coordinateMin.latitude(), coordinateMax.latitude());

    const int minTileX = provider->long2tileX(minLon, zoomLevel);
    const int maxTileX = provider->long2tileX(maxLon, zoomLevel);
    const int minTileY = provider->lat2tileY(maxLat, zoomLevel);
    const int maxTileY = provider->lat2tileY(minLat, zoomLevel);

    return (maxTileX - minTileX + 1) * (maxTileY - minTileY + 1);
}

void Viewer3DTileQuery::_requestTile(const Viewer3DTileInfo& tile, int retries)
{
    auto* const reply = new TileReply(tile.mapId, tile.x, tile.y, tile.zoomLevel, this);
    (void) connect(reply, &TileReply::finished, this,
                   [this, reply, tile, retries] { _tileReplyFinished(reply, tile, retries); });
    _tileReplies.append(reply);
    reply->start();
}

void Viewer3DTileQuery::_tileReplyFinished(TileReply* reply, Viewer3DTileInfo tileData, int retries)
{
    (void) _tileReplies.removeOne(reply);
    reply->deleteLater();

    switch (reply->error()) {
        case TileReply::Error::NoError:
            tileData.data = reply->data();
            _tileDone(tileData);
            break;
        case TileReply::Error::NoTileError:
            _tileEmpty(tileData);
            break;
        default:
            if (!_retryLater(*reply, tileData, retries)) {
                qCWarning(Viewer3DTileQueryLog) << "Request error for tile x:" << tileData.x << "y:" << tileData.y
                                                << "zoom:" << tileData.zoomLevel << reply->errorString();
                _tileGiveUp(tileData);
            }
            break;
    }
}

bool Viewer3DTileQuery::_retryLater(const TileReply& reply, const Viewer3DTileInfo& tile, int retries)
{
    const TileReply::NetworkFailure failure = reply.networkFailure();
    const bool canClearUp = (failure == TileReply::NetworkFailure::Unreachable) ||
                            (failure == TileReply::NetworkFailure::Unavailable) ||
                            (failure == TileReply::NetworkFailure::RateLimited);
    if (!canClearUp || (retries >= static_cast<int>(kRetryDelays.size()))) {
        return false;
    }

    if (failure == TileReply::NetworkFailure::RateLimited) {
        // The answer paused the server: the request waits for the pause to end
        qCDebug(Viewer3DTileQueryLog) << reply.errorString() << "- asking again once the pause of the server ends";
        _requestTile(tile, retries + 1);
        return true;
    }

    const std::chrono::milliseconds delay = kRetryDelays[static_cast<size_t>(retries)];
    // The task tells its own id, by which a load that follows cancels it
    const auto task = std::make_shared<RuntimeScheduler::TaskId>(0);
    *task = _scheduler->schedule(this, delay, [this, tile, retries, task] {
        (void) _retryTasks.remove(*task);
        _requestTile(tile, retries + 1);
    });
    if (*task == 0) {
        return false;
    }
    (void) _retryTasks.insert(*task);
    qCDebug(Viewer3DTileQueryLog) << reply.errorString() << "- asking again in" << delay;
    return true;
}

void Viewer3DTileQuery::_abortTileReplies()
{
    for (const RuntimeScheduler::TaskId task : std::as_const(_retryTasks)) {
        _scheduler->cancel(task);
    }
    _retryTasks.clear();

    for (TileReply* const reply : std::as_const(_tileReplies)) {
        reply->abort();
        reply->deleteLater();
    }
    _tileReplies.clear();
}

void Viewer3DTileQuery::_tileDone(Viewer3DTileInfo tileData)
{
    const QString key = _tileKey(tileData.mapId, tileData.x, tileData.y, tileData.zoomLevel);
    const qsizetype itemRemoved = _mapToBeLoaded.tileList.removeAll(key);

    if (itemRemoved > 0) {
        _mapToBeLoaded.currentTileIndex = QPoint(tileData.x, tileData.y);
        _mapToBeLoaded.currentTileData = tileData.data;
        _mapToBeLoaded.setMapTile();

        if (_mapToBeLoaded.tileList.isEmpty()) {
            qCDebug(Viewer3DTileQueryLog) << "All tiles downloaded";
            emit loadingMapCompleted();
        }
    }
}

void Viewer3DTileQuery::_tileGiveUp(Viewer3DTileInfo tileData)
{
    const QString key = _tileKey(tileData.mapId, tileData.x, tileData.y, tileData.zoomLevel);
    _mapToBeLoaded.tileList.removeAll(key);

    if (_mapToBeLoaded.tileList.isEmpty()) {
        emit loadingMapCompleted();
    }
}

void Viewer3DTileQuery::_tileEmpty(Viewer3DTileInfo tileData)
{
    if ((tileData.zoomLevel == 0) && (_zoomLevel == 0)) {
        // No coarser zoom is left to load: the map is done without the tile
        _tileGiveUp(tileData);
        return;
    }

    if (tileData.zoomLevel > 0 && tileData.zoomLevel == _zoomLevel) {
        _zoomLevel -= 1;
        emit textureGeometryReady(_findAndLoadMapTiles(_zoomLevel, _textureCoordinateMin, _textureCoordinateMax));
    }
}

QString Viewer3DTileQuery::_tileKey(int mapId, int x, int y, int zoomLevel)
{
    return QString::asprintf("%010d%08d%08d%03d", mapId, x, y, zoomLevel);
}
