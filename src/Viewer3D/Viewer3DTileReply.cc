/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "Viewer3DTileReply.h"

#include <MapProvider.h>
#include <QGCMapUrlEngine.h>
#include <QGeoTileFetcherQGC.h>

#include <QtCore/QFile>
#include <QtCore/QTimer>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

QByteArray  Viewer3DTileReply::_bingNoTileImage;

Viewer3DTileReply::Viewer3DTileReply(int zoomLevel, int tileX, int tileY, int mapId, QObject *parent)
    : QObject{parent}
{
    if (_bingNoTileImage.length() == 0) {
        QFile file(":/res/BingNoTileBytes.dat");
        file.open(QFile::ReadOnly);
        _bingNoTileImage = file.readAll();
        file.close();
    }

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
    const QNetworkRequest request = QGeoTileFetcherQGC::getNetworkRequest(_mapId, _tile.x, _tile.y, _tile.zoomLevel);
    _reply = _networkManager->get(request);
    connect(_reply, &QNetworkReply::finished, this, &Viewer3DTileReply::requestFinished);
    connect(_reply, &QNetworkReply::errorOccurred, this, &Viewer3DTileReply::requestError);
}

void Viewer3DTileReply::requestFinished()
{
    _tile.data = _reply->readAll();
    const SharedMapProvider mapProvider = UrlFactory::getMapProviderFromQtMapId(_tile.mapId);
    // disconnect(_networkManager, &QNetworkAccessManager::finished, this, &Viewer3DTileReply::requestFinished);
    _timeoutTimer->stop();
    disconnect(_reply, &QNetworkReply::finished, this, &Viewer3DTileReply::requestFinished);
    disconnect(_reply, &QNetworkReply::errorOccurred, this, &Viewer3DTileReply::requestError);
    disconnect(_timeoutTimer, &QTimer::timeout, this, &Viewer3DTileReply::timeoutTimerEvent);

    if(mapProvider && mapProvider->isBingProvider() && _tile.data.size() && _tile.data == _bingNoTileImage){
        // Bing doesn't return an error if you request a tile above supported zoom level
        // It instead returns an image of a missing tile graphic. We need to detect that
        // and error out so 3D View will deal with zooming correctly even if it doesn't have the tile.
        // This allows us to zoom up to level 23 even though the tiles don't actually exist
        // so we clear the data to imdicate it is not a valid tile
        _tile.data.clear();
        emit tileEmpty(_tile);
        return;
    }
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
        // _timeoutCounter = 0;
        // _networkManager->setTransferTimeout(14000);
        // _timeoutTimer->stop();
        // _timeoutTimer->start(15000);
        disconnect(_reply, &QNetworkReply::finished, this, &Viewer3DTileReply::requestFinished);
        disconnect(_reply, &QNetworkReply::errorOccurred, this, &Viewer3DTileReply::requestError);
        disconnect(_timeoutTimer, &QTimer::timeout, this, &Viewer3DTileReply::timeoutTimerEvent);
        emit tileGiveUp(_tile);
        _timeoutTimer->stop();
    }else if(_tile.data.isEmpty()){
        emit tileError(_tile);
        prepareDownload();
        _timeoutCounter++;
    }
}
