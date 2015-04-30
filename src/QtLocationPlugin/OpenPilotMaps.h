/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

    QGROUNDCONTROL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QGROUNDCONTROL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 *  @file
 *  @brief  QGC Open Pilot Mapping Tools
 *  @author Gus Grubba <mavlink@grubba.com>
 *  Original work: The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 */

#ifndef OPENPILOTTOOLS_H
#define OPENPILOTTOOLS_H

#include <QString>
#include <QPoint>
#include <QByteArray>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QMutex>

#define MAX_MAP_ZOOM (20.0)

namespace OpenPilot {

enum MapType
{
        GoogleMap               = 1,
        GoogleSatellite         = 4,
        GoogleLabels            = 8,
        GoogleTerrain           = 16,
        GoogleHybrid            = 20,

        GoogleMapChina          = 22,
        GoogleSatelliteChina    = 24,
        GoogleLabelsChina       = 26,
        GoogleTerrainChina      = 28,
        GoogleHybridChina       = 29,

        OpenStreetMap           = 32,
        OpenStreetOsm           = 33,
        OpenStreetMapSurfer     = 34,
        OpenStreetMapSurferTerrain=35,

        YahooMap                = 64,
        YahooSatellite          = 128,
        YahooLabels             = 256,
        YahooHybrid             = 333,

        BingMap                 = 444,
        BingSatellite           = 555,
        BingHybrid              = 666,

        ArcGIS_Map              = 777,
        ArcGIS_Satellite        = 788,
        ArcGIS_ShadedRelief     = 799,
        ArcGIS_Terrain          = 811,

        ArcGIS_MapsLT_Map       = 1000,
        ArcGIS_MapsLT_OrtoFoto  = 1001,
        ArcGIS_MapsLT_Map_Labels= 1002,
        ArcGIS_MapsLT_Map_Hybrid= 1003,

        PergoTurkeyMap          = 2001,
        SigPacSpainMap          = 3001,

        GoogleMapKorea          = 4001,
        GoogleSatelliteKorea    = 4002,
        GoogleLabelsKorea       = 4003,
        GoogleHybridKorea       = 4005,

        YandexMapRu             = 5000
};

class ProviderStrings : public QObject {
    Q_OBJECT
public:
    ProviderStrings();
    static const QString kLevelsForSigPacSpainMap[];
    QString GoogleMapsAPIKey;
    // Google version strings
    QString VersionGoogleMap;
    QString VersionGoogleSatellite;
    QString VersionGoogleLabels;
    QString VersionGoogleTerrain;
    QString SecGoogleWord;
    // Google (China) version strings
    QString VersionGoogleMapChina;
    QString VersionGoogleSatelliteChina;
    QString VersionGoogleLabelsChina;
    QString VersionGoogleTerrainChina;
    // Google (Korea) version strings
    QString VersionGoogleMapKorea;
    QString VersionGoogleSatelliteKorea;
    QString VersionGoogleLabelsKorea;
    /// <summary>
    /// Google Maps API generated using http://greatmaps.codeplex.com/
    /// from http://code.google.com/intl/en-us/apis/maps/signup.html
    /// </summary>
    // Yahoo version strings
    QString VersionYahooMap;
    QString VersionYahooSatellite;
    QString VersionYahooLabels;
    // BingMaps
    QString VersionBingMaps;
    // YandexMap
    QString VersionYandexMap;
    /// <summary>
    /// Bing Maps Customer Identification, more info here
    /// http://msdn.microsoft.com/en-us/library/bb924353.aspx
    /// </summary>
    QString BingMapsClientToken;
};

class UrlFactory : public ProviderStrings {
    Q_OBJECT
public:

    UrlFactory(QNetworkAccessManager* network);
    ~UrlFactory();

    QString makeImageUrl                (const MapType &type, const QPoint &pos, const int &zoom, const QString &language);

private slots:
    void    _networkReplyError          (QNetworkReply::NetworkError error);
    void    _googleVersionCompleted     ();
    void    _replyDestroyed             ();

private:
    void    _getSecGoogleWords          (const QPoint &pos, QString &sec1, QString &sec2);
    int     _getServerNum               (const QPoint& pos, const int &max) const;
    void    _tryCorrectGoogleVersions   ();
    QString _tileXYToQuadKey            (const int &tileX, const int &tileY, const int &levelOfDetail) const;

    int                     _timeout;
    bool                    _googleVersionRetrieved;
    QNetworkAccessManager*  _network;
    QNetworkReply*          _googleReply;
    QMutex                  _googleVersionMutex;
    QByteArray              _userAgent;
};

}

#endif // FOO_H
