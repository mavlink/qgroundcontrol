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
    StatkartMapProvider(QObject* parent);
    ~StatkartMapProvider();

  protected:
    QString _getURL(int x, int y, int zoom,
                    QNetworkAccessManager* networkManager);
};

