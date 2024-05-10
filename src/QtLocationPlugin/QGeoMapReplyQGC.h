#pragma once

#include <QtNetwork/QNetworkReply>
#include <QtLocation/private/qgeotiledmapreply_p.h>
#include <QtCore/QLoggingCategory>

#include "QGCMapEngineData.h"

Q_DECLARE_LOGGING_CATEGORY(QGeoTiledMapReplyQGCLog)

class QGeoTiledMapReplyQGC : public QGeoTiledMapReply
{
    Q_OBJECT

public:
    QGeoTiledMapReplyQGC(QNetworkAccessManager* networkManager, const QGeoTileSpec &spec, QObject *parent = nullptr);
    ~QGeoTiledMapReplyQGC();

private slots:
    void _networkReplyFinished();
    void _networkReplyError(QNetworkReply::NetworkError error);
    void _cacheReply(QGCCacheTile* tile);
    void _cacheError(QGCMapTask::TaskType type, QStringView errorString);

private:
    void _initDataFromResources();

    QNetworkAccessManager* m_networkManager = nullptr;

    static QByteArray s_bingNoTileImage;
    static QByteArray s_badTile;
};
