#include "QGCCachedTileSet.h"

#include <QtCore/QTimer>
#include <chrono>
#include <utility>

#include "ElevationMapProvider.h"
#include "QGCFormat.h"
#include "QGCHostCircuitBreaker.h"
#include "QGCLoggingCategory.h"
#include "QGCMapEngine.h"
#include "QGCMapEngineManager.h"
#include "QGCMapTasks.h"
#include "QGCMapUrlEngine.h"
#include "QGCNetworkHelper.h"
#include "QGCTile.h"
#include "QGCTileCache.h"
#include "QGCTileCacheDatabase.h"
#include "QGCTileCacheWorker.h"
#include "QGeoTileFetcherQGC.h"

QGC_LOGGING_CATEGORY(QGCCachedTileSetLog, "QtLocationPlugin.QGCCachedTileSet")

namespace {
// O1: builds the fire-and-forget tile-download-state update as a generic command
// task, keeping the "*" wildcard (all-tiles) vs single-hash branch in one place.
QGCCommandTask* makeUpdateStateTask(quint64 setID, QGCTile::TileState state, const QString& hash,
                                    QGCTile::TileState fromState = QGCTile::StatePending,
                                    bool filterByFromState = false)
{
    return new QGCCommandTask(
        QGCMapTask::TaskType::taskUpdateTileDownloadState,
        [setID, state, hash, fromState, filterByFromState](QGCCacheWorker& worker, QGCMapTask& self) {
            if (!worker.validateDatabase(&self)) {
                return false;
            }
            bool ok;
            if (hash == QStringLiteral("*")) {
                const int from = filterByFromState ? static_cast<int>(fromState) : -1;
                ok = worker.database()->updateAllTileDownloadStates(setID, static_cast<int>(state), from);
            } else {
                ok = worker.database()->updateTileDownloadState(setID, static_cast<int>(state), hash);
            }
            if (!ok) {
                self.setError("Error updating tile download state");
            }
            return true;
        });
}
}  // namespace

QGCCachedTileSet::QGCCachedTileSet(const QString& name, QObject* parent) : QObject(parent), _name(name)
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
    if (_cancelPending || _paused) {
        setDownloading(false);
        return;
    }

    if (!_downloading) {
        setErrorCount(0);
        setDownloading(true);
        _noMoreTiles = false;
        _maxConcurrentDownloads = QGeoTileFetcherQGC::concurrentDownloads(_type);
    }

    QGCGetTileDownloadListTask* task = new QGCGetTileDownloadListTask(_id, kTileBatchSize);
    // Queued: the task signal is emitted from the cache-worker thread; this slot
    // must run on the main thread where all download state lives.
    (void) connect(task, &QGCGetTileDownloadListTask::tileListFetched, this, &QGCCachedTileSet::_tileListFetched,
                   Qt::QueuedConnection);
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
    setPaused(false);

    // Reactivate tiles parked by a pause (Paused -> Pending) without disturbing
    // already-completed rows.
    QGCCommandTask* task =
        makeUpdateStateTask(_id, QGCTile::StatePending, QStringLiteral("*"), QGCTile::StatePaused, true);
    if (!getQGCMapEngine()->addTask(task)) {
        task->deleteLater();
    }

    createDownloadTask();
}

void QGCCachedTileSet::pauseDownloadTask()
{
    if (!_downloading && !_paused) {
        return;
    }

    setPaused(true);
    ++_downloadEpoch;

    // Abort in-flight replies without letting their finished/error slots run, so
    // they don't mutate state or re-enter _prepareDownload after the pause. Their
    // gate slots would otherwise leak (the handlers that release are blocked), so
    // release here.
    for (const DownloadReply& pending : std::as_const(_replies)) {
        pending.reply->blockSignals(true);
        pending.reply->abort();
        pending.reply->deleteLater();
        HostConcurrencyGate::instance().release(pending.host);
    }
    _replies.clear();

    qDeleteAll(_tilesToDownload);
    _tilesToDownload.clear();

    QGCMapEngine* engine = getQGCMapEngine();

    // Park both in-flight (Downloading) and queued (Pending) tiles as Paused so
    // getTileDownloadList (which only pulls Pending) stops handing them out.
    QGCCommandTask* downloadingTask =
        makeUpdateStateTask(_id, QGCTile::StatePaused, QStringLiteral("*"), QGCTile::StateDownloading, true);
    if (!engine || !engine->addTask(downloadingTask)) {
        downloadingTask->deleteLater();
    }

    QGCCommandTask* pendingTask =
        makeUpdateStateTask(_id, QGCTile::StatePaused, QStringLiteral("*"), QGCTile::StatePending, true);
    if (!engine || !engine->addTask(pendingTask)) {
        pendingTask->deleteLater();
    }

    _noMoreTiles = false;
    _batchRequested = false;
    setDownloading(false);
}

