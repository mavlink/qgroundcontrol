/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtLocation/private/qgeomaptype_p.h>
#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QLoggingCategory>

#include "QGCTileSet.h"

Q_DECLARE_LOGGING_CATEGORY(MapProviderLog)

// qgeomaptype_p.h
/*enum MapStyle {
    NoMap = 0,
    StreetMap,
    SatelliteMapDay,
    SatelliteMapNight,
    TerrainMap,
    HybridMap,
    TransitMap,
    GrayStreetMap,
    PedestrianMap,
    CarNavigationMap,
    CycleMap,
    CustomMap = 100
};*/

static constexpr const quint32 AVERAGE_TILE_SIZE = 13652;

class QNetworkRequest;

class MapProvider
{
public:
    MapProvider(const QString &mapName, const QString &referrer, const QString &imageFormat, quint32 averageSize = AVERAGE_TILE_SIZE,
                QGeoMapType::MapStyle mapStyle = QGeoMapType::CustomMap);
    virtual ~MapProvider();

    QString getImageFormat(QByteArrayView image) const;

    // TODO: Download Random Tile And Use That Size Instead?
    quint32 getAverageSize() const { return _averageSize; }

    QGeoMapType::MapStyle getMapStyle() const { return _mapStyle; }
    const QString& getMapName() const { return _mapName; }
    int getMapId() const { return _mapId; }

    virtual QNetworkRequest getTileURL(int x, int y, int zoom) const;

    virtual int long2tileX(double lon, int z) const;
    virtual int lat2tileY(double lat, int z) const;

    virtual bool isElevationProvider() const { return false; }
    virtual bool isBingProvider() const { return false; }

    virtual QGCTileSet getTileCount(int zoom, double topleftLon,
                                    double topleftLat, double bottomRightLon,
                                    double bottomRightLat) const;

protected:
    QString _tileXYToQuadKey(int tileX, int tileY, int levelOfDetail) const;
    int _getServerNum(int x, int y, int max) const;

    virtual QString _getURL(int x, int y, int zoom) const = 0;

    const QString _mapName;
    const QString _referrer;
    const QString _imageFormat;
    const quint32 _averageSize;
    const QGeoMapType::MapStyle _mapStyle;
    const QString _language;
    const int _mapId;

private:
    static int _mapIdIndex;
};
