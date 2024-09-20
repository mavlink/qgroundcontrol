/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "Viewer3DTileQuery.h"

#define PI                  acos(-1.0f)
#define DEG_TO_RAD          PI/180.0f
#define RAD_TO_DEG          180.0f/PI
#define MAX_TILE_COUNTS     200
#define MAX_ZOOM_LEVEL      23


enum RequestStat{
    STARTED,
    IN_PROGRESS,
    FINISHED,
    ERROR,
};

MapTileQuery::MapTileQuery(QObject *parent)
    : QObject{parent}
{
}

void MapTileQuery::loadMapTiles(int zoomLevel, QPoint tileMinIndex, QPoint tileMaxIndex)
{
    _mapTilesLoadStat = RequestStat::STARTED;
    _mapToBeLoaded.clear();
    _mapToBeLoaded.zoomLevel = zoomLevel;
    _mapToBeLoaded.tileMinIndex = tileMinIndex;
    _mapToBeLoaded.tileMaxIndex = tileMaxIndex;
    _mapToBeLoaded.init();

    for (int x = tileMinIndex.x(); x <= tileMaxIndex.x(); x++) {
        for (int y = tileMinIndex.y(); y <= tileMaxIndex.y(); y++) {
            QString tileKey = getTileKey(_mapId, x, y, zoomLevel);
            _mapToBeLoaded.tileList.append(tileKey);
            Viewer3DTileReply* _reply = new Viewer3DTileReply(zoomLevel, x, y, _mapId, this);
            connect(_reply, &Viewer3DTileReply::tileDone, this, &MapTileQuery::tileDone);
            connect(_reply, &Viewer3DTileReply::tileGiveUp, this, &MapTileQuery::tileGiveUp);
            connect(_reply, &Viewer3DTileReply::tileEmpty, this, &MapTileQuery::tileEmpty);
        }
    }
    totalTilesCount = _mapToBeLoaded.tileList.size();
    downloadedTilesCount = 0;
    qDebug() << totalTilesCount << "Tiles to be downloaded!!";
}

MapTileQuery::TileStatistics_t MapTileQuery::findAndLoadMapTiles(int zoomLevel, QGeoCoordinate coordinate_1, QGeoCoordinate coordinate_2)
{
    float lat_1 = coordinate_1.latitude(); float lon_1 = coordinate_1.longitude();
    float lat_2 = coordinate_2.latitude(); float lon_2 = coordinate_2.longitude();

    QGeoCoordinate minCoordinate = QGeoCoordinate(fmax(lat_1, lat_2), fmin(lon_1, lon_2), 0);
    QGeoCoordinate maxCoordinate = QGeoCoordinate(fmin(lat_1, lat_2), fmax(lon_1, lon_2), 0);

    QPoint minPixel = latLonToPixelXY(minCoordinate, zoomLevel);
    QPoint maxPixel = latLonToPixelXY(maxCoordinate, zoomLevel);

    QPoint minTile = pixelXYToTileXY(minPixel);
    QPoint maxTile = pixelXYToTileXY(maxPixel);

    minPixel = tileXYToPixelXY(minTile);
    maxPixel = tileXYToPixelXY(QPoint(maxTile.x() + 1, maxTile.y() + 1)); //since the coordinate is for the top left corner of each tile

    minCoordinate = pixelXYToLatLong(minPixel, zoomLevel);
    maxCoordinate = pixelXYToLatLong(maxPixel, zoomLevel);

    // qDebug() << maxCoordinate.latitude() << "," << minCoordinate.longitude() << ";"<< minCoordinate.latitude()<< "," << maxCoordinate.longitude();
    QGeoCoordinate minCoordinate_ = QGeoCoordinate(maxCoordinate.latitude(), minCoordinate.longitude(), 0);
    QGeoCoordinate maxCoordinate_ = QGeoCoordinate(minCoordinate.latitude(), maxCoordinate.longitude(), 0);

    loadMapTiles(zoomLevel, minTile, maxTile);

    TileStatistics_t _output;
    _output.coordinateMin = minCoordinate_;
    _output.coordinateMax = maxCoordinate_;
    _output.tileCounts = QSize(maxTile.x() - minTile.x() + 1, maxTile.y() - minTile.y() + 1);
    _output.zoomLevel = zoomLevel;

    return _output;
}

void MapTileQuery::adaptiveMapTilesLoader(QString mapType, int mapId, QGeoCoordinate coordinate_1, QGeoCoordinate coordinate_2)
{
    _mapId = mapId;
    _mapType = mapType;
    for(_zoomLevel=MAX_ZOOM_LEVEL; _zoomLevel>0; _zoomLevel--){
        if(maxTileCount(_zoomLevel, coordinate_1, coordinate_2) < MAX_TILE_COUNTS){
            break;
        }
    }

    _textureCoordinateMin = coordinate_1;
    _textureCoordinateMax = coordinate_2;
    emit textureGeometryReady(findAndLoadMapTiles(_zoomLevel, coordinate_1, coordinate_2));
}

int MapTileQuery::maxTileCount(int zoomLevel, QGeoCoordinate coordinateMin, QGeoCoordinate coordinateMax)
{
    double mapSize = powf(2, zoomLevel);
    double latResolution = 180.0 / mapSize;
    double lonResolution = 360.0 / mapSize;

    double latLen = coordinateMax.latitude() - coordinateMin.latitude();
    double lonLen = coordinateMax.longitude() - coordinateMin.longitude();

    int tileXCount = ceil(lonLen/lonResolution);
    int tileYCount = ceil(latLen/latResolution);

    return tileXCount * tileYCount;
}

