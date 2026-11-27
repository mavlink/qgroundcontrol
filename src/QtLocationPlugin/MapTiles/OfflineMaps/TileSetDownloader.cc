#include "TileSetDownloader.h"

#include <utility>

#include "CacheTask.h"
#include "DownloadTile.h"
#include "QGCLoggingCategory.h"
#include "TileCache.h"
#include "TileReply.h"
#include "TileSetTasks.h"

QGC_LOGGING_CATEGORY(TileSetDownloaderLog, "MapTiles.TileSetDownloader")

TileSetDownloader::TileSetDownloader(QObject* parent)
    : QObject(parent)
{}

void TileSetDownloader::start(quint64 setID)
{
    _setID = setID;
    if (!_active) {
        _active = true;
        _noMoreTiles = false;
    }

    _requestTileList();
}

void TileSetDownloader::retry(quint64 setID)
{
    const quint32 serial = _downloadSerial;
    // Also the tiles that a cancelled download was given: the database has them as being downloaded
    CacheTask* const task = TileSetTasks::resetTileDownloads(setID);
    // Queued, so that the failure reaches the download that the next start() begins, also when the cache refuses the
    // task at once. The worker ends this task before the tile list of that start().
    (void) connect(
        task, &CacheTask::finished, this,
        [this, serial](const QString& error) {
            if (!error.isEmpty()) {
                _retryFailed(serial);
            }
        },
        Qt::QueuedConnection);
    emit retryRequested(task);
    if (!TileCache::instance()->addTask(task)) {
        QMetaObject::invokeMethod(this, [this, serial] { _retryFailed(serial); }, Qt::QueuedConnection);
    }
}

void TileSetDownloader::_retryFailed(quint32 downloadSerial)
{
    if ((downloadSerial != _downloadSerial) || !_batchRequested) {
        return;
    }

    // The tiles that failed are not listed again, and the download would end without an error. The list that it
    // asked for is dropped, as is a second notice of a task that the cache refused.
    qCWarning(TileSetDownloaderLog) << "The tiles of the set" << _setID << "could not be reset to download them again";
    _downloadSerial++;
    _tileListFailed(_downloadSerial);
}

void TileSetDownloader::cancel()
{
    _storeStates();

    // A download list that is still to arrive is dropped
    _downloadSerial++;
    _batchRequested = false;

    _tilesToDownload.clear();

    // An aborted request does not finish: its tile is no error of the set, and stays to be fetched
    for (TileReply* const reply : std::as_const(_replies)) {
        reply->abort();
        reply->deleteLater();
    }
    _replies.clear();

    _active = false;
}

void TileSetDownloader::_requestTileList()
{
    const quint32 serial = _downloadSerial;
    CacheTask* const task = TileSetTasks::tileDownloadList(
        _setID, kTileBatchSize, this,
        [this, serial](const QList<DownloadTile>& tiles) { _tileListFetched(tiles, serial); });
    // Queued, as below: a cache that takes no task can end the task at once, and the download is not to end from
    // within the function that asks for its tiles
    (void) connect(
        task, &CacheTask::finished, this,
        [this, serial](const QString& error) {
            if (!error.isEmpty()) {
                _tileListFailed(serial);
            }
        },
        Qt::QueuedConnection);
    _batchRequested = true;
    emit tileListRequested(task);
    if (!TileCache::instance()->addTask(task)) {
        QMetaObject::invokeMethod(this, [this, serial] { _tileListFailed(serial); }, Qt::QueuedConnection);
    }
}

void TileSetDownloader::_tileListFailed(quint32 downloadSerial)
{
    // A task that the cache refuses can tell of it twice: with its end, and with what the cache answers
    if ((downloadSerial != _downloadSerial) || !_batchRequested) {
        return;
    }

    qCWarning(TileSetDownloaderLog) << "No list of the tiles to download for the set" << _setID;
    _batchRequested = false;
    _noMoreTiles = true;
    emit tileFailed();
    _fetchTiles();
}

void TileSetDownloader::_tileListFetched(const QList<DownloadTile>& tiles, quint32 downloadSerial)
{
    if (downloadSerial != _downloadSerial) {
        // The list of a download that was cancelled, or that gave it up: a retry gets these tiles from the database
        // again
        return;
    }

    _batchRequested = false;
    if (tiles.size() < kTileBatchSize) {
        _noMoreTiles = true;
    }

    _tilesToDownload.append(tiles);
    _fetchTiles();
}

void TileSetDownloader::_fetchTiles()
{
    // A slot of one of the signals can have cancelled the download that the caller goes on with
    if (!_active) {
        return;
    }

    if (_tilesToDownload.isEmpty()) {
        if (_noMoreTiles) {
            // A tile that is still being fetched can fail, so the download is done with the last answer
            if (_replies.isEmpty()) {
                _active = false;
                _storeStates();
                emit finished();
            }
        } else if (!_batchRequested) {
            _requestTileList();
        }
        return;
    }

    for (qsizetype i = _replies.size(); i < kConcurrentDownloads; i++) {
        if (_tilesToDownload.isEmpty()) {
            break;
        }

        const DownloadTile tile = _tilesToDownload.dequeue();
        const QString hash = tile.hash;
        TileReply* const reply = new TileReply(tile.mapId, tile.x, tile.y, tile.z, this);
        reply->setTileSet(_setID);
        (void) connect(reply, &TileReply::finished, this, [this, reply, hash] { _tileReplyFinished(reply, hash); });
        (void) _replies.insert(hash, reply);
        reply->start();

        if (!_batchRequested && !_noMoreTiles && (_tilesToDownload.size() < (kConcurrentDownloads * 10))) {
            _requestTileList();
        }
    }
}

void TileSetDownloader::_tileReplyFinished(TileReply* reply, const QString& hash)
{
    reply->deleteLater();
    (void) _replies.remove(hash);

    if (reply->error() == TileReply::Error::NoTileError) {
        // Nothing to download: the set is complete without a tile its provider does not have
        qCDebug(TileSetDownloaderLog) << "No tile to fetch:" << hash;
        _statesToStore.append({hash, DownloadTile::TileState::Complete});
    } else if (reply->error() != TileReply::Error::NoError) {
        qCWarning(TileSetDownloaderLog) << "Error fetching tile" << hash << reply->errorString();
        _statesToStore.append({hash, DownloadTile::TileState::Error});
        emit tileFailed();
    } else {
        qCDebug(TileSetDownloaderLog) << "Tile fetched:" << hash;
        _statesToStore.append({hash, DownloadTile::TileState::Complete});
        emit tileSaved(reply->data().size());
    }
    if (_statesToStore.size() >= kStateBatchSize) {
        _storeStates();
    }

    _fetchTiles();
}

void TileSetDownloader::_storeStates()
{
    if (_statesToStore.isEmpty()) {
        return;
    }

    const qsizetype count = _statesToStore.size();
    CacheTask* const task = TileSetTasks::updateTileDownloadStates(_setID, std::exchange(_statesToStore, {}));
    // The tiles stay listed as being downloaded, and a retry() lists them again
    (void) connect(task, &CacheTask::finished, this, [setID = _setID, count](const QString& error) {
        if (!error.isEmpty()) {
            qCWarning(TileSetDownloaderLog)
                << "No download states stored for" << count << "tiles of the set" << setID << error;
        }
    });
    (void) TileCache::instance()->addTask(task);
}
