#pragma once

#include <QtCore/QObject>

class QNetworkReply;
class QNetworkAccessManager;
class QTimer;

///     @author Omid Esrafilian <esrafilian.omid@gmail.com>

class Viewer3DTileReply : public QObject
{
public:

    typedef struct tileInfo_s{
        int x, y, zoomLevel;
        QByteArray data;
        int mapId;
    } tileInfo_t;

    Q_OBJECT
public:
    explicit Viewer3DTileReply(int zoomLevel, int tileX, int tileY, int mapId, QObject *parent = nullptr);
    ~Viewer3DTileReply();

private:

    QNetworkAccessManager* _networkManager;
    QNetworkReply* _reply;
    tileInfo_t _tile;
    QTimer* _timeoutTimer;
    int _mapId;
    int _timeoutCounter;
    static QByteArray       _bingNoTileImage;

    void prepareDownload();
    void requestFinished();
    void requestError();
    void timeoutTimerEvent();

signals:
    void tileDone(tileInfo_t);
    void tileEmpty(tileInfo_t);
    void tileError(tileInfo_t);
    void tileGiveUp(tileInfo_t);
};
