#include "Viewer3DTileReply.h"

#include "QGCMapEngine.h"


Viewer3DTileReply::Viewer3DTileReply(int zoomLevel, int tileX, int tileY, int mapId, QObject *parent)
    : QObject{parent}
{
    _timeoutCounter = 0;
    _timeoutTimer = new QTimer(this);
    _networkManager = new QNetworkAccessManager(this);
    _networkManager->setTransferTimeout(9000);
    // connect(_networkManager, &QNetworkAccessManager::finished, this, &Viewer3DTileReply::requestFinished);

    _tile.x = tileX;
    _tile.y = tileY;
    _tile.zoomLevel = zoomLevel;
    _tile.mapId = mapId;
    _tile.data.clear();
    _mapId = mapId;
    prepareDownload();

    _timeoutTimer->start(10000);
    connect(_timeoutTimer, &QTimer::timeout, this, &Viewer3DTileReply::timeoutTimerEvent);
}

Viewer3DTileReply::~Viewer3DTileReply()
{
    delete _networkManager;
    delete _timeoutTimer;
}

void Viewer3DTileReply::prepareDownload()
{
    QNetworkRequest request = getQGCMapEngine()->urlFactory()->getTileURL(_mapId, _tile.x, _tile.y, _tile.zoomLevel, _networkManager);
    _reply = _networkManager->get(request);
    connect(_reply, &QNetworkReply::finished, this, &Viewer3DTileReply::requestFinished);
    connect(_reply, &QNetworkReply::errorOccurred, this, &Viewer3DTileReply::requestError);
}

void Viewer3DTileReply::requestFinished()
{
    _tile.data = _reply->readAll();
    // disconnect(_networkManager, &QNetworkAccessManager::finished, this, &Viewer3DTileReply::requestFinished);
    _timeoutTimer->stop();
    disconnect(_reply, &QNetworkReply::finished, this, &Viewer3DTileReply::requestFinished);
    disconnect(_reply, &QNetworkReply::errorOccurred, this, &Viewer3DTileReply::requestError);
    disconnect(_timeoutTimer, &QTimer::timeout, this, &Viewer3DTileReply::timeoutTimerEvent);
    emit tileDone(_tile);
}

void Viewer3DTileReply::requestError()
{
    emit tileError(_tile);
    disconnect(_reply, &QNetworkReply::finished, this, &Viewer3DTileReply::requestFinished);
    disconnect(_reply, &QNetworkReply::errorOccurred, this, &Viewer3DTileReply::requestError);
}

void Viewer3DTileReply::timeoutTimerEvent()
{
    if(_timeoutCounter > 5){
        _timeoutCounter = 0;
        _networkManager->setTransferTimeout(14000);
        _timeoutTimer->stop();
        _timeoutTimer->start(15000);
        // disconnect(_reply, &QNetworkReply::finished, this, &Viewer3DTileReply::requestFinished);
        // disconnect(_reply, &QNetworkReply::errorOccurred, this, &Viewer3DTileReply::requestError);
        // disconnect(_timeoutTimer, &QTimer::timeout, this, &Viewer3DTileReply::timeoutTimerEvent);
        // emit tileGiveUp(_tile);
        // _timeoutTimer->stop();
    }else if(_tile.data.isEmpty()){
        emit tileError(_tile);
        prepareDownload();
        _timeoutCounter++;
    }
}
