/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief Map Tile Set
 *
 *   @author Gus Grubba <gus@auterion.com>
 *
 */

#include "QGCMapEngine.h"
#include "QGCMapTileSet.h"
#include "QGCMapEngineManager.h"
#include "TerrainTile.h"

#include <QSettings>
#include <math.h>

QGC_LOGGING_CATEGORY(QGCCachedTileSetLog, "QGCCachedTileSetLog")

#define TILE_BATCH_SIZE      256

//-----------------------------------------------------------------------------
QGCCachedTileSet::QGCCachedTileSet(const QString& name)
    : _name(name)
    , _topleftLat(0.0)
    , _topleftLon(0.0)
    , _bottomRightLat(0.0)
    , _bottomRightLon(0.0)
    , _totalTileCount(0)
    , _totalTileSize(0)
    , _uniqueTileCount(0)
    , _uniqueTileSize(0)
    , _savedTileCount(0)
    , _savedTileSize(0)
    , _minZoom(3)
    , _maxZoom(3)
    , _defaultSet(false)
    , _deleting(false)
    , _downloading(false)
    , _id(0)
    , _type("Invalid")
    , _networkManager(nullptr)
    , _errorCount(0)
    , _noMoreTiles(false)
    , _batchRequested(false)
    , _manager(nullptr)
    , _selected(false)
{

}

//-----------------------------------------------------------------------------
QGCCachedTileSet::~QGCCachedTileSet()
{
    delete _networkManager;
    _networkManager = nullptr;
}

//-----------------------------------------------------------------------------
QString
QGCCachedTileSet::errorCountStr() const
{
    return QGCMapEngine::numberToString(_errorCount);
}

//-----------------------------------------------------------------------------
QString
QGCCachedTileSet::totalTileCountStr() const
{
    return QGCMapEngine::numberToString(_totalTileCount);
}

//-----------------------------------------------------------------------------
QString
QGCCachedTileSet::totalTilesSizeStr() const
{
    return QGCMapEngine::bigSizeToString(_totalTileSize);
}

//-----------------------------------------------------------------------------
QString
QGCCachedTileSet::uniqueTileSizeStr() const
{
    return QGCMapEngine::bigSizeToString(_uniqueTileSize);
}

//-----------------------------------------------------------------------------
QString
QGCCachedTileSet::uniqueTileCountStr() const
{
    return QGCMapEngine::numberToString(_uniqueTileCount);
}

//-----------------------------------------------------------------------------
QString
QGCCachedTileSet::savedTileCountStr() const
{
    return QGCMapEngine::numberToString(_savedTileCount);
}

//-----------------------------------------------------------------------------
QString
QGCCachedTileSet::savedTileSizeStr() const
{
    return QGCMapEngine::bigSizeToString(_savedTileSize);
}

//-----------------------------------------------------------------------------
QString
QGCCachedTileSet::downloadStatus()
{
    if(_defaultSet) {
        return totalTilesSizeStr();
    }
    if(_totalTileCount <= _savedTileCount) {
        return savedTileSizeStr();
    } else {
        return savedTileSizeStr() + " / " + totalTilesSizeStr();
    }
}

//-----------------------------------------------------------------------------
void
QGCCachedTileSet::createDownloadTask()
{
    if(!_downloading) {
        _errorCount   = 0;
        _downloading  = true;
        _noMoreTiles  = false;
        emit downloadingChanged();
        emit errorCountChanged();
    }
    QGCGetTileDownloadListTask* task = new QGCGetTileDownloadListTask(_id, TILE_BATCH_SIZE);
    connect(task, &QGCGetTileDownloadListTask::tileListFetched, this, &QGCCachedTileSet::_tileListFetched);
    if(_manager)
        connect(task, &QGCMapTask::error, _manager, &QGCMapEngineManager::taskError);
    getQGCMapEngine()->addTask(task);
    emit totalTileCountChanged();
    emit totalTilesSizeChanged();
    _batchRequested = true;
}

