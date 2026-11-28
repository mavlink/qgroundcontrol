#pragma once

#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtCore/QQueue>
#include <QtCore/QString>

#include "DownloadTile.h"

class CacheTask;
class TileReply;

/// Downloads the tiles that an offline tile set of the tile cache lacks. The cache lists them in batches, and is told
/// what became of each, kStateBatchSize of them at a time: the request of a tile stores it in the set
/// (TileReply::setTileSet()), and a tile that failed stays for retry(). A tile whose state the cache was not told,
/// as when the application ends first, counts as being downloaded still, and a retry() downloads it again.
///
/// For the main thread only.
class TileSetDownloader : public QObject
{
    Q_OBJECT

public:
    explicit TileSetDownloader(QObject* parent = nullptr);

    /// Downloads the tiles that the set \a setID is to download. For a download that is running, asks the cache for
    /// more of them.
    void start(quint64 setID);

    /// Has the cache list again the tiles of the set \a setID that failed, and those that a cancelled download was
    /// given: the next start() downloads them. That download ends with an error when the cache cannot do so.
    void retry(quint64 setID);

    /// Stops the download at once, without finished(): the requests in flight are aborted, and their tiles stay to be
    /// downloaded
    void cancel();

signals:
    /// The cache is about to be asked, with \a task, for tiles to download
    void tileListRequested(CacheTask* task);
    /// The cache is about to be asked, with \a task, to list the tiles of retry() again
    void retryRequested(CacheTask* task);
    /// A tile of \a size bytes was fetched, and is stored in the set
    void tileSaved(qint64 size);
    /// A tile could not be fetched, or the cache could not list the tiles that are left to download
    void tileFailed();
    /// The download is over: every tile was fetched, or failed
    void finished();

private:
    void _requestTileList();
    void _tileListFetched(const QList<DownloadTile>& tiles, quint32 downloadSerial);
    /// Ends the download once the requests in flight are answered: no further tile can be asked for
    void _tileListFailed(quint32 downloadSerial);
    /// Ends the download that followed a retry() whose tiles the cache could not list again
    void _retryFailed(quint32 downloadSerial);
    /// Requests the tiles that are listed, kConcurrentDownloads of them at a time, and ends the download after the
    /// last one
    void _fetchTiles();
    void _tileReplyFinished(TileReply* reply, const QString& hash);
    /// Tells the cache what became of the tiles that it was not told of yet
    void _storeStates();

    quint64 _setID = 0;
    bool _active = false;
    bool _noMoreTiles = false;
    bool _batchRequested = false;
    /// Counts the cancelled downloads, and the tile lists given up: a download list is used by the download that
    /// asked for it only
    quint32 _downloadSerial = 0;
    QHash<QString, TileReply*> _replies;
    QQueue<DownloadTile> _tilesToDownload;
    QList<DownloadTileState> _statesToStore;

    static constexpr int kTileBatchSize = 256;
    /// QNetworkAccessManager queues the requests it receives and runs 6 of them in parallel for one HTTP host and
    /// port on desktop platforms
    static constexpr int kConcurrentDownloads = 6;
    /// The states of this many tiles are one task of the cache and one transaction, in place of one for each tile
    static constexpr int kStateBatchSize = 64;
};
