/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QByteArray>
#include <QString>

#include <cmath>

#include "QGCTileSet.h" 
#include <QtLocation/private/qgeomaptype_p.h>

static const unsigned char pngSignature[]  = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00};
static const unsigned char jpegSignature[] = {0xFF, 0xD8, 0xFF, 0x00};
static const unsigned char gifSignature[]  = {0x47, 0x49, 0x46, 0x38, 0x00};

static const quint32 AVERAGE_TILE_SIZE = 13652;

class QNetworkRequest;
class QNetworkAccessManager;

class MapProvider : public QObject {
    Q_OBJECT

public:
    MapProvider(const QString& referrer, const QString& imageFormat, const quint32 averageSize,
        const QGeoMapType::MapStyle mapType = QGeoMapType::CustomMap, QObject* parent = nullptr);

    virtual QNetworkRequest getTileURL(const int x, const int y, const int zoom, QNetworkAccessManager* networkManager);

    QString getImageFormat(const QByteArray& image) const;

    quint32 getAverageSize() const { return _averageSize; }

    QGeoMapType::MapStyle getMapStyle() { return _mapType; }

    virtual int long2tileX(const double lon, const int z) const;

    virtual int lat2tileY(const double lat, const int z) const;

    virtual bool _isElevationProvider() const;

    virtual QGCTileSet getTileCount(const int zoom, const double topleftLon,
                                     const double topleftLat, const double bottomRightLon,
                                     const double bottomRightLat) const;

protected:
    QString _tileXYToQuadKey(const int tileX, const int tileY, const int levelOfDetail) const;
    int _getServerNum(const int x, const int y, const int max) const;
    // Define the url to Request
    virtual QString _getURL(const int x, const int y, const int zoom, QNetworkAccessManager* networkManager) = 0;

    // Define Referrer for Request RawHeader
    QString     _referrer;
    QString     _imageFormat;
    quint32     _averageSize;
    QByteArray  _userAgent;
    QString     _language;
    QGeoMapType::MapStyle _mapType;

};
