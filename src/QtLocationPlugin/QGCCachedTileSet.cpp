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

void QGCCachedTileSet::cancelDownloadTask()
{
    _cancelPending = true;
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

    _tilesToDownload.append(tiles);
    _prepareDownload();
}

void QGCCachedTileSet::_doneWithDownload()
{
    if (_errorCount == 0) {
        setTotalTileCount(_savedTileCount);
        setTotalTileSize(_savedTileSize);

        quint32 avg = 0;
        if (_savedTileSize != 0) {
            avg = _savedTileSize / _savedTileCount;
        } else {
            qCWarning(QGCCachedTileSetLog) << "_savedTileSize=0";
        }

        setUniqueTileSize(_uniqueTileCount * avg);
    }

    setDownloading(false);

    emit completeChanged();
}

void QGCCachedTileSet::_prepareDownload()
{
    if (_tilesToDownload.isEmpty()) {
        if (_noMoreTiles) {
            _doneWithDownload();
        } else if (!_batchRequested) {
            createDownloadTask();
        }
        return;
    }

    for (qsizetype i = _replies.count(); i < QGeoTileFetcherQGC::concurrentDownloads(_type); i++) {
        if (_tilesToDownload.isEmpty()) {
            break;
        }

        QGCTile* const tile = _tilesToDownload.dequeue();
        const int mapId = UrlFactory::getQtMapIdFromProviderType(tile->type);
        QNetworkRequest request = QGeoTileFetcherQGC::getNetworkRequest(mapId, tile->x, tile->y, tile->z);
        request.setOriginatingObject(this);
        request.setAttribute(QNetworkRequest::User, tile->hash);

        QNetworkReply* const reply = _networkManager->get(request);
        reply->setParent(this);
        QGCNetworkHelper::ignoreSslErrorsIfNeeded(reply);
        (void) connect(reply, &QNetworkReply::finished, this, &QGCCachedTileSet::_networkReplyFinished);
        (void) connect(reply, &QNetworkReply::errorOccurred, this, &QGCCachedTileSet::_networkReplyError);
        (void) _replies.insert(tile->hash, reply);

        delete tile;
        if (!_batchRequested && !_noMoreTiles && (_tilesToDownload.count() < (QGeoTileFetcherQGC::concurrentDownloads(_type) * 10))) {
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

    if (_replies.contains(hash)) {
        (void) _replies.remove(hash);
    } else {
        qCWarning(QGCCachedTileSetLog) << "Reply not in list: " << hash;
    }
    qCDebug(QGCCachedTileSetLog) << "Tile fetched:" << hash;

    QByteArray image = reply->readAll();
    if (image.isEmpty()) {
        qCWarning(QGCCachedTileSetLog) << "Empty Image";
        return;
    }

    const QString type = UrlFactory::tileHashToType(hash);
    const SharedMapProvider mapProvider = UrlFactory::getMapProviderFromProviderType(type);
    Q_CHECK_PTR(mapProvider);

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

    if (_savedTileCount % 10 == 0) {
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

    setErrorCount(_errorCount + 1);

    const QString hash = reply->request().attribute(QNetworkRequest::User).toString();
    if (hash.isEmpty()) {
        qCWarning(QGCCachedTileSetLog) << "Empty Hash";
        return;
    }

    if (_replies.contains(hash)) {
        (void) _replies.remove(hash);
    } else {
        qCWarning(QGCCachedTileSetLog) << "Reply not in list:" << hash;
    }

    if (error != QNetworkReply::OperationCanceledError) {
        qCWarning(QGCCachedTileSetLog) << "Error:" << reply->errorString();
    }

    QGCUpdateTileDownloadStateTask *task = new QGCUpdateTileDownloadStateTask(_id, QGCTile::StateError, hash);
    if (!getQGCMapEngine()->addTask(task)) {
        task->deleteLater();
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
