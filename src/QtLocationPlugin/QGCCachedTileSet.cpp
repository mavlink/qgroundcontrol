/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCCachedTileSet.h"

#include <QtNetwork/QNetworkProxy>

#include "ElevationMapProvider.h"
#include "QGCApplication.h"
#include "QGCFileDownload.h"
#include "QGCLoggingCategory.h"
#include "QGCMapEngine.h"
#include "QGCMapEngineManager.h"
#include "QGCMapTasks.h"
#include "QGCMapUrlEngine.h"
#include "QGeoFileTileCacheQGC.h"
#include "QGeoTileFetcherQGC.h"

QGC_LOGGING_CATEGORY(QGCCachedTileSetLog, "QtLocationPlugin.QGCCachedTileSet")

QGCCachedTileSet::QGCCachedTileSet(const QString &name, QObject *parent)
    : QObject(parent)
    , _name(name)
{
    qCDebug(QGCCachedTileSetLog) << this;
}

QGCCachedTileSet::~QGCCachedTileSet()
{
    qCDebug(QGCCachedTileSetLog) << this;
}

qreal QGCCachedTileSet::downloadProgress() const
{
    if (_defaultSet || (_totalTileCount == 0)) {
        return 0.0;
    }

    // Calculate progress as percentage (0.0 - 1.0)
    // Progress = downloaded / total
    return static_cast<qreal>(_savedTileCount) / static_cast<qreal>(_totalTileCount);
}

QString QGCCachedTileSet::downloadStatus() const
{
    if (_defaultSet) {
        return totalTilesSizeStr();
    }

    if (_totalTileCount <= _savedTileCount) {
        return savedTileSizeStr();
    }

    return (savedTileSizeStr() + " / " + totalTilesSizeStr());
}

void QGCCachedTileSet::createDownloadTask()
{
    if (_cancelPending) {
        setDownloading(false);
        return;
    }

    if (!_downloading) {
        setErrorCount(0);
        setDownloading(true);
        _noMoreTiles = false;
    }

    QGCGetTileDownloadListTask *task = new QGCGetTileDownloadListTask(_id, kTileBatchSize);
    (void) connect(task, &QGCGetTileDownloadListTask::tileListFetched, this, &QGCCachedTileSet::_tileListFetched);
    if (_manager) {
        (void) connect(task, &QGCMapTask::error, _manager, &QGCMapEngineManager::taskError);
    }
    if (!getQGCMapEngine()->addTask(task)) {
        task->deleteLater();
    }

    emit totalTileCountChanged();
    emit totalTilesSizeChanged();

    _batchRequested = true;
}

void QGCCachedTileSet::resumeDownloadTask()
{
    _cancelPending = false;

    QGCUpdateTileDownloadStateTask *task = new QGCUpdateTileDownloadStateTask(_id, QGCTile::StatePending, "*");
    if (!getQGCMapEngine()->addTask(task)) {
        task->deleteLater();
    }

    createDownloadTask();
}

void QGCCachedTileSet::pauseDownloadTask()
{
    _cancelPending = true;

    // Abort any in-flight network replies so they don't continue downloading
    {
        QMutexLocker lock(&_repliesMutex);
        for (auto replyIt = _replies.begin(); replyIt != _replies.end(); ++replyIt) {
            if (QNetworkReply *reply = replyIt.value()) {
                reply->blockSignals(true);
                reply->abort();
                reply->deleteLater();
            }
        }
        _replies.clear();
    }

    // Drop any queued tiles which haven't started downloading yet
    qDeleteAll(_tilesToDownload);
    _tilesToDownload.clear();

    // Mark all tiles in this set as pending again so a future resume works
    QGCUpdateTileDownloadStateTask *task = new QGCUpdateTileDownloadStateTask(_id, QGCTile::StatePending, "*");
    if (!getQGCMapEngine()->addTask(task)) {
        task->deleteLater();
    }

    setDownloading(false);
}

void QGCCachedTileSet::cancelDownloadTask()
{
    pauseDownloadTask();
}

void QGCCachedTileSet::copyFrom(const QGCCachedTileSet *other)
{
    if (!other || other == this) {
        return;
    }

    setName(other->name());
    setMapTypeStr(other->mapTypeStr());
    setType(other->type());
    setTopleftLat(other->topleftLat());
    setTopleftLon(other->topleftLon());
    setBottomRightLat(other->bottomRightLat());
    setBottomRightLon(other->bottomRightLon());
    setMinZoom(other->minZoom());
    setMaxZoom(other->maxZoom());
    setCreationDate(other->creationDate());
    setId(other->id());
    setDefaultSet(other->defaultSet());
    setDeleting(other->deleting());
    setDownloading(other->downloading());
    setErrorCount(other->errorCount());
    setSelected(other->selected());
    setUniqueTileCount(other->uniqueTileCount());
    setUniqueTileSize(other->uniqueTileSize());
    setTotalTileCount(other->totalTileCount());
    setTotalTileSize(other->totalTilesSize());
    setSavedTileCount(other->savedTileCount());
    setSavedTileSize(other->savedTileSize());
    setDownloadStats(other->pendingTiles(), other->downloadingTiles(), other->errorTiles());
}