void QGCCachedTileSet::cancelDownloadTask()
{
    _cancelPending = true;
    ++_downloadEpoch;
}

void QGCCachedTileSet::_tileListFetched(const QQueue<QGCTile*>& tiles)
{
    _batchRequested = false;
    if (tiles.size() < kTileBatchSize) {
        _noMoreTiles = true;
    }

    if (tiles.isEmpty()) {
        _doneWithDownload();
        return;
    }

    if (_paused || _cancelPending) {
        qDeleteAll(tiles);
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
    if (_tilesToDownload.isEmpty()) {
        if (_noMoreTiles) {
            _doneWithDownload();
        } else if (!_batchRequested) {
            createDownloadTask();
        }
        return;
    }

    if (_paused || _cancelPending) {
        return;
    }

    const qsizetype maxConcurrent = _maxConcurrentDownloads;

    for (qsizetype i = _replies.count(); i < maxConcurrent; i++) {
        if (_tilesToDownload.isEmpty()) {
            break;
        }
        QGCTile* tile = _tilesToDownload.dequeue();

        QNetworkRequest request = QGeoTileFetcherQGC::getNetworkRequest(tile->type, tile->x, tile->y, tile->z);
        if (!request.url().isValid()) {
            qCWarning(QGCCachedTileSetLog) << "Invalid URL for tile" << tile->hash << "- skipping";
            setErrorCount(_errorCount + 1);
            delete tile;
            continue;
        }
        request.setOriginatingObject(this);
        request.setAttribute(QNetworkRequest::User, tile->hash);

        const QString host = request.url().host();

        // Circuit breaker (R2): a host the live fetcher has tripped is skipped here
        // too, so bulk downloads don't keep hammering a dead endpoint.
        if (!HostCircuitBreaker::instance().allowRequest(host)) {
            qCDebug(QGCCachedTileSetLog) << "Circuit breaker open; skipping tile" << tile->hash;
            setErrorCount(_errorCount + 1);
            QGCCommandTask* task = makeUpdateStateTask(_id, QGCTile::StateError, tile->hash);
            if (!getQGCMapEngine()->addTask(task)) {
                task->deleteLater();
            }
            delete tile;
            continue;
        }

        // Shared cross-path gate (A2): the combined live+bulk in-flight count for
        // this host must stay under the provider cap. If the live fetcher already
        // holds the host's slots, re-queue this tile (front) and stop filling; a
        // gate release (here or in the live path) plus the next completion's
        // _prepareDownload drains the backlog.
        const SharedMapProvider provider = UrlFactory::getMapProviderFromProviderType(tile->type);
        const bool isOSM = provider && provider->isOSMProvider();
        if (!HostConcurrencyGate::instance().tryAcquire(host, static_cast<int>(maxConcurrent), isOSM)) {
            _tilesToDownload.prepend(tile);
            break;
        }

        _startTileGet(tile->hash, request, 0, host);
        delete tile;

        if (!_batchRequested && !_noMoreTiles && (_tilesToDownload.count() < (maxConcurrent * 10))) {
            createDownloadTask();
        }
    }
}

void QGCCachedTileSet::_startTileGet(const QString& hash, const QNetworkRequest& request, int attempt,
                                     const QString& host)
{
    QNetworkReply* const reply = _networkManager->get(request);
    reply->setParent(this);
    QGCNetworkHelper::ignoreSslErrorsIfNeeded(reply);
    (void) connect(reply, &QNetworkReply::finished, this, [this, hash, reply]() { _replyFinished(hash, reply); });
    (void) connect(reply, &QNetworkReply::errorOccurred, this,
                   [this, hash, reply](QNetworkReply::NetworkError error) { _replyError(hash, reply, error); });
    _replies.insert(hash, DownloadReply{reply, request, attempt, host});
}

void QGCCachedTileSet::_replyFinished(const QString& hash, QNetworkReply* reply)
{
    if (!reply) {
        qCWarning(QGCCachedTileSetLog) << "NULL Reply";
        return;
    }
    reply->deleteLater();

    // errorOccurred fires before finished and owns the failure/retry path; bail
    // here so a retried reply isn't also treated as a (failed) completion.
    if (reply->error() != QNetworkReply::NoError) {
        return;
    }

    if (!reply->isOpen()) {
        qCWarning(QGCCachedTileSetLog) << "Empty Reply";
        return;
    }

    const auto replyIt = _replies.constFind(hash);
    const QString host = (replyIt != _replies.constEnd()) ? replyIt->host : reply->request().url().host();
    if (replyIt != _replies.constEnd()) {
        (void) _replies.erase(replyIt);
    } else {
        qCWarning(QGCCachedTileSetLog) << "Reply not in list: " << hash;
    }

    // Release the shared per-host slot claimed in _prepareDownload so a parked
    // live/bulk request for this host can proceed.
    HostConcurrencyGate::instance().release(host);
    HostCircuitBreaker::instance().recordSuccess(host);
    qCDebug(QGCCachedTileSetLog) << "Tile fetched:" << hash;

    QByteArray image = reply->readAll();
    if (image.isEmpty()) {
        qCWarning(QGCCachedTileSetLog) << "Empty Image";
        return;
    }

    const QString type = UrlFactory::tileHashToType(hash);
    const SharedMapProvider mapProvider = UrlFactory::getMapProviderFromProviderType(type);
    if (!mapProvider) {
        qCWarning(QGCCachedTileSetLog) << "Invalid map provider for type:" << type;
        return;
    }

    if (const SharedElevationProvider elevationProvider =
            std::dynamic_pointer_cast<const ElevationProvider>(mapProvider)) {
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

    QGCTileCache::cacheTile(type, hash, image, format, _id);

    QGCCommandTask* task = makeUpdateStateTask(_id, QGCTile::StateComplete, hash);
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

void QGCCachedTileSet::_replyError(const QString& hash, QNetworkReply* reply, QNetworkReply::NetworkError error)
{
    if (!reply) {
        return;
    }
    qCDebug(QGCCachedTileSetLog) << "Error fetching tile" << reply->errorString();

    const auto it = _replies.constFind(hash);
    const DownloadReply context = (it != _replies.constEnd()) ? *it : DownloadReply{};
    if (it != _replies.constEnd()) {
        (void) _replies.remove(hash);
    } else {
        qCWarning(QGCCachedTileSetLog) << "Reply not in list:" << hash;
    }

    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (_retryOrFail(hash, context, statusCode, error, reply)) {
        // Retry scheduled: the per-host slot stays held across the backoff timer
        // and is reused by the re-issued GET (no re-acquire, no double-count).
        return;
    }

    // Terminal failure: release the per-host slot claimed in _prepareDownload.
    HostConcurrencyGate::instance().release(context.host);
    setErrorCount(_errorCount + 1);
    if (error != QNetworkReply::OperationCanceledError) {
        qCWarning(QGCCachedTileSetLog) << "Error:" << reply->errorString();
    }

    QGCCommandTask* task = makeUpdateStateTask(_id, QGCTile::StateError, hash);
    if (!getQGCMapEngine()->addTask(task)) {
        task->deleteLater();
    }

    _prepareDownload();
}

bool QGCCachedTileSet::_retryOrFail(const QString& hash, const DownloadReply& context, int statusCode,
                                    QNetworkReply::NetworkError error, const QNetworkReply* reply)
{
    if (!QGCNetworkHelper::isTransientError(error, statusCode)) {
        return false;
    }
    HostCircuitBreaker::instance().recordFailure(context.request.url().host());
    if ((context.attempt >= kMaxRetryAttempts) || _paused || _cancelPending) {
        return false;
    }

    std::chrono::milliseconds delay = QGCNetworkHelper::retryBackoff(context.attempt);
    if (statusCode == QGCNetworkHelper::kHttpTooManyRequests) {
        delay = QGCNetworkHelper::retryAfterFromReply(reply).value_or(QGCNetworkHelper::kDefaultRateLimitDelay);
    }

    const QNetworkRequest request = context.request;
    const int nextAttempt = context.attempt + 1;
    const quint64 epoch = _downloadEpoch;
    qCDebug(QGCCachedTileSetLog) << "Retrying tile" << hash << "attempt" << nextAttempt << "in" << delay.count()
                                 << "ms";

    const QString host = context.host;
    QTimer::singleShot(delay, this, [this, hash, request, nextAttempt, epoch, host]() {
        // A pause/cancel between scheduling and firing bumps _downloadEpoch; bail so
        // a resumed session that already re-queued this tile doesn't get a duplicate
        // GET. The held per-host slot must be released on the bail path; pause/cancel
        // teardown released the rest of the in-flight slots but this retry was not yet
        // back in _replies, so it owns its slot here.
        if ((epoch != _downloadEpoch) || _paused || _cancelPending || !_networkManager) {
            HostConcurrencyGate::instance().release(host);
            return;
        }
        _startTileGet(hash, request, nextAttempt, host);
    });
    return true;
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
    return QGC::numberToString(_errorCount);
}

QString QGCCachedTileSet::totalTileCountStr() const
{
    return QGC::numberToString(_totalTileCount);
}

QString QGCCachedTileSet::totalTilesSizeStr() const
{
    return QGC::bigSizeToString(_totalTileSize);
}

QString QGCCachedTileSet::uniqueTileSizeStr() const
{
    return QGC::bigSizeToString(_uniqueTileSize);
}

QString QGCCachedTileSet::uniqueTileCountStr() const
{
    return QGC::numberToString(_uniqueTileCount);
}

QString QGCCachedTileSet::savedTileCountStr() const
{
    return QGC::numberToString(_savedTileCount);
}

QString QGCCachedTileSet::savedTileSizeStr() const
{
    return QGC::bigSizeToString(_savedTileSize);
}
