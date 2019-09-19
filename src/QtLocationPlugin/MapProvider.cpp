#include "MapProvider.h"

MapProvider::MapProvider(QString referrer, QString imageFormat,
                         quint32 averageSize, QGeoMapType::MapStyle mapType,QObject* parent)
    : QObject(parent), _referrer(referrer), _imageFormat(imageFormat),
      _averageSize(averageSize), _mapType(mapType) {
    QStringList langs = QLocale::system().uiLanguages();
    if (langs.length() > 0) {
        _language = langs[0];
    }
}

QNetworkRequest MapProvider::getTileURL(int x, int y, int zoom,
                                        QNetworkAccessManager* networkManager) {
    //-- Build URL
    QNetworkRequest request;
    QString         url = _getURL(x, y, zoom, networkManager);
    if (url.isEmpty()) {
        return request;
    }
    request.setUrl(QUrl(url));
    request.setRawHeader("Accept", "*/*");
    request.setRawHeader("Referrer", _referrer.toUtf8());
    request.setRawHeader("User-Agent", _userAgent);
    return request;
}

QString MapProvider::getImageFormat(const QByteArray& image) {
    QString format;
    if (image.size() > 2) {
        if (image.startsWith(reinterpret_cast<const char*>(pngSignature)))
            format = "png";
        else if (image.startsWith(reinterpret_cast<const char*>(jpegSignature)))
            format = "jpg";
        else if (image.startsWith(reinterpret_cast<const char*>(gifSignature)))
            format = "gif";
        else {
            return _imageFormat;
        }
    }
    return format;
}

QString MapProvider::_tileXYToQuadKey(int tileX, int tileY, int levelOfDetail) {
    QString quadKey;
    for (int i = levelOfDetail; i > 0; i--) {
        char digit = '0';
        int  mask  = 1 << (i - 1);
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

int MapProvider::_getServerNum(int x, int y, int max) {
    return (x + 2 * y) % max;
}

int MapProvider::long2tileX(double lon, int z) {
    return static_cast<int>(floor((lon + 180.0) / 360.0 * pow(2.0, z)));
}

//-----------------------------------------------------------------------------
int MapProvider::lat2tileY(double lat, int z) {
    return static_cast<int>(floor(
        (1.0 -
         log(tan(lat * M_PI / 180.0) + 1.0 / cos(lat * M_PI / 180.0)) / M_PI) /
        2.0 * pow(2.0, z)));
}

bool MapProvider::_isElevationProvider() { return false; }

QGCTileSet MapProvider::getTileCount(int zoom, double topleftLon,
                                     double topleftLat, double bottomRightLon,
                                     double bottomRightLat) {
    QGCTileSet set;
    set.tileX0 = long2tileX(topleftLon, zoom);
    set.tileY0 = lat2tileY(topleftLat, zoom);
    set.tileX1 = long2tileX(bottomRightLon, zoom);
    set.tileY1 = lat2tileY(bottomRightLat, zoom);

    set.tileCount = (static_cast<quint64>(set.tileX1) -
                     static_cast<quint64>(set.tileX0) + 1) *
                    (static_cast<quint64>(set.tileY1) -
                     static_cast<quint64>(set.tileY0) + 1);

    set.tileSize = getAverageSize() * set.tileCount;
    return set;
}
