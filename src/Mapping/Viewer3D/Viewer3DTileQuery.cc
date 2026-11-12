#include "Viewer3DTileQuery.h"

#include <cmath>

#include <QtCore/QTimer>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>

#include "MapProvider.h"
#include "QGCLoggingCategory.h"
#include "QGCMapUrlEngine.h"
#include "QGCTileReply.h"

QGC_LOGGING_CATEGORY(Viewer3DTileQueryLog, "Viewer3d.Viewer3DTileQuery")

static constexpr int kMaxTileCounts = 200;
static constexpr int kMaxZoomLevel = 23;

Viewer3DTileQuery::Viewer3DTileQuery(QObject* parent)
    : QObject{parent}
    , _answerTimer(new QTimer(this))
{
    _answerTimer->setInterval(kAnswerTimeout);
    connect(_answerTimer, &QTimer::timeout, this, &Viewer3DTileQuery::_retryUnansweredTiles);
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
    return QByteArray(reinterpret_cast<const char*>(mapTextureImage.constBits()), mapTextureImage.sizeInBytes());
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

    // The load that this one replaces has no use for the tiles it still waits for
    _abortRequests();
    _retryCount = 0;

    for (int x = tileMinIndex.x(); x <= tileMaxIndex.x(); x++) {
        for (int y = tileMinIndex.y(); y <= tileMaxIndex.y(); y++) {
            _mapToBeLoaded.tileList.append(_tileKey(_mapId, x, y, zoomLevel));
            _requestTile({.data = {}, .x = x, .y = y, .zoomLevel = zoomLevel, .mapId = _mapId});
        }
    }
    _answerTimer->start();

    _totalTilesCount = _mapToBeLoaded.tileList.size();
    qCDebug(Viewer3DTileQueryLog) << "Requesting" << _totalTilesCount << "tiles at zoom" << zoomLevel << "x:["
                                  << tileMinIndex.x() << ".." << tileMaxIndex.x() << "]"
                                  << "y:[" << tileMinIndex.y() << ".." << tileMaxIndex.y() << "]";
}

Viewer3DTileQuery::TileStatistics_t Viewer3DTileQuery::_findAndLoadMapTiles(int zoomLevel,
                                                                            const QGeoCoordinate& coordinateMin,
                                                                            const QGeoCoordinate& coordinateMax)
{
    const SharedMapProvider provider = UrlFactory::getMapProviderFromMapId(_mapId);
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

    const QGeoCoordinate nwCoord(provider->tileY2lat(minTileY, zoomLevel), provider->tileX2long(minTileX, zoomLevel),
                                 0);
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

int Viewer3DTileQuery::maxTileCount(int zoomLevel, const QGeoCoordinate& coordinateMin,
                                    const QGeoCoordinate& coordinateMax)
{
    const SharedMapProvider provider = UrlFactory::getMapProviderFromMapId(_mapId);
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

void Viewer3DTileQuery::_requestTile(const Viewer3DTileInfo& tile)
{
    QGCTileReply* const reply = new QGCTileReply(_networkManager, tile.mapId, tile.x, tile.y, tile.zoomLevel, this);
    connect(reply, &QGCTileReply::finished, this, [this, reply] { _tileReplyFinished(reply); });
    _requests.insert(reply, tile);
    reply->start();
}

void Viewer3DTileQuery::_abortRequests()
{
    _answerTimer->stop();
    for (auto request = _requests.cbegin(); request != _requests.cend(); ++request) {
        request.key()->abort();  // an aborted request does not finish
        request.key()->deleteLater();
    }
    _requests.clear();
}

void Viewer3DTileQuery::_tileReplyFinished(QGCTileReply* reply)
{
    reply->deleteLater();

    const auto request = _requests.constFind(reply);
    if (request == _requests.cend()) {
        return;
    }
    Viewer3DTileInfo tile = request.value();
    _requests.erase(request);
    if (_requests.isEmpty()) {
        _answerTimer->stop();
    }

    switch (reply->error()) {
        case QGCTileReply::Error::NoError:
            tile.data = reply->data();
            _tileDone(tile);
            break;
        case QGCTileReply::Error::NoTileError:
            _tileEmpty(tile);
            break;
        default:
            qCWarning(Viewer3DTileQueryLog) << "Request error for tile x:" << tile.x << "y:" << tile.y
                                            << "zoom:" << tile.zoomLevel << reply->errorString();
            _tileGiveUp(tile);
            break;
    }
}

void Viewer3DTileQuery::_retryUnansweredTiles()
{
    const QList<Viewer3DTileInfo> unanswered = _requests.values();
    qCDebug(Viewer3DTileQueryLog) << "No answer for" << unanswered.count() << "tiles, retry:" << _retryCount << "/"
                                  << kMaxRetries;
    _abortRequests();

    if (_retryCount >= kMaxRetries) {
        for (const Viewer3DTileInfo& tile : unanswered) {
            qCWarning(Viewer3DTileQueryLog)
                << "Request error for tile x:" << tile.x << "y:" << tile.y << "zoom:" << tile.zoomLevel << "timed out";
            _tileGiveUp(tile);
        }
        return;
    }

    _retryCount++;
    for (const Viewer3DTileInfo& tile : unanswered) {
        _requestTile(tile);
    }
    _answerTimer->start();
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
