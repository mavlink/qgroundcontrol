#include "maptileimageryloader.h"
// #include "math.h"

#define PI                  acos(-1.0f)
#define DEG_TO_RAD          PI/180.0f
#define RAD_TO_DEG          180.0f/PI

enum RequestStat{
    STARTED,
    IN_PROGRESS,
    FINISHED,
    ERROR,
};

MapTileImageryLoader::MapTileImageryLoader(QObject *parent)
    : QObject{parent}
{
    _manager = new QNetworkAccessManager(this);
    connect(_manager, &QNetworkAccessManager::finished, this, &MapTileImageryLoader::httpServerReplyFinished);
}

void MapTileImageryLoader::loadMapTiles(int zoomLevel, QPoint tileMinIndex, QPoint tileMaxIndex)
{
    _mapTileLoadStat = RequestStat::STARTED;
    _mapToBeLoaded.clear();
    _mapToBeLoaded.zoomLevel = zoomLevel;
    _mapToBeLoaded.tileMinIndex = tileMinIndex;
    _mapToBeLoaded.tileMaxIndex = tileMaxIndex;
    for (int x = tileMinIndex.x(); x <= tileMaxIndex.x(); x++) {
        for (int y = tileMinIndex.y(); y <= tileMaxIndex.y(); y++) {
            _mapToBeLoaded.tilesIndexArray.push_back(QPoint(x, y));
        }
    }
    qDebug() << _mapToBeLoaded.tilesIndexArray.size() << "Tiles to be downloaded!!";
    _mapToBeLoaded.init();
    _loadNextMaptile();
}

std::pair<QGeoCoordinate, QGeoCoordinate> MapTileImageryLoader::findAndLoadMapTiles(int zoomLevel, QGeoCoordinate coordinate_1, QGeoCoordinate coordinate_2)
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
    maxPixel = tileXYToPixelXY(QPoint(maxTile.x() + 1, maxTile.y() + 1));

    minCoordinate = pixelXYToLatLong(minPixel, zoomLevel);
    maxCoordinate = pixelXYToLatLong(maxPixel, zoomLevel);

    // qDebug() << maxCoordinate.latitude() << "," << minCoordinate.longitude() << ";"<< minCoordinate.latitude()<< "," << maxCoordinate.longitude();
    QGeoCoordinate minCoordinate_ = QGeoCoordinate(maxCoordinate.latitude(), minCoordinate.longitude(), 0);
    QGeoCoordinate maxCoordinate_ = QGeoCoordinate(minCoordinate.latitude(), maxCoordinate.longitude(), 0);

    loadMapTiles(zoomLevel, minTile, maxTile);
    return std::pair<QGeoCoordinate, QGeoCoordinate>(minCoordinate_, maxCoordinate_);
}

double MapTileImageryLoader::valueClip(double n, double _minValue, double _maxValue)
{
    return fmin(fmax(n, _minValue), _maxValue);
}

QPoint MapTileImageryLoader::latLonToPixelXY(QGeoCoordinate pointCoordinate, int zoomLevel)
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

QPoint MapTileImageryLoader::pixelXYToTileXY(QPoint pixel)
{
    return  QPoint(pixel.x() / 256, pixel.y() / 256);
}

QPoint MapTileImageryLoader::tileXYToPixelXY(QPoint tile)
{
    return QPoint(tile.x() * 256, tile.y() * 256);
}

QGeoCoordinate MapTileImageryLoader::pixelXYToLatLong(QPoint pixel, int zoomLevel)
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

void MapTileImageryLoader::_loadNextMaptile()
{
    if(_mapToBeLoaded.tilesIndexArray.size() > 0){
        _mapTileLoadStat = RequestStat::IN_PROGRESS;
        _mapToBeLoaded.currentTileStat = RequestStat::STARTED;

        _mapToBeLoaded.currentTileIndex = _mapToBeLoaded.tilesIndexArray.back();
        _mapToBeLoaded.tilesIndexArray.pop_back();
        _mapToBeLoaded.currentTileData.clear();

        // const QString tileName = QString("https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/") +
        //                          QString::number(int(_mapToBeLoaded.zoomLevel)) + QString("/") +
        //                          QString::number(int(_mapToBeLoaded.currentTileIndex.y())) + QString("/") +
        //                          QString::number(int(_mapToBeLoaded.currentTileIndex.x()));

        const QString tileName = bingMapProvider(_mapToBeLoaded.currentTileIndex.x(), _mapToBeLoaded.currentTileIndex.y(), _mapToBeLoaded.zoomLevel);
        // qDebug() << _mapToBeLoaded.currentTileIndex.x() << _mapToBeLoaded.currentTileIndex.y() << _mapToBeLoaded.zoomLevel;
        // qDebug() << tileName;


        QNetworkRequest request;
        request.setUrl(QUrl(tileName));
        _reply = _manager->get(request);
        connect(_reply, &QIODevice::readyRead, this, &MapTileImageryLoader::httpReadyRead);
    }else if(_mapTileLoadStat == RequestStat::IN_PROGRESS){
        _mapTileLoadStat = RequestStat::FINISHED;
        qDebug() << "All tiles downloaded ";
        emit loadingMapCompleted();
    }
}

void MapTileImageryLoader::httpServerReplyFinished()
{
    if(_mapToBeLoaded.currentTileStat == RequestStat::IN_PROGRESS){
        _mapToBeLoaded.currentTileStat = RequestStat::FINISHED;

        _mapToBeLoaded.setMapTile();
        // qDebug() << "Tile index " << _mapToBeLoaded.currentTileIndex << " Downloaded";

        disconnect(_reply, &QIODevice::readyRead, this, &MapTileImageryLoader::httpReadyRead);
        _loadNextMaptile();
    }
}

void MapTileImageryLoader::httpReadyRead()
{
    if(_mapToBeLoaded.currentTileStat == RequestStat::STARTED || _mapToBeLoaded.currentTileStat == RequestStat::IN_PROGRESS){
        _mapToBeLoaded.currentTileStat = RequestStat::IN_PROGRESS;
        _mapToBeLoaded.currentTileData += _reply->readAll();
    }
}

QString MapTileImageryLoader::_tileXYToQuadKey(const int tileX, const int tileY, const int zoomLevel)
{
    QString quadKey;
    for (int i = zoomLevel; i > 0; i--) {
        char digit = '0';
        const int  mask  = 1 << (i - 1);
        if ((tileX & mask) != 0) {
            digit++;
        }
        if ((tileY & mask) != 0) {
            digit++;
            digit++;
        }
        quadKey.append(digit);
    }
    return quadKey;
}

int MapTileImageryLoader::_getServerNum(const int x, const int y, const int max)
{
    return (x + 2 * y) % max;
}

QString MapTileImageryLoader::bingMapProvider(const int tileX, const int tileY, const int zoomLevel, QString  language)
{
    const QString _versionBingMaps = QStringLiteral("563");
    static const QString HybridMapUrl = QStringLiteral("http://ecn.t%1.tiles.virtualearth.net/tiles/h%2.jpeg?g=%3&mkt=%4");
    const QString key = _tileXYToQuadKey(tileX, tileY, zoomLevel);
    return HybridMapUrl.arg(QString::number(_getServerNum(tileX, tileY, 4)), key, _versionBingMaps, language);
}
