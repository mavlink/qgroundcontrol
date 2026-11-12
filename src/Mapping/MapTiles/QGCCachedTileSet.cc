#include "QGCCachedTileSet.h"

#include <memory>
#include <utility>

#include "QGCFormat.h"
#include "QGCLoggingCategory.h"
#include "QGCMapEngine.h"
#include "QGCMapEngineManager.h"
#include "QGCMapTasks.h"
#include "QGCTileReply.h"

QGC_LOGGING_CATEGORY(QGCCachedTileSetLog, "MapTiles.QGCCachedTileSet")

namespace {

void queueDownloadState(quint64 setId, const QString& hash, QGCTile::TileState state)
{
    QGCUpdateTileDownloadStateTask* const task = new QGCUpdateTileDownloadStateTask(setId, state, hash);
    if (!getQGCMapEngine()->addTask(task)) {
        task->deleteLater();
    }
}

}  // namespace

QGCCachedTileSet::QGCCachedTileSet(const QString& name, QObject* parent)
    : QObject(parent)
    , _name(name)
{
    qCDebug(QGCCachedTileSetLog) << this;

    // downloadStatus() is built from the tile counts and sizes
    (void) connect(this, &QGCCachedTileSet::totalTileCountChanged, this, &QGCCachedTileSet::downloadStatusChanged);
    (void) connect(this, &QGCCachedTileSet::totalTilesSizeChanged, this, &QGCCachedTileSet::downloadStatusChanged);
    (void) connect(this, &QGCCachedTileSet::savedTileCountChanged, this, &QGCCachedTileSet::downloadStatusChanged);
    (void) connect(this, &QGCCachedTileSet::savedTileSizeChanged, this, &QGCCachedTileSet::downloadStatusChanged);
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
    if (!_downloading) {
        setErrorCount(0);
        setDownloading(true);
        _noMoreTiles = false;
    }

    QGCGetTileDownloadListTask* task = new QGCGetTileDownloadListTask(_id, kTileBatchSize);
    (void) connect(
        task, &QGCGetTileDownloadListTask::tileListFetched, this,
        [this, serial = _downloadSerial](const QQueue<QGCTile*>& tiles) { _tileListFetched(tiles, serial); });
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
    if (_downloading) {
        return;
    }

    // Also the tiles that a cancelled download was given: the database has them as being downloaded
    QGCUpdateTileDownloadStateTask* task = new QGCUpdateTileDownloadStateTask(_id, QGCTile::StatePending, "*");
    if (!getQGCMapEngine()->addTask(task)) {
        task->deleteLater();
    }

    createDownloadTask();
}

void QGCCachedTileSet::cancelDownloadTask()
{
    // A download list that is still to arrive is dropped
    _downloadSerial++;
    _batchRequested = false;

    qDeleteAll(_tilesToDownload);
    _tilesToDownload.clear();

    // An aborted request does not finish: its tile is no error of the set, and stays to be fetched
    for (QGCTileReply* const reply : std::as_const(_replies)) {
        reply->abort();
        reply->deleteLater();
    }
    _replies.clear();

    setDownloading(false);
}

void QGCCachedTileSet::_tileListFetched(const QQueue<QGCTile*>& tiles, quint32 downloadSerial)
{
    if (downloadSerial != _downloadSerial) {
        // The list of a download that was cancelled: a resume gets these tiles from the database again
        qDeleteAll(tiles);
        return;
    }

    _batchRequested = false;
    if (tiles.size() < kTileBatchSize) {
        _noMoreTiles = true;
    }

    if (tiles.isEmpty()) {
        _prepareDownload();
        return;
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
    // A slot of a signal of this set can have cancelled the download that the caller goes on with
    if (!_downloading) {
        return;
    }

    if (_tilesToDownload.isEmpty()) {
        if (_noMoreTiles) {
            // A tile that is still being fetched can fail, so the download is done with the last answer
            if (_replies.isEmpty()) {
                _doneWithDownload();
            }
        } else if (!_batchRequested) {
            createDownloadTask();
        }
        return;
    }

    for (qsizetype i = _replies.count(); i < kConcurrentDownloads; i++) {
        if (_tilesToDownload.isEmpty()) {
            break;
        }

        const std::unique_ptr<QGCTile> tile(_tilesToDownload.dequeue());
        const QString hash = tile->hash;
        QGCTileReply* const reply = new QGCTileReply(_networkManager, tile->type, tile->x, tile->y, tile->z, this);
        reply->setTileSet(_id);
        (void) connect(reply, &QGCTileReply::finished, this, [this, reply, hash] { _tileReplyFinished(reply, hash); });
        (void) _replies.insert(hash, reply);
        reply->start();

        if (!_batchRequested && !_noMoreTiles && (_tilesToDownload.count() < (kConcurrentDownloads * 10))) {
            createDownloadTask();
        }
    }
}

void QGCCachedTileSet::_tileReplyFinished(QGCTileReply* reply, const QString& hash)
{
    reply->deleteLater();
    (void) _replies.remove(hash);

    if (reply->error() == QGCTileReply::Error::NoTileError) {
        // Nothing to download: the set is complete without a tile its provider does not have
        qCDebug(QGCCachedTileSetLog) << "No tile to fetch:" << hash;
        queueDownloadState(_id, hash, QGCTile::StateComplete);
    } else if (reply->error() != QGCTileReply::Error::NoError) {
        _tileFailed(hash, reply->errorString());
    } else {
        qCDebug(QGCCachedTileSetLog) << "Tile fetched:" << hash;
        queueDownloadState(_id, hash, QGCTile::StateComplete);

        setSavedTileSize(_savedTileSize + reply->data().size());
        setSavedTileCount(_savedTileCount + 1);

        if (_savedTileCount % 10 == 0) {
            const quint32 avg = _savedTileSize / _savedTileCount;
            setTotalTileSize(avg * _totalTileCount);
            setUniqueTileSize(avg * _uniqueTileCount);
        }
    }

    _prepareDownload();
}

void QGCCachedTileSet::_tileFailed(const QString& hash, const QString& reason)
{
    qCWarning(QGCCachedTileSetLog) << "Error fetching tile" << hash << reason;

    setErrorCount(_errorCount + 1);
    queueDownloadState(_id, hash, QGCTile::StateError);
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