void QGCCachedTileSet::_tileListFetched(const QQueue<QGCTile*> &tiles)
{
    _batchRequested = false;
    if (tiles.size() < kTileBatchSize) {
        _noMoreTiles = true;
    }

    if (tiles.isEmpty()) {
        _doneWithDownload();
        return;
    }

    if (!_networkManager) {
        _networkManager = new QNetworkAccessManager(this);
#if !defined(Q_OS_IOS) && !defined(Q_OS_ANDROID)
        QNetworkProxy proxy = _networkManager->proxy();
        proxy.setType(QNetworkProxy::DefaultProxy);
        _networkManager->setProxy(proxy);
#endif
    }

    _tilesToDownload.append(tiles);
    _prepareDownload();
}

void QGCCachedTileSet::_doneWithDownload()
{
    if (_errorCount == 0) {
        setTotalTileCount(_savedTileCount);
        setTotalTileSize(_savedTileSize);

        quint32 avg = 0;
        if (_savedTileCount != 0) {
            avg = _savedTileSize / _savedTileCount;
        } else {
            qCWarning(QGCCachedTileSetLog) << "_savedTileCount=0";
        }

        setUniqueTileSize(_uniqueTileCount * avg);
    }

    setDownloading(false);

    emit completeChanged();
}

void QGCCachedTileSet::_prepareDownload()
{
    if (_cancelPending) {
        // Ensure queued tiles are dropped and no new requests are scheduled
        qDeleteAll(_tilesToDownload);
        _tilesToDownload.clear();
        return;
    }

    if (_tilesToDownload.isEmpty()) {
        if (_noMoreTiles) {
            _doneWithDownload();
        } else if (!_batchRequested) {
            createDownloadTask();
        }
        return;
    }

    if (!_networkManager) {
        qCWarning(QGCCachedTileSetLog) << "Network manager unavailable, delaying download";
        createDownloadTask();
        return;
    }

    const qsizetype maxConcurrent = QGeoTileFetcherQGC::concurrentDownloads(_type);

    for (qsizetype i = 0; i < maxConcurrent; i++) {
        if (_tilesToDownload.isEmpty()) {
            break;
        }

        // Check if we've reached the concurrent limit
        {
            QMutexLocker lock(&_repliesMutex);
            if (_replies.count() >= maxConcurrent) {
                break;
            }
        }

        // Dequeue and prepare tile request without holding the mutex
        QGCTile* const tile = _tilesToDownload.dequeue();
        const int mapId = UrlFactory::getQtMapIdFromProviderType(tile->type);
        QNetworkRequest request = QGeoTileFetcherQGC::getNetworkRequest(mapId, tile->x, tile->y, tile->z);
        request.setOriginatingObject(this);
        request.setAttribute(QNetworkRequest::User, tile->hash);

        QNetworkReply* const reply = _networkManager->get(request);
        reply->setParent(this);
        QGCFileDownload::setIgnoreSSLErrorsIfNeeded(*reply);
        (void) connect(reply, &QNetworkReply::finished, this, &QGCCachedTileSet::_networkReplyFinished);
        (void) connect(reply, &QNetworkReply::errorOccurred, this, &QGCCachedTileSet::_networkReplyError);

        // Insert into replies map
        {
            QMutexLocker lock(&_repliesMutex);
            (void) _replies.insert(tile->hash, reply);
        }

        delete tile;
        if (!_batchRequested && !_noMoreTiles && (_tilesToDownload.count() < (maxConcurrent * 10))) {
            createDownloadTask();
        }
    }
}

