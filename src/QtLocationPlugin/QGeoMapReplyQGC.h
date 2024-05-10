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
    QGeoTiledMapReplyQGC(QNetworkAccessManager* networkManager, const QNetworkRequest &request, const QGeoTileSpec &spec, QObject *parent = nullptr);
    ~QGeoTiledMapReplyQGC();

private slots:
    void _networkReplyFinished();
    void _networkReplyError(QNetworkReply::NetworkError error);
    void _cacheReply(QGCCacheTile* tile);
    void _cacheError(QGCMapTask::TaskType type, const QString &errorString);

private:
    void _initDataFromResources();

    QNetworkReply* m_reply = nullptr;
    QNetworkAccessManager* m_networkManager = nullptr;
    QNetworkRequest m_request;

    static QByteArray s_bingNoTileImage;
    static QByteArray s_badTile;
};
