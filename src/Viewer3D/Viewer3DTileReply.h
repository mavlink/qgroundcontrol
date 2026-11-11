#pragma once

#include <QtCore/QObject>

#include "Viewer3DTileInfo.h"

class QGCTileReply;
class QNetworkAccessManager;
class QTimer;

class Viewer3DTileReply : public QObject
{
    Q_OBJECT

public:
    using TileInfo_t = Viewer3DTileInfo;

    explicit Viewer3DTileReply(int zoomLevel, int tileX, int tileY, int mapId, QNetworkAccessManager* networkManager,
                               QObject* parent = nullptr);

signals:
    void tileDone(TileInfo_t);
    void tileEmpty(TileInfo_t);
    void tileGiveUp(TileInfo_t);

private:
    void _startFetch();
    void _onFetchFinished();
    void _onTimeout();
    void _giveUp(const QString& reason);

    static constexpr int kTimeoutMs = 10000;
    static constexpr int kMaxRetries = 5;

    QNetworkAccessManager* _networkManager = nullptr;
    QGCTileReply* _tileReply = nullptr;
    QTimer* _timeoutTimer = nullptr;

    TileInfo_t _tile;
    int _timeoutCounter = 0;
};
