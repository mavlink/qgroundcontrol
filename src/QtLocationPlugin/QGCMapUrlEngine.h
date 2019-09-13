/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 *  @file
 *  @author Gus Grubba <mavlink@grubba.com>
 */

#ifndef QGC_MAP_URL_ENGINE_H
#define QGC_MAP_URL_ENGINE_H

#include "GoogleMapProvider.h"

#define MAX_MAP_ZOOM (20.0)

class UrlFactory : public QObject {
    Q_OBJECT
public:

    enum MapType
    {
        Invalid                 = -1,

        GoogleMap               = 1,
        GoogleSatellite         = 4,
        GoogleLabels            = 8,
        GoogleTerrain           = 16,
        GoogleHybrid            = 20,

        OpenStreetMap           = 32,
        OpenStreetOsm           = 33,
        OpenStreetMapSurfer     = 34,
        OpenStreetMapSurferTerrain=35,

        StatkartTopo            = 100,
        EniroTopo               = 110,

        BingMap                 = 444,
        BingSatellite           = 555,
        BingHybrid              = 666,

        /*
        MapQuestMap             = 700,
        MapQuestSat             = 701,
        */

        VWorldMap                = 800,
        VWorldSatellite          = 801,
        VWorldStreet             = 802,

        MapboxStreets           = 6000,
        MapboxLight             = 6001,
        MapboxDark              = 6002,
        MapboxSatellite         = 6003,
        MapboxHybrid            = 6004,
        MapboxWheatPaste        = 6005,
        MapboxStreetsBasic      = 6006,
        MapboxComic             = 6007,
        MapboxOutdoors          = 6008,
        MapboxRunBikeHike       = 6009,
        MapboxPencil            = 6010,
        MapboxPirates           = 6011,
        MapboxEmerald           = 6012,
        MapboxHighContrast      = 6013,

        EsriWorldStreet         = 7000,
        EsriWorldSatellite      = 7001,
        EsriTerrain             = 7002,

        AirmapElevation         = 8001
    };

    UrlFactory      ();
    ~UrlFactory     ();

    QNetworkRequest getTileURL          (MapType type, int x, int y, int zoom, QNetworkAccessManager* networkManager);
    QString         getImageFormat      (MapType type, const QByteArray& image);

    static quint32  averageSizeForType  (MapType type);


//private:
//    QString _getURL                     (MapType type, int x, int y, int zoom, QNetworkAccessManager* networkManager);

private:
    int             _timeout;

    // BingMaps
    //QString         _versionBingMaps;
    MapProvider*   _curMapProvider;

    QHash<QString, MapProvider*> _providersTable;

};

#endif
