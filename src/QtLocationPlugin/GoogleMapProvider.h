#pragma once

#include "MapProvider.h"

#include <QByteArray>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QString>
#include <QPoint>
#include <QMutex>

class GoogleMapProvider : public MapProvider {
    Q_OBJECT
  public:
    GoogleMapProvider(QObject* parent);

    ~GoogleMapProvider();

    static quint32 getAverageSize();
    // Google Specific private slots
  private slots:
    void _networkReplyError(QNetworkReply::NetworkError error);
    void _googleVersionCompleted();
    void _replyDestroyed();

  protected:
    // Define the url to Request
    QString _getURL(int x, int y, int zoom,
                    QNetworkAccessManager* networkManager) ;

    // Google Specific private methods
    void _getSecGoogleWords(int x, int y, QString& sec1, QString& sec2);
    void _tryCorrectGoogleVersions(QNetworkAccessManager* networkManager);

    // Google Specific attributes
    bool           _googleVersionRetrieved;
    QNetworkReply* _googleReply;
    QMutex         _googleVersionMutex;
    QString        _versionGoogleMap;
    QString        _versionGoogleSatellite;
    QString        _versionGoogleLabels;
    QString        _versionGoogleTerrain;
    QString        _secGoogleWord;
};

class GoogleSatelliteMapProvider : public GoogleMapProvider {
    Q_OBJECT
  public:
    GoogleSatelliteMapProvider(QObject* parent):GoogleMapProvider(parent){}
  protected:
    QString _getURL(int x, int y, int zoom,
                    QNetworkAccessManager* networkManager);
};

class GoogleLabelsMapProvider : public GoogleMapProvider {
    Q_OBJECT
  public:
    GoogleLabelsMapProvider(QObject* parent):GoogleMapProvider(parent){}
  protected:
    QString _getURL(int x, int y, int zoom,
                    QNetworkAccessManager* networkManager);
};

class GoogleTerrainMapProvider : public GoogleMapProvider {
    Q_OBJECT
  public:
    GoogleTerrainMapProvider(QObject* parent):GoogleMapProvider(parent){}
  protected:
    QString _getURL(int x, int y, int zoom,
                    QNetworkAccessManager* networkManager);
};
