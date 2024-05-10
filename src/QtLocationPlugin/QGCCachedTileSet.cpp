/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

// TODO: This is just an alternate QGeoTileFetcher, hook this into mapping manager engine

/**
 * @file
 *   @brief Map Tile Set
 *
 *   @author Gus Grubba <gus@auterion.com>
 *
 */

#include "QGCCachedTileSet.h"
#include "QGCMapEngine.h"
#include "QGCMapEngineManager.h"
#include "QGCFileDownload.h"
#include "QGeoTileFetcherQGC.h"
#include "TerrainTile.h"
#include "QGCMapUrlEngine.h"
#include "QGCMapEngineData.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "ElevationMapProvider.h"
#include "QGeoFileTileCacheQGC.h"

#include <QtNetwork/QNetworkProxy>

QGC_LOGGING_CATEGORY(QGCCachedTileSetLog, "qgc.qtlocation.qgccachedtileset")

#define TILE_BATCH_SIZE 256

QGCCachedTileSet::QGCCachedTileSet(const QString& name, QObject* parent)
    : QObject(parent)
    , _name(name)
{
    qCDebug(QGCCachedTileSetLog) << Q_FUNC_INFO << this;
}

QGCCachedTileSet::~QGCCachedTileSet()
{
    qCDebug(QGCCachedTileSetLog) << Q_FUNC_INFO << this;
}

QString QGCCachedTileSet::errorCountStr() const
{
    return qgcApp()->numberToString(_errorCount);
}

QString QGCCachedTileSet::totalTileCountStr() const
{
    return qgcApp()->numberToString(_totalTileCount);
}

QString QGCCachedTileSet::totalTilesSizeStr() const
{
    return qgcApp()->bigSizeToString(_totalTileSize);
}

QString QGCCachedTileSet::uniqueTileSizeStr() const
{
    return qgcApp()->bigSizeToString(_uniqueTileSize);
}

QString QGCCachedTileSet::uniqueTileCountStr() const
{
    return qgcApp()->numberToString(_uniqueTileCount);
}

QString QGCCachedTileSet::savedTileCountStr() const
{
    return qgcApp()->numberToString(_savedTileCount);
}

QString QGCCachedTileSet::savedTileSizeStr() const
{
    return qgcApp()->bigSizeToString(_savedTileSize);
}

QString QGCCachedTileSet::downloadStatus() const
{
    if (_defaultSet) {
        return totalTilesSizeStr();
    }

    if (_totalTileCount <= _savedTileCount) {
        return savedTileSizeStr();
    }

    return savedTileSizeStr() + " / " + totalTilesSizeStr();
}

void QGCCachedTileSet::createDownloadTask()
{
    if (!_downloading) {
        _errorCount = 0;
        emit errorCountChanged();
        _downloading = true;
        emit downloadingChanged();
        _noMoreTiles = false;
    }

    QGCGetTileDownloadListTask* task = new QGCGetTileDownloadListTask(_id, TILE_BATCH_SIZE);
    (void) connect(task, &QGCGetTileDownloadListTask::tileListFetched, this, &QGCCachedTileSet::_tileListFetched);
    if (_manager) {
        (void) connect(task, &QGCMapTask::error, _manager, &QGCMapEngineManager::taskError);
    }
    getQGCMapEngine()->addTask(task);

    emit totalTileCountChanged();
    emit totalTilesSizeChanged();

    _batchRequested = true;
}

void QGCCachedTileSet::resumeDownloadTask()
{
    QGCUpdateTileDownloadStateTask* task = new QGCUpdateTileDownloadStateTask(_id, QGCTile::StatePending, "*");
    getQGCMapEngine()->addTask(task);
    createDownloadTask();
}

void QGCCachedTileSet::cancelDownloadTask()
{
    if (_downloading) {
        _downloading = false;
        emit downloadingChanged();
    }
}

void QGCCachedTileSet::_tileListFetched(QList<QGCTile *> tiles)
{
    _batchRequested = false;
    if (tiles.size() < TILE_BATCH_SIZE) {
        _noMoreTiles = true;
    }

    if (!tiles.size()) {
        _doneWithDownload();
        return;
    }

    if (!_networkManager) {
        _networkManager = new QNetworkAccessManager(this);
    }

    _tilesToDownload += tiles;
    _prepareDownload();
}

void QGCCachedTileSet::_doneWithDownload()
{
    if (!_errorCount) {
        _totalTileCount = _savedTileCount;
        emit totalTileCountChanged();
        _totalTileSize  = _savedTileSize;
        emit totalTilesSizeChanged();

        quint32 avg = 0;
        if (_savedTileSize != 0) {
            avg = _savedTileSize / _savedTileCount;
        } else {
            qCWarning(QGCCachedTileSetLog) << Q_FUNC_INFO << "_savedTileSize=0";
        }

        _uniqueTileSize = _uniqueTileCount * avg;
        emit uniqueTileSizeChanged();
    }

    _downloading = false;
    emit downloadingChanged();
    emit completeChanged();
}

