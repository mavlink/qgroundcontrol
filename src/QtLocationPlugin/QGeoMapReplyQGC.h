#pragma once

#include <QtCore/QPointer>
#include <QtLocation/private/qgeotiledmapreply_p.h>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

#include "QGCMapTaskBase.h"

struct QGCCacheTile;
class QNetworkAccessManager;
class QSslError;

class QGeoTiledMapReplyQGC : public QGeoTiledMapReply
{
    Q_OBJECT

    friend class QGeoMapReplyQGCTest;

public:
    explicit QGeoTiledMapReplyQGC(QNetworkAccessManager *networkManager, const QNetworkRequest &request, const QGeoTileSpec &spec, QObject *parent = nullptr);
    ~QGeoTiledMapReplyQGC();

    bool init();
    void abort() final;

private slots:
    void _networkReplyFinished();
    void _networkReplyError(QNetworkReply::NetworkError error);
    void _networkReplySslErrors(const QList<QSslError> &errors);
    void _cacheReply(QGCCacheTile *tile);
    void _cacheError(QGCMapTask::TaskType type, QStringView errorString);

private:
    static void _initDataFromResources();

    QPointer<QNetworkAccessManager> _networkManager;
    QPointer<QNetworkReply> _networkReply;
    QNetworkRequest _request;
    bool m_initialized = false;

    static QByteArray _bingNoTileImage;
    static QByteArray _badTile;
};