//-----------------------------------------------------------------------------
void
QGCCachedTileSet::resumeDownloadTask()
{
    //-- Reset and download error flag (for all tiles)
    QGCUpdateTileDownloadStateTask* task = new QGCUpdateTileDownloadStateTask(_id, QGCTile::StatePending, "*");
    getQGCMapEngine()->addTask(task);
    //-- Start download
    createDownloadTask();
}

//-----------------------------------------------------------------------------
void
QGCCachedTileSet::cancelDownloadTask()
{
    if(_downloading) {
        _downloading = false;
        emit downloadingChanged();
    }
}

//-----------------------------------------------------------------------------
void
QGCCachedTileSet::_tileListFetched(QList<QGCTile *> tiles)
{
    _batchRequested = false;
    //-- Done?
    if(tiles.size() < TILE_BATCH_SIZE) {
        _noMoreTiles = true;
    }
    if(!tiles.size()) {
        _doneWithDownload();
        return;
    }
    //-- If this is the first time, create Network Manager
    if (!_networkManager) {
        _networkManager = new QNetworkAccessManager(this);
    }
    //-- Add tiles to the list
    _tilesToDownload += tiles;
    //-- Kick downloads
    _prepareDownload();
}

//-----------------------------------------------------------------------------
void QGCCachedTileSet::_doneWithDownload()
{
    if(!_errorCount) {
        _totalTileCount = _savedTileCount;
        _totalTileSize  = _savedTileSize;
        //-- Too expensive to compute the real size now. Estimate it for the time being.
        quint32 avg;
        if(_savedTileSize != 0){
            avg = _savedTileSize / _savedTileCount;
        }
        else{
            qWarning() << "QGCMapEngineManager::_doneWithDownload _savedTileSize=0 !";
            avg = 0;
        }

        _uniqueTileSize = _uniqueTileCount * avg;
    }
    emit totalTileCountChanged();
    emit totalTilesSizeChanged();
    emit savedTileSizeChanged();
    emit savedTileCountChanged();
    emit uniqueTileSizeChanged();
    _downloading = false;
    emit downloadingChanged();
    emit completeChanged();
}

//-----------------------------------------------------------------------------
void QGCCachedTileSet::_prepareDownload()
{
    if(!_tilesToDownload.count()) {
        //-- Are we done?
        if(_noMoreTiles) {
            _doneWithDownload();
        } else {
            if(!_batchRequested)
                createDownloadTask();
        }
        return;
    }
    //-- Prepare queue (QNetworkAccessManager has a limit for concurrent downloads)
    for(int i = _replies.count(); i < QGCMapEngine::concurrentDownloads(_type); i++) {
        if(_tilesToDownload.count()) {
            QGCTile* tile = _tilesToDownload.first();
            _tilesToDownload.removeFirst();
            QNetworkRequest request = getQGCMapEngine()->urlFactory()->getTileURL(tile->type(), tile->x(), tile->y(), tile->z(), _networkManager);
            request.setAttribute(QNetworkRequest::User, tile->hash());
#if !defined(__mobile__)
            QNetworkProxy proxy = _networkManager->proxy();
            QNetworkProxy tProxy;
            tProxy.setType(QNetworkProxy::DefaultProxy);
            _networkManager->setProxy(tProxy);
#endif
            QNetworkReply* reply = _networkManager->get(request);
            reply->setParent(0);
            connect(reply, &QNetworkReply::finished, this, &QGCCachedTileSet::_networkReplyFinished);
            connect(reply, &QNetworkReply::errorOccurred, this, &QGCCachedTileSet::_networkReplyError);
            _replies.insert(tile->hash(), reply);
#if !defined(__mobile__)
            _networkManager->setProxy(proxy);
#endif
            delete tile;
            //-- Refill queue if running low
            if(!_batchRequested && !_noMoreTiles && _tilesToDownload.count() < (QGCMapEngine::concurrentDownloads(_type) * 10)) {
                //-- Request new batch of tiles
                createDownloadTask();
            }
        }
    }
}