double MapTileQuery::valueClip(double n, double _minValue, double _maxValue)
{
    return fmin(fmax(n, _minValue), _maxValue);
}

QPoint MapTileQuery::latLonToPixelXY(QGeoCoordinate pointCoordinate, int zoomLevel)
{
    double MinLatitude = -85.05112878;
    double MaxLatitude = 85.05112878;
    double MinLongitude = -180.0f;
    double MaxLongitude = 180.0f;

    double latitude = valueClip(pointCoordinate.latitude(), MinLatitude, MaxLatitude);
    double longitude = valueClip(pointCoordinate.longitude(), MinLongitude, MaxLongitude);

    double x = (longitude + 180) / (360);
    double y = 0;
    if(fabs(latitude) < MaxLatitude){
        double sinLatitude = sin(latitude * DEG_TO_RAD);
        y = 0.5 - log((1 + sinLatitude) / (1 - sinLatitude)) / (4 * PI);
    }else{
        y = (90 - latitude) / 180;
    }

    double mapSize = powf(2, zoomLevel) * 256.0f;
    int pixelX = (int) valueClip(x * mapSize + 0.5, 0, mapSize - 1);
    int pixelY = (int) valueClip(y * mapSize + 0.5, 0, mapSize - 1);

    return QPoint(pixelX, pixelY);
}

QPoint MapTileQuery::pixelXYToTileXY(QPoint pixel)
{
    return  QPoint(pixel.x() / 256, pixel.y() / 256);
}

QPoint MapTileQuery::tileXYToPixelXY(QPoint tile)
{
    return QPoint(tile.x() * 256, tile.y() * 256);
}

QGeoCoordinate MapTileQuery::pixelXYToLatLong(QPoint pixel, int zoomLevel)
{
    double mapSize = powf(2, zoomLevel) * 256.0f;
    double x = (valueClip(pixel.x(), 0, mapSize - 1) / mapSize) - 0.5;
    double y = 0;
    if(pixel.y() <mapSize - 1 && pixel.y() > 0){
        y = 0.5 - (valueClip(pixel.y(), 0, mapSize - 1) / mapSize);
    }else{
        y = (pixel.y() >= mapSize - 1)?(-1):(1);
    }

    double latitude = 90.0f - 360.0f * atan(exp(-y * 2 * PI)) / PI;
    double longitude = 360 * x;
    return QGeoCoordinate(latitude, longitude, 0);
}

void MapTileQuery::tileDone(Viewer3DTileReply::tileInfo_t _tileData)
{
    Viewer3DTileReply* reply = qobject_cast<Viewer3DTileReply*>(QObject::sender());

    QString tileKey = getTileKey(_tileData.mapId, _tileData.x, _tileData.y, _tileData.zoomLevel);
    qsizetype itemRemoved = _mapToBeLoaded.tileList.removeAll(tileKey);

    if(itemRemoved > 0){
        _mapToBeLoaded.currentTileIndex = QPoint(_tileData.x, _tileData.y);
        _mapToBeLoaded.currentTileData = _tileData.data;
        _mapToBeLoaded.currentTileStat = RequestStat::FINISHED;
        _mapToBeLoaded.setMapTile();
        downloadedTilesCount++;
        emit mapTileDownloaded(100.0 * ((float) downloadedTilesCount/ (float)totalTilesCount));

        // qDebug() << _tileData.x << _tileData.y << _tileData.zoomLevel << "tile downloaded!!!" << _mapToBeLoaded.tileList.size();

        if(_mapToBeLoaded.tileList.size() == 0){
            _mapTilesLoadStat = RequestStat::FINISHED;
            qDebug() << "All tiles downloaded ";
            downloadedTilesCount = totalTilesCount;
            emit loadingMapCompleted();
        }

    }
    disconnect(reply, &Viewer3DTileReply::tileDone, this, &MapTileQuery::tileDone);
    disconnect(reply, &Viewer3DTileReply::tileGiveUp, this, &MapTileQuery::tileGiveUp);
    disconnect(reply, &Viewer3DTileReply::tileEmpty, this, &MapTileQuery::tileEmpty);
    reply->deleteLater();
}

void MapTileQuery::tileGiveUp(Viewer3DTileReply::tileInfo_t _tileData)
{
    Viewer3DTileReply* reply = qobject_cast<Viewer3DTileReply*>(QObject::sender());
    disconnect(reply, &Viewer3DTileReply::tileDone, this, &MapTileQuery::tileDone);
    disconnect(reply, &Viewer3DTileReply::tileGiveUp, this, &MapTileQuery::tileGiveUp);
    disconnect(reply, &Viewer3DTileReply::tileEmpty, this, &MapTileQuery::tileEmpty);
    reply->deleteLater();
}

void MapTileQuery::tileEmpty(Viewer3DTileReply::tileInfo_t _tileData)
{
    Viewer3DTileReply* reply = qobject_cast<Viewer3DTileReply*>(QObject::sender());
    disconnect(reply, &Viewer3DTileReply::tileDone, this, &MapTileQuery::tileDone);
    disconnect(reply, &Viewer3DTileReply::tileEmpty, this, &MapTileQuery::tileEmpty);
    reply->deleteLater();
    if(_tileData.zoomLevel > 0 && _tileData.zoomLevel == _zoomLevel){
        _zoomLevel -= 1;
        emit textureGeometryReady(findAndLoadMapTiles(_zoomLevel, _textureCoordinateMin, _textureCoordinateMax));
    }
}

QString MapTileQuery::getTileKey(int mapId, int x, int y, int zoomLevel)
{
    return QString::asprintf("%010d%08d%08d%03d", mapId, x, y, zoomLevel);
}

