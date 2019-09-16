#pragma once

#include "MapProvider.h"

#include <QByteArray>
#include <QMutex>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QPoint>
#include <QString>

class StatkartMapProvider : public MapProvider {
    Q_OBJECT
  public:
    StatkartMapProvider(QObject* parent)
        : MapProvider(QString("https://www.norgeskart.no/"), QString("png"),
                      AVERAGE_TILE_SIZE, QGeoMapType::StreetMap, parent) {}

    QString _getURL(int x, int y, int zoom,
                    QNetworkAccessManager* networkManager);
};

class EniroMapProvider : public MapProvider {
    Q_OBJECT
  public:
    EniroMapProvider(QObject* parent)
        : MapProvider(QString("https://www.eniro.se/"), QString("png"),
                      AVERAGE_TILE_SIZE, QGeoMapType::StreetMap, parent) {}

    QString _getURL(int x, int y, int zoom,
                    QNetworkAccessManager* networkManager);
};
