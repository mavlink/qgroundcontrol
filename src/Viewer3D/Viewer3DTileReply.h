#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QString>

Q_DECLARE_LOGGING_CATEGORY(Viewer3DTileReplyLog)

struct QGCCacheTile;
class QNetworkAccessManager;
class QNetworkReply;
class QTimer;

class Viewer3DTileReply : public QObject
{
    Q_OBJECT

public:
    struct TileInfo_t {
        QByteArray data;
        int x = 0;
        int y = 0;
        int zoomLevel = 0;
        int mapId = 0;
    };

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