void QGCCachedTileSet::_prepareDownload()
{
    if (!_tilesToDownload.count()) {
        if (_noMoreTiles) {
            _doneWithDownload();
        } else if (!_batchRequested) {
            createDownloadTask();
        }
        return;
    }

    for (size_t i = _replies.count(); i < QGeoTileFetcherQGC::concurrentDownloads(_type); i++) {
        if (!_tilesToDownload.count()) {
            continue;
        }

        QGCTile* const tile = _tilesToDownload.takeFirst();
        const int mapId = UrlFactory::getQtMapIdFromProviderType(tile->type());

        QNetworkRequest request = QGeoTileFetcherQGC::getNetworkRequest(mapId, tile->x(), tile->y(), tile->z());
        request.setOriginatingObject(this);
        request.setAttribute(QNetworkRequest::User, tile->hash());

#if !defined(__mobile__)
        const QNetworkProxy proxy = _networkManager->proxy();
        QNetworkProxy tempProxy;
        tempProxy.setType(QNetworkProxy::DefaultProxy);
        _networkManager->setProxy(tempProxy);
#endif

        QNetworkReply* const reply = _networkManager->get(request);
        reply->setParent(this);
        QGCFileDownload::setIgnoreSSLErrorsIfNeeded(*reply);
        (void) connect(reply, &QNetworkReply::finished, this, &QGCCachedTileSet::_networkReplyFinished);
        (void) connect(reply, &QNetworkReply::errorOccurred, this, &QGCCachedTileSet::_networkReplyError);
        (void) _replies.insert(tile->hash(), reply);

#if !defined(__mobile__)
        _networkManager->setProxy(proxy);
#endif

        delete tile;
        if (!_batchRequested && !_noMoreTiles && (_tilesToDownload.count() < (QGeoTileFetcherQGC::concurrentDownloads(_type) * 10))) {
            createDownloadTask();
        }
    }
}

void QGCCachedTileSet::_networkReplyFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(QObject::sender());
    if (!reply) {
        qCWarning(QGCCachedTileSetLog) << Q_FUNC_INFO << "NULL Reply";
        return;
    }
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        return;
    }

    const QString hash = reply->request().attribute(QNetworkRequest::User).toString();
    if (hash.isEmpty()) {
        qCWarning(QGCCachedTileSetLog) << Q_FUNC_INFO << "Empty Hash";
        return;
    }

    if (_replies.contains(hash)) {
        _replies.remove(hash);
    } else {
        qCWarning(QGCCachedTileSetLog) << Q_FUNC_INFO << "Reply not in list: " << hash;
    }
    qCDebug(QGCCachedTileSetLog) << "Tile fetched" << hash;

    QByteArray image = reply->readAll();
    const QString type = UrlFactory::tileHashToType(hash);
    if (type == CopernicusElevationProvider::kProviderKey) {
        image = TerrainTile::serializeFromAirMapJson(image);
    }

    const QString format = UrlFactory::getImageFormat(type, image);
    if (!format.isEmpty()) {
        QGeoFileTileCacheQGC::cacheTile(type, hash, image, format, _id);

        QGCUpdateTileDownloadStateTask* task = new QGCUpdateTileDownloadStateTask(_id, QGCTile::StateComplete, hash);
        getQGCMapEngine()->addTask(task);

        _savedTileSize += image.size();
        emit savedTileSizeChanged();
        _savedTileCount++;
        emit savedTileCountChanged();

        if (_savedTileCount % 10 == 0) {
            const quint32 avg = _savedTileSize / _savedTileCount;
            _totalTileSize = avg * _totalTileCount;
            emit totalTilesSizeChanged();
            _uniqueTileSize = avg * _uniqueTileCount;
            emit uniqueTileSizeChanged();
        }
    }

    _prepareDownload();
}

void QGCCachedTileSet::_networkReplyError(QNetworkReply::NetworkError error)
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(QObject::sender());
    if (!reply) {
        return;
    }
    reply->deleteLater();
    qCDebug(QGCCachedTileSetLog) << Q_FUNC_INFO << "Error fetching tile" << reply->errorString();

    _errorCount++;
    emit errorCountChanged();

    const QString hash = reply->request().attribute(QNetworkRequest::User).toString();
    if (!hash.isEmpty()) {
        if (_replies.contains(hash)) {
            _replies.remove(hash);
        } else {
            qCWarning(QGCCachedTileSetLog) << Q_FUNC_INFO << "Reply not in list: " << hash;
        }

        if (error != QNetworkReply::OperationCanceledError) {
            qCWarning(QGCCachedTileSetLog) << Q_FUNC_INFO << "Error:" << reply->errorString();
        }

        QGCUpdateTileDownloadStateTask* task = new QGCUpdateTileDownloadStateTask(_id, QGCTile::StateError, hash);
        getQGCMapEngine()->addTask(task);
    } else {
        qCWarning(QGCCachedTileSetLog) << Q_FUNC_INFO << "Empty Hash";
    }

    _prepareDownload();
}

void QGCCachedTileSet::setSelected(bool sel)
{
    if (sel != _selected) {
        _selected = sel;
        emit selectedChanged();
        if (_manager) {
            emit _manager->selectedCountChanged();
        }
    }
}
