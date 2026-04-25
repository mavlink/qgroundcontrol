#pragma once

#include <QtCore/QObject>
#include <QtCore/qcontainerfwd.h>

#include "Viewer3DTileInfo.h"

struct QGCCacheTile;
class QNetworkAccessManager;
class QNetworkReply;
class QTimer;

class Viewer3DTileReply : public QObject
{
    Q_OBJECT

public:
    using TileInfo_t = Viewer3DTileInfo;

    explicit Viewer3DTileReply(int zoomLevel, int tileX, int tileY, int mapId, const QString &mapType, QNetworkAccessManager *networkManager, QObject *parent = nullptr);
    ~Viewer3DTileReply();

signals:
    void tileDone(TileInfo_t);
    void tileEmpty(TileInfo_t);
    void tileGiveUp(TileInfo_t);

private:
    void _prepareDownload();
    void _onRequestFinished();
    void _onRequestError();
    void _onTimeout();
    void _onCacheHit(QGCCacheTile *tile);
    void _onCacheMiss();
    void _disconnectReply();
    bool _isBingEmptyTile() const;

    static constexpr int kTimeoutMs   = 10000;
    static constexpr int kMaxRetries  = 5;

    QNetworkAccessManager *_networkManager = nullptr;
    QNetworkReply *_reply = nullptr;
    QTimer *_timeoutTimer = nullptr;

    TileInfo_t _tile;
    int _timeoutCounter = 0;

    static QByteArray _bingNoTileImage;
};
