/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "QGCTileSet.h"
#include <QtLocation/private/qgeomaptype_p.h>
#include <QtLocation/private/qgeocameracapabilities_p.h>

#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QLoggingCategory>

#define AVERAGE_TILE_SIZE 13652
#define MAX_MAP_ZOOM 23.0

Q_DECLARE_LOGGING_CATEGORY(MapProviderLog)

// TODO: Inherit from QGeoMapType
class MapProvider
{
public:
    MapProvider(const QString& mapName, const QString& referrer, const QString& imageFormat, quint32 averageSize,
        QGeoMapType::MapStyle mapStyle = QGeoMapType::CustomMap);

    virtual ~MapProvider();

    virtual QString getTileURL(int x, int y, int zoom) const;

    QString getImageFormat(QByteArrayView image) const;

    quint32 getAverageSize() const { return m_averageSize; }

    QGeoMapType::MapStyle getMapStyle() const { return m_mapStyle; }
    QString getMapName() const { return m_mapName; }
    uint32_t getMapId() const { return m_mapId; }

    QString getReferrer() const { return m_referrer; }
    virtual QByteArray getToken() const { return m_token; }
    QGeoCameraCapabilities getCameraCapabilities() const { return m_cameraCapabilities; }

    virtual int long2tileX(double lon, int z) const;

    virtual int lat2tileY(double lat, int z) const;

    virtual bool isElevationProvider() const { return false; }
    virtual bool isBingProvider() const { return false; }

    virtual QGCTileSet getTileCount(int zoom, double topleftLon,
                                     double topleftLat, double bottomRightLon,
                                     double bottomRightLat) const;

protected:
    static QString _tileXYToQuadKey(int tileX, int tileY, int levelOfDetail);
    static int _getServerNum(int x, int y, int max);

    virtual QString _getURL(int x, int y, int zoom) const = 0;

    const QString m_mapName;
    const QString m_referrer;
    const QString m_imageFormat;
    const quint32 m_averageSize;
    const QByteArray m_token;
    const QGeoMapType::MapStyle m_mapStyle;
    const uint32_t m_mapId;
    // const bool m_requiresToken;
    QGeoCameraCapabilities m_cameraCapabilities;
};