void QGCCachedTileSet::_networkReplyFinished()
{
    QNetworkReply* const reply = qobject_cast<QNetworkReply*>(QObject::sender());
    if (!reply) {
        qCWarning(QGCCachedTileSetLog) << "NULL Reply";
        return;
    }
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        return;
    }

    if (!reply->isOpen()) {
        qCWarning(QGCCachedTileSetLog) << "Empty Reply";
        return;
    }

    const QString hash = reply->request().attribute(QNetworkRequest::User).toString();
    if (hash.isEmpty()) {
        qCWarning(QGCCachedTileSetLog) << "Empty Hash";
        return;
    }

    {
        QMutexLocker lock(&_repliesMutex);
        if (_replies.contains(hash)) {
            (void) _replies.remove(hash);
        } else {
            qCWarning(QGCCachedTileSetLog) << "Reply not in list: " << hash;
        }
    }
    qCDebug(QGCCachedTileSetLog) << "Tile fetched:" << hash;

    QByteArray image = reply->readAll();
    if (image.isEmpty()) {
        qCWarning(QGCCachedTileSetLog) << "Empty Image";
        return;
    }

    const QString type = UrlFactory::tileHashToType(hash);
    const SharedMapProvider mapProvider = UrlFactory::getMapProviderFromProviderType(type);
    if (!mapProvider) {
        qCWarning(QGCCachedTileSetLog) << "Invalid map provider for type:" << type << "hash:" << hash;
        setErrorCount(_errorCount + 1);
        QGCUpdateTileDownloadStateTask *task = new QGCUpdateTileDownloadStateTask(_id, QGCTile::StateError, hash);
        if (!getQGCMapEngine()->addTask(task)) {
            task->deleteLater();
        }
        _prepareDownload();
        return;
    }

    if (mapProvider->isElevationProvider()) {
        const SharedElevationProvider elevationProvider = std::dynamic_pointer_cast<const ElevationProvider>(mapProvider);
        image = elevationProvider->serialize(image);
        if (image.isEmpty()) {
            qCWarning(QGCCachedTileSetLog) << "Failed to Serialize Terrain Tile";
            return;
        }
    }

    const QString format = mapProvider->getImageFormat(image);
    if (format.isEmpty()) {
        qCWarning(QGCCachedTileSetLog) << "Empty Format";
        return;
    }

    QGeoFileTileCacheQGC::cacheTile(type, hash, image, format, _id);

    QGCUpdateTileDownloadStateTask *task = new QGCUpdateTileDownloadStateTask(_id, QGCTile::StateComplete, hash);
    if (!getQGCMapEngine()->addTask(task)) {
        task->deleteLater();
    }

    setSavedTileSize(_savedTileSize + image.size());
    setSavedTileCount(_savedTileCount + 1);

    if (_savedTileCount % kSizeEstimateInterval == 0) {
        const quint32 avg = _savedTileSize / _savedTileCount;
        setTotalTileSize(avg * _totalTileCount);
        setUniqueTileSize(avg * _uniqueTileCount);
    }

    _prepareDownload();
}

void QGCCachedTileSet::_networkReplyError(QNetworkReply::NetworkError error)
{
    QNetworkReply* const reply = qobject_cast<QNetworkReply*>(QObject::sender());
    if (!reply) {
        return;
    }
    qCDebug(QGCCachedTileSetLog) << "Error fetching tile" << reply->errorString();

    const bool cancelled = (_cancelPending && (error == QNetworkReply::OperationCanceledError));
    if (!cancelled) {
        setErrorCount(_errorCount + 1);
    }

    const QString hash = reply->request().attribute(QNetworkRequest::User).toString();
    if (hash.isEmpty()) {
        qCWarning(QGCCachedTileSetLog) << "Empty Hash";
        return;
    }

    {
        QMutexLocker lock(&_repliesMutex);
        if (_replies.contains(hash)) {
            (void) _replies.remove(hash);
        } else {
            qCWarning(QGCCachedTileSetLog) << "Reply not in list:" << hash;
        }
    }

    if (!cancelled && (error != QNetworkReply::OperationCanceledError)) {
        qCWarning(QGCCachedTileSetLog) << "Error:" << reply->errorString();
    }

    const QGCTile::TileState newState = cancelled ? QGCTile::StatePaused : QGCTile::StateError;
    QGCUpdateTileDownloadStateTask *task = new QGCUpdateTileDownloadStateTask(_id, newState, hash);
    if (!getQGCMapEngine()->addTask(task)) {
        task->deleteLater();
    }

    if (!cancelled) {
        _prepareDownload();
    }
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

void QGCCachedTileSet::setTotalTileCount(quint32 num)
{
    const bool wasComplete = complete();
    if (num != _totalTileCount) {
        _totalTileCount = num;
        emit totalTileCountChanged();
    }
    if (wasComplete != complete()) {
        emit completeChanged();
    }
}

void QGCCachedTileSet::setSavedTileCount(quint32 num)
{
    const bool wasComplete = complete();
    if (num != _savedTileCount) {
        _savedTileCount = num;
        emit savedTileCountChanged();
    }
    if (wasComplete != complete()) {
        emit completeChanged();
    }
}

void QGCCachedTileSet::setDefaultSet(bool def)
{
    const bool wasComplete = complete();
    _defaultSet = def;
    if (wasComplete != complete()) {
        emit completeChanged();
    }
}

void QGCCachedTileSet::setDownloadStats(quint32 pending, quint32 downloading, quint32 errors)
{
    if ((pending == _pendingTiles) && (downloading == _downloadingTiles) && (errors == _errorTiles)) {
        return;
    }
    _pendingTiles = pending;
    _downloadingTiles = downloading;
    _errorTiles = errors;
    emit downloadStatsChanged();
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
