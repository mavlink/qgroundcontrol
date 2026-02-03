#include "Viewer3DTileQuery.h"

#include "QGCLoggingCategory.h"
#include "MapProvider.h"
#include "QGCMapUrlEngine.h"

#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QtNetwork/QNetworkAccessManager>

#include <cmath>

QGC_LOGGING_CATEGORY(Viewer3DTileQueryLog, "Viewer3d.Viewer3DTileQuery")

static constexpr int kMaxTileCounts = 200;
static constexpr int kMaxZoomLevel  = 23;

Viewer3DTileQuery::Viewer3DTileQuery(QObject *parent)
    : QObject{parent}
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

    if (!_networkManager) {
        _networkManager = new QNetworkAccessManager(this);
        _networkManager->setTransferTimeout(9000);
    }

    for (int x = tileMinIndex.x(); x <= tileMaxIndex.x(); x++) {
        for (int y = tileMinIndex.y(); y <= tileMaxIndex.y(); y++) {
            _mapToBeLoaded.tileList.append(_tileKey(_mapId, x, y, zoomLevel));

            auto *reply = new Viewer3DTileReply(zoomLevel, x, y, _mapId, _mapType, _networkManager, this);
            connect(reply, &Viewer3DTileReply::tileDone, this, &Viewer3DTileQuery::_tileDone);
            connect(reply, &Viewer3DTileReply::tileGiveUp, this, &Viewer3DTileQuery::_tileGiveUp);
            connect(reply, &Viewer3DTileReply::tileEmpty, this, &Viewer3DTileQuery::_tileEmpty);
        }
    }

    _totalTilesCount = _mapToBeLoaded.tileList.size();
    _downloadedTilesCount = 0;
    qCDebug(Viewer3DTileQueryLog) << "Requesting" << _totalTilesCount << "tiles at zoom" << zoomLevel
                                  << "x:[" << tileMinIndex.x() << ".." << tileMaxIndex.x() << "]"
                                  << "y:[" << tileMinIndex.y() << ".." << tileMaxIndex.y() << "]";
}

Viewer3DTileQuery::TileStatistics_t Viewer3DTileQuery::_findAndLoadMapTiles(int zoomLevel, const QGeoCoordinate &coordinateMin, const QGeoCoordinate &coordinateMax)
{
    const SharedMapProvider provider = UrlFactory::getMapProviderFromQtMapId(_mapId);
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

void Viewer3DTileQuery::adaptiveMapTilesLoader(const QString &mapType, int mapId, const QGeoCoordinate &coordinateMin, const QGeoCoordinate &coordinateMax)
{
    _mapId = mapId;
    _mapType = mapType;

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
    const SharedMapProvider provider = UrlFactory::getMapProviderFromQtMapId(_mapId);
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

void Viewer3DTileQuery::_cleanupReply(Viewer3DTileReply *reply)
{
    disconnect(reply, &Viewer3DTileReply::tileDone, this, &Viewer3DTileQuery::_tileDone);
    disconnect(reply, &Viewer3DTileReply::tileGiveUp, this, &Viewer3DTileQuery::_tileGiveUp);
    disconnect(reply, &Viewer3DTileReply::tileEmpty, this, &Viewer3DTileQuery::_tileEmpty);
    reply->deleteLater();
}

void Viewer3DTileQuery::_tileDone(Viewer3DTileReply::TileInfo_t tileData)
{
    auto *reply = qobject_cast<Viewer3DTileReply *>(QObject::sender());

    const QString key = _tileKey(tileData.mapId, tileData.x, tileData.y, tileData.zoomLevel);
    const qsizetype itemRemoved = _mapToBeLoaded.tileList.removeAll(key);

    if (itemRemoved > 0) {
        _mapToBeLoaded.currentTileIndex = QPoint(tileData.x, tileData.y);
        _mapToBeLoaded.currentTileData = tileData.data;
        _mapToBeLoaded.setMapTile();
        _downloadedTilesCount++;
        emit mapTileDownloaded(100.0f * (static_cast<float>(_downloadedTilesCount) / static_cast<float>(_totalTilesCount)));

        if (_mapToBeLoaded.tileList.isEmpty()) {
            qCDebug(Viewer3DTileQueryLog) << "All tiles downloaded";
            _downloadedTilesCount = _totalTilesCount;
            emit loadingMapCompleted();
        }
    }

    _cleanupReply(reply);
}

void Viewer3DTileQuery::_tileGiveUp(Viewer3DTileReply::TileInfo_t tileData)
{
    auto *reply = qobject_cast<Viewer3DTileReply *>(QObject::sender());
    _cleanupReply(reply);

    const QString key = _tileKey(tileData.mapId, tileData.x, tileData.y, tileData.zoomLevel);
    _mapToBeLoaded.tileList.removeAll(key);
    _downloadedTilesCount++;
    emit mapTileDownloaded(100.0f * (static_cast<float>(_downloadedTilesCount) / static_cast<float>(_totalTilesCount)));

    if (_mapToBeLoaded.tileList.isEmpty()) {
        _downloadedTilesCount = _totalTilesCount;
        emit loadingMapCompleted();
    }
}

void Viewer3DTileQuery::_tileEmpty(Viewer3DTileReply::TileInfo_t tileData)
{
    auto *reply = qobject_cast<Viewer3DTileReply *>(QObject::sender());
    _cleanupReply(reply);

    if (tileData.zoomLevel > 0 && tileData.zoomLevel == _zoomLevel) {
        _zoomLevel -= 1;
        emit textureGeometryReady(_findAndLoadMapTiles(_zoomLevel, _textureCoordinateMin, _textureCoordinateMax));
    }
}

QString Viewer3DTileQuery::_tileKey(int mapId, int x, int y, int zoomLevel)
{
    return QString::asprintf("%010d%08d%08d%03d", mapId, x, y, zoomLevel);
}
