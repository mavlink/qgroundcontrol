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

#include <QString>
#include <QPoint>
#include <QByteArray>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QMutex>

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

        BingMap                 = 444,
        BingSatellite           = 555,
        BingHybrid              = 666,

        MapQuestMap             = 700,
        MapQuestSat             = 701,

        MapBoxStreets           = 6000,
        MapBoxLight             = 6001,
        MapBoxDark              = 6002,
        MapBoxSatellite         = 6003,
        MapBoxHybrid            = 6004,
        MapBoxWheatPaste        = 6005,
        MapBoxStreetsBasic      = 6006,
        MapBoxComic             = 6007,
        MapBoxOutdoors          = 6008,
        MapBoxRunBikeHike       = 6009,
        MapBoxPencil            = 6010,
        MapBoxPirates           = 6011,
        MapBoxEmerald           = 6012,
        MapBoxHighContrast      = 6013
    };

    UrlFactory      ();
    ~UrlFactory     ();

    QNetworkRequest getTileURL          (MapType type, int x, int y, int zoom, QNetworkAccessManager* networkManager);
    QString         getImageFormat      (MapType type, const QByteArray& image);

    static quint32  averageSizeForType  (MapType type);

private slots:
    void    _networkReplyError          (QNetworkReply::NetworkError error);
    void    _googleVersionCompleted     ();
    void    _replyDestroyed             ();

private:
    QString _getURL                     (MapType type, int x, int y, int zoom, QNetworkAccessManager* networkManager);
    void    _getSecGoogleWords          (int x, int y, QString& sec1, QString& sec2);
    int     _getServerNum               (int x, int y, int max);
    void    _tryCorrectGoogleVersions   (QNetworkAccessManager* networkManager);
    QString _tileXYToQuadKey            (int tileX, int tileY, int levelOfDetail);

    int             _timeout;
    bool            _googleVersionRetrieved;
    QNetworkReply*  _googleReply;
    QMutex          _googleVersionMutex;
    QByteArray      _userAgent;
    QString         _language;

    // Google version strings
    QString         _versionGoogleMap;
    QString         _versionGoogleSatellite;
    QString         _versionGoogleLabels;
    QString         _versionGoogleTerrain;
    QString         _secGoogleWord;
    // BingMaps
    QString         _versionBingMaps;

};

#endif
