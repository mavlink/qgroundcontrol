#pragma once

#include <QtCore/QLoggingCategory>
#include <QtLocation/private/qgeotiledmapreply_p.h>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

#include "QGCMapTasks.h"

Q_DECLARE_LOGGING_CATEGORY(QGeoTiledMapReplyQGCLog)

class QNetworkAccessManager;
class QSslError;

class QGeoTiledMapReplyQGC : public QGeoTiledMapReply
{
    Q_OBJECT

public:
    QGeoTiledMapReplyQGC(QNetworkAccessManager *networkManager, const QNetworkRequest &request, const QGeoTileSpec &spec, QObject *parent = nullptr);
    ~QGeoTiledMapReplyQGC();

    void abort() final;

private slots:
    void _networkReplyFinished();
    void _networkReplyError(QNetworkReply::NetworkError error);
#if QT_CONFIG(ssl)
    void _networkReplySslErrors(const QList<QSslError> &errors);
#endif
    void _cacheReply(QGCCacheTile *tile);
    void _cacheError(QGCMapTask::TaskType type, QStringView errorString);

private:
    static void _initDataFromResources();

    QNetworkAccessManager *_networkManager = nullptr;
    QNetworkRequest _request;

    static QByteArray _bingNoTileImage;
    static QByteArray _badTile;

    enum HTTP_Response {
        SUCCESS_OK = 200,
        REDIRECTION_MULTIPLE_CHOICES = 300
    };
};
