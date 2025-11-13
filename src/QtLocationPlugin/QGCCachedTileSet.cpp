#include "QGCCachedTileSet.h"

#include "ElevationMapProvider.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "QGCMapEngine.h"
#include "QGCMapEngineManager.h"
#include "QGCNetworkHelper.h"
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

    _stopDownload = true;

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

    {
        QMutexLocker queueLock(&_queueMutex);
        qDeleteAll(_tilesToDownload);
        _tilesToDownload.clear();
    }
}

qreal QGCCachedTileSet::downloadProgress() const
{
    if (_defaultSet || (_totalTileCount == 0)) {
        return 0.0;
    }

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
    if (_stopDownload) {
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

void QGCCachedTileSet::resumeDownloadTask(bool systemResume)
{
    if (systemResume && !_pausedBySystem) {
        return;
    }

    if (systemResume) {
        _pausedBySystem = false;
    }
    _stopDownload = false;

    QGCUpdateTileDownloadStateTask *task = new QGCUpdateTileDownloadStateTask(_id, QGCTile::StatePending, "*");
    if (!getQGCMapEngine()->addTask(task)) {
        task->deleteLater();
    }

    createDownloadTask();
}

void QGCCachedTileSet::retryFailedTiles()
{
    // Don't retry if already downloading (unless paused)
    if (_downloading && !_stopDownload) {
        qCWarning(QGCCachedTileSetLog) << "Retry requested during active download - ignoring";
        return;
    }

    _stopDownload = false;

    // Mark all tiles in StateError as StatePending to retry them
    QGCUpdateTileDownloadStateTask *task = new QGCUpdateTileDownloadStateTask(_id, QGCTile::StatePending, "*", QGCTile::StateError);
    if (!getQGCMapEngine()->addTask(task)) {
        task->deleteLater();
        qCWarning(QGCCachedTileSetLog) << "Failed to queue retry task for tile set" << _id;
        return;
    }

    // Reset error count only after task is successfully queued
    setErrorCount(0);

    // Resume download to pick up the failed tiles
    createDownloadTask();
}

void QGCCachedTileSet::pauseDownloadTask(bool systemPause)
{
    if (systemPause) {
        _pausedBySystem = true;
    }
    // Note: User pause does not clear _pausedBySystem flag
    // This allows system resume to work even after user pause/resume

    _stopDownload = true;

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
    {
        QMutexLocker queueLock(&_queueMutex);
        qDeleteAll(_tilesToDownload);
        _tilesToDownload.clear();
    }

    // Mark all tiles in this set as pending again so a future resume works
    QGCUpdateTileDownloadStateTask *task = new QGCUpdateTileDownloadStateTask(_id, QGCTile::StatePending, "*");
    if (!getQGCMapEngine()->addTask(task)) {
        task->deleteLater();
    }

    setDownloading(false);
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
        QGCNetworkHelper::configureProxy(_networkManager);
    }

    {
        QMutexLocker queueLock(&_queueMutex);
        _tilesToDownload.append(tiles);
    }
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
    QMutexLocker prepareLock(&_prepareDownloadMutex);

    if (_stopDownload) {
        QMutexLocker queueLock(&_queueMutex);
        qDeleteAll(_tilesToDownload);
        _tilesToDownload.clear();
        return;
    }

    bool queueEmpty;
    {
        QMutexLocker queueLock(&_queueMutex);
        queueEmpty = _tilesToDownload.isEmpty();
    }

    if (queueEmpty) {
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
        {
            QMutexLocker lock(&_repliesMutex);
            if (_replies.count() >= maxConcurrent) {
                break;
            }
        }

        QGCTile* tile = nullptr;
        qsizetype queueCount = 0;
        {
            QMutexLocker queueLock(&_queueMutex);
            if (_tilesToDownload.isEmpty()) {
                break;
            }
            tile = _tilesToDownload.dequeue();
            queueCount = _tilesToDownload.count();
        }

        const int mapId = UrlFactory::getQtMapIdFromProviderType(tile->type);
        QNetworkRequest request = QGeoTileFetcherQGC::getNetworkRequest(mapId, tile->x, tile->y, tile->z);
        request.setOriginatingObject(this);
        request.setAttribute(QNetworkRequest::User, tile->hash);

        QNetworkReply* const reply = _networkManager->get(request);
        reply->setParent(this);
        QGCNetworkHelper::ignoreSslErrorsIfNeeded(reply);
        (void) connect(reply, &QNetworkReply::finished, this, &QGCCachedTileSet::_networkReplyFinished);
        (void) connect(reply, &QNetworkReply::errorOccurred, this, &QGCCachedTileSet::_networkReplyError);

        {
            QMutexLocker lock(&_repliesMutex);
            (void) _replies.insert(tile->hash, reply);
        }

        delete tile;
        if (!_batchRequested && !_noMoreTiles && (queueCount < (maxConcurrent * 10))) {
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
        qCWarning(QGCCachedTileSetLog) << "Empty Image for hash:" << hash;
        setErrorCount(_errorCount + 1);
        QGCUpdateTileDownloadStateTask *task = new QGCUpdateTileDownloadStateTask(_id, QGCTile::StateError, hash);
        if (!getQGCMapEngine()->addTask(task)) {
            task->deleteLater();
        }
        _prepareDownload();
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
        if (!elevationProvider) {
            qCWarning(QGCCachedTileSetLog) << "Failed to cast to ElevationProvider for hash:" << hash;
            setErrorCount(_errorCount + 1);
            QGCUpdateTileDownloadStateTask *task = new QGCUpdateTileDownloadStateTask(_id, QGCTile::StateError, hash);
            if (!getQGCMapEngine()->addTask(task)) {
                task->deleteLater();
            }
            _prepareDownload();
            return;
        }
        image = elevationProvider->serialize(image);
        if (image.isEmpty()) {
            qCWarning(QGCCachedTileSetLog) << "Failed to Serialize Terrain Tile for hash:" << hash;
            setErrorCount(_errorCount + 1);
            QGCUpdateTileDownloadStateTask *task = new QGCUpdateTileDownloadStateTask(_id, QGCTile::StateError, hash);
            if (!getQGCMapEngine()->addTask(task)) {
                task->deleteLater();
            }
            _prepareDownload();
            return;
        }
    }

    const QString format = mapProvider->getImageFormat(image);
    if (format.isEmpty()) {
        qCWarning(QGCCachedTileSetLog) << "Empty Format for hash:" << hash;
        setErrorCount(_errorCount + 1);
        QGCUpdateTileDownloadStateTask *task = new QGCUpdateTileDownloadStateTask(_id, QGCTile::StateError, hash);
        if (!getQGCMapEngine()->addTask(task)) {
            task->deleteLater();
        }
        _prepareDownload();
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

    const bool cancelled = (_stopDownload && (error == QNetworkReply::OperationCanceledError));
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
