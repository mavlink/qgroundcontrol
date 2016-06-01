/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief Map Tile Set
 *
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

#include "QGCMapEngine.h"
#include "QGCMapTileSet.h"
#include "QGCMapEngineManager.h"

#include <QSettings>
#include <math.h>

QGC_LOGGING_CATEGORY(QGCCachedTileSetLog, "QGCCachedTileSetLog")

#define TILE_BATCH_SIZE      256

//-----------------------------------------------------------------------------
QGCCachedTileSet::QGCCachedTileSet(const QString& name, const QString& description)
    : _name(name)
    , _description(description)
    , _topleftLat(0.0)
    , _topleftLon(0.0)
    , _bottomRightLat(0.0)
    , _bottomRightLon(0.0)
    , _numTiles(0)
    , _tilesSize(0)
    , _savedTiles(0)
    , _savedSize(0)
    , _minZoom(3)
    , _maxZoom(3)
    , _defaultSet(false)
    , _deleting(false)
    , _downloading(false)
    , _id(0)
    , _type(UrlFactory::Invalid)
    , _networkManager(NULL)
    , _errorCount(0)
    , _noMoreTiles(false)
    , _batchRequested(false)
    , _manager(NULL)
{

}

//-----------------------------------------------------------------------------
QGCCachedTileSet::~QGCCachedTileSet()
{
    if(_networkManager) {
        delete _networkManager;
    }
}

//-----------------------------------------------------------------------------
QString
QGCCachedTileSet::errorCountStr()
{
    return QGCMapEngine::numberToString(_errorCount);
}

//-----------------------------------------------------------------------------
QString
QGCCachedTileSet::numTilesStr()
{
    return QGCMapEngine::numberToString(_numTiles);
}

//-----------------------------------------------------------------------------
QString
QGCCachedTileSet::tilesSizeStr()
{
    return QGCMapEngine::bigSizeToString(_tilesSize);
}

//-----------------------------------------------------------------------------
QString
QGCCachedTileSet::savedTilesStr()
{
    return QGCMapEngine::numberToString(_savedTiles);
}

//-----------------------------------------------------------------------------
QString
QGCCachedTileSet::savedSizeStr()
{
    return QGCMapEngine::bigSizeToString(_savedSize);
}

//-----------------------------------------------------------------------------
QString
QGCCachedTileSet::downloadStatus()
{
    if(_defaultSet) {
        return tilesSizeStr();
    }
    if(_numTiles == _savedTiles) {
        return savedSizeStr();
    } else {
        return savedSizeStr() + " / " + tilesSizeStr();
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
    emit numTilesChanged();
    emit tilesSizeChanged();
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
void QGCCachedTileSet::_prepareDownload()
{
    if(!_tilesToDownload.count()) {
        //-- Are we done?
        if(_noMoreTiles) {
            if(!_errorCount) {
                _numTiles  = _savedTiles;
                _tilesSize = _savedSize;
            }
            emit numTilesChanged();
            emit tilesSizeChanged();
            emit savedSizeChanged();
            emit savedTilesChanged();
            _downloading = false;
            emit downloadingChanged();
            emit completeChanged();
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
            QNetworkReply* reply = _networkManager->get(request);
            reply->setParent(0);
            connect(reply, &QNetworkReply::finished, this, &QGCCachedTileSet::_networkReplyFinished);
            connect(reply, static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error), this, &QGCCachedTileSet::_networkReplyError);
            _replies.insert(tile->hash(), reply);
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
    //-- Get tile hash
    const QString hash = reply->request().attribute(QNetworkRequest::User).toString();
    if(!hash.isEmpty()) {
        if(_replies.contains(hash)) {
            _replies.remove(hash);
        } else {
            qWarning() << "QGCMapEngineManager::networkReplyFinished() Reply not in list: " << hash;
        }
        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "QGCMapEngineManager::networkReplyFinished() Error:" << reply->errorString();
            return;
        }
        qCDebug(QGCCachedTileSetLog) << "Tile fetched" << hash;
        QByteArray image = reply->readAll();
        UrlFactory::MapType type = getQGCMapEngine()->hashToType(hash);
        QString format = getQGCMapEngine()->urlFactory()->getImageFormat(type, image);
        if(!format.isEmpty()) {
            //-- Cache tile
            getQGCMapEngine()->cacheTile(type, hash, image, format, _id);
            QGCUpdateTileDownloadStateTask* task = new QGCUpdateTileDownloadStateTask(_id, QGCTile::StateComplete, hash);
            getQGCMapEngine()->addTask(task);
            //-- Updated cached (downloaded) data
            _savedSize += image.size();
            _savedTiles++;
            emit savedSizeChanged();
            emit savedTilesChanged();
            //-- Update estimate
            if(_savedTiles % 10 == 0) {
                quint32 avg = _savedSize / _savedTiles;
                _tilesSize = avg * _numTiles;
                emit tilesSizeChanged();
            }
        }
        //-- Setup a new download
        _prepareDownload();
    } else {
        qWarning() << "QGCMapEngineManager::networkReplyFinished() Empty Hash";
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
    //-- Upodate error count
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
