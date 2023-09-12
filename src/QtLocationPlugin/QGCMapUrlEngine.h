/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 *  @file
 *  @author Gus Grubba <gus@auterion.com>
 */

#ifndef QGC_MAP_URL_ENGINE_H
#define QGC_MAP_URL_ENGINE_H


#include "GoogleMapProvider.h"
#include "BingMapProvider.h"
#include "GenericMapProvider.h"
#include "EsriMapProvider.h"
#include "MapboxMapProvider.h"
#include "ElevationMapProvider.h"

#define MAX_MAP_ZOOM (23.0)

class UrlFactory : public QObject {
    Q_OBJECT
public:
    static const char* kCopernicusElevationProviderKey;
    static const char* kCopernicusElevationProviderNotice;

    UrlFactory      ();
    ~UrlFactory     ();

    QNetworkRequest getTileURL          (const QString& type, int x, int y, int zoom, QNetworkAccessManager* networkManager);
    QNetworkRequest getTileURL          (int id, int x, int y, int zoom, QNetworkAccessManager* networkManager);

    QString         getImageFormat      (const QString& type, const QByteArray& image);
    QString         getImageFormat      (int id , const QByteArray& image);

    quint32  averageSizeForType  (const QString& type);

    int long2tileX(const QString& mapType, double lon, int z);
    int lat2tileY(const QString& mapType, double lat, int z);

    QHash<QString, MapProvider*> getProviderTable(){return _providersTable;}

    int getIdFromType(const QString& type);
    QString getTypeFromId(int id);
    MapProvider* getMapProviderFromId(int id);

    QGCTileSet getTileCount(int zoom, double topleftLon, double topleftLat,
                            double bottomRightLon, double bottomRightLat,
                            const QString& mapType);

    bool isElevation(int mapId);

  private:
    int             _timeout;
    QHash<QString, MapProvider*> _providersTable;
    void registerProvider(QString Name, MapProvider* provider);

};

#endif