//-----------------------------------------------------------------------------
void
QGCCachedTileSet::_networkReplyFinished()
{
    //-- Figure out which reply this is
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(QObject::sender());
    if(!reply) {
        qWarning() << "QGCMapEngineManager::networkReplyFinished() NULL Reply";
        return;
    }
    if (reply->error() == QNetworkReply::NoError) {
        //-- Get tile hash
        const QString hash = reply->request().attribute(QNetworkRequest::User).toString();
        if(!hash.isEmpty()) {
            if(_replies.contains(hash)) {
                _replies.remove(hash);
            } else {
                qWarning() << "QGCMapEngineManager::networkReplyFinished() Reply not in list: " << hash;
            }
            qCDebug(QGCCachedTileSetLog) << "Tile fetched" << hash;
            QByteArray image = reply->readAll();
            QString type = getQGCMapEngine()->hashToType(hash);
            if (type == "Airmap Elevation" ) {
                image = TerrainTile::serializeFromAirMapJson(image);
            }
            QString format = getQGCMapEngine()->urlFactory()->getImageFormat(type, image);
            if(!format.isEmpty()) {
                //-- Cache tile
                getQGCMapEngine()->cacheTile(type, hash, image, format, _id);
                QGCUpdateTileDownloadStateTask* task = new QGCUpdateTileDownloadStateTask(_id, QGCTile::StateComplete, hash);
                getQGCMapEngine()->addTask(task);
                //-- Updated cached (downloaded) data
                _savedTileSize += image.size();
                _savedTileCount++;
                emit savedTileSizeChanged();
                emit savedTileCountChanged();
                //-- Update estimate
                if(_savedTileCount % 10 == 0) {
                    quint32 avg = _savedTileSize / _savedTileCount;
                    _totalTileSize  = avg * _totalTileCount;
                    _uniqueTileSize = avg * _uniqueTileCount;
                    emit totalTilesSizeChanged();
                    emit uniqueTileSizeChanged();
                }
            }
            //-- Setup a new download
            _prepareDownload();
        } else {
            qWarning() << "QGCMapEngineManager::networkReplyFinished() Empty Hash";
        }
    }
    reply->deleteLater();
}

//-----------------------------------------------------------------------------
void
QGCCachedTileSet::_networkReplyError(QNetworkReply::NetworkError error)
{
    //-- Figure out which reply this is
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(QObject::sender());
    if (!reply) {
        return;
    }
    //-- Update error count
    _errorCount++;
    emit errorCountChanged();
    //-- Get tile hash
    QString hash = reply->request().attribute(QNetworkRequest::User).toString();
    qCDebug(QGCCachedTileSetLog) << "Error fetching tile" << reply->errorString();
    if(!hash.isEmpty()) {
        if(_replies.contains(hash)) {
            _replies.remove(hash);
        } else {
            qWarning() << "QGCMapEngineManager::networkReplyError() Reply not in list: " << hash;
        }
        if (error != QNetworkReply::OperationCanceledError) {
            qWarning() << "QGCMapEngineManager::networkReplyError() Error:" << reply->errorString();
        }
        QGCUpdateTileDownloadStateTask* task = new QGCUpdateTileDownloadStateTask(_id, QGCTile::StateError, hash);
        getQGCMapEngine()->addTask(task);
    } else {
        qWarning() << "QGCMapEngineManager::networkReplyError() Empty Hash";
    }
    //-- Setup a new download
    _prepareDownload();
    reply->deleteLater();
}

//-----------------------------------------------------------------------------
void
QGCCachedTileSet::setManager(QGCMapEngineManager* mgr)
{
    _manager = mgr;
}

//-----------------------------------------------------------------------------
void
QGCCachedTileSet::setSelected(bool sel)
{
    _selected = sel;
    emit selectedChanged();
    if(_manager) {
        emit _manager->selectedCountChanged();
    }
}
