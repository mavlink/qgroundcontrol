#include "CachedTileSet.h"

#include <utility>

#include "QGCFormat.h"
#include "QGCLoggingCategory.h"
#include "TileSetDownloader.h"

QGC_LOGGING_CATEGORY(CachedTileSetLog, "MapTiles.CachedTileSet")

CachedTileSet::CachedTileSet(const TileSetRecord& record, QObject* parent)
    : QObject(parent)
    , _record(record)
    , _totals{.totalTileCount = record.numTiles}
    , _downloader(new TileSetDownloader(this))
{
    qCDebug(CachedTileSetLog) << this;

    (void) connect(_downloader, &TileSetDownloader::tileListRequested, this, &CachedTileSet::cacheTaskQueued);
    (void) connect(_downloader, &TileSetDownloader::retryRequested, this, &CachedTileSet::cacheTaskQueued);
    (void) connect(_downloader, &TileSetDownloader::tileSaved, this, &CachedTileSet::_tileSaved);
    (void) connect(_downloader, &TileSetDownloader::tileFailed, this, [this] { setErrorCount(_errorCount + 1); });
    (void) connect(_downloader, &TileSetDownloader::finished, this, &CachedTileSet::_downloadFinished);

    // downloadStatus() is built from the tile counts and sizes
    (void) connect(this, &CachedTileSet::totalTileCountChanged, this, &CachedTileSet::downloadStatusChanged);
    (void) connect(this, &CachedTileSet::totalTilesSizeChanged, this, &CachedTileSet::downloadStatusChanged);
    (void) connect(this, &CachedTileSet::savedTileCountChanged, this, &CachedTileSet::downloadStatusChanged);
    (void) connect(this, &CachedTileSet::savedTileSizeChanged, this, &CachedTileSet::downloadStatusChanged);
}

CachedTileSet::~CachedTileSet()
{
    qCDebug(CachedTileSetLog) << this;
}

TileSetRecord CachedTileSet::record() const
{
    TileSetRecord record = _record;
    record.numTiles = _totals.totalTileCount;
    return record;
}

QString CachedTileSet::downloadStatus() const
{
    if (_record.defaultSet) {
        return QGC::bigSizeToString(_totals.totalTileSize);
    }

    const QString savedSize = QGC::bigSizeToString(_totals.savedTileSize);
    if (_totals.totalTileCount <= _totals.savedTileCount) {
        return savedSize;
    }

    return savedSize + QStringLiteral(" / ") + QGC::bigSizeToString(_totals.totalTileSize);
}

void CachedTileSet::setName(const QString& name)
{
    if (name != _record.name) {
        _record.name = name;
        emit nameChanged();
    }
}

void CachedTileSet::setTotals(const TileSetTotals& totals)
{
    const bool wasComplete = complete();
    const TileSetTotals old = std::exchange(_totals, totals);
    if (complete() != wasComplete) {
        emit completeChanged();
    }
    if (totals.totalTileCount != old.totalTileCount) {
        emit totalTileCountChanged();
    }
    if (totals.totalTileSize != old.totalTileSize) {
        emit totalTilesSizeChanged();
    }
    if (totals.savedTileCount != old.savedTileCount) {
        emit savedTileCountChanged();
    }
    if (totals.savedTileSize != old.savedTileSize) {
        emit savedTileSizeChanged();
    }
    if (totals.uniqueTileCount != old.uniqueTileCount) {
        emit uniqueTileCountChanged();
    }
    if (totals.uniqueTileSize != old.uniqueTileSize) {
        emit uniqueTileSizeChanged();
    }
}

void CachedTileSet::setDeleting(bool deleting)
{
    if (deleting != _deleting) {
        _deleting = deleting;
        emit deletingChanged();
    }
}

void CachedTileSet::setDownloading(bool downloading)
{
    if (downloading != _downloading) {
        _downloading = downloading;
        emit downloadingChanged();
    }
}

void CachedTileSet::setErrorCount(quint32 count)
{
    if (count != _errorCount) {
        _errorCount = count;
        emit errorCountChanged();
    }
}

void CachedTileSet::createDownloadTask()
{
    if (!_downloading) {
        setErrorCount(0);
        setDownloading(true);
    }

    _downloader->start(_record.setID);
}

void CachedTileSet::resumeDownloadTask()
{
    if (_downloading) {
        return;
    }

    _downloader->retry(_record.setID);
    createDownloadTask();
}

void CachedTileSet::cancelDownloadTask()
{
    _downloader->cancel();
    setDownloading(false);
}

void CachedTileSet::_tileSaved(qint64 size)
{
    TileSetTotals totals = _totals;
    totals.savedTileSize += size;
    totals.savedTileCount++;
    if ((totals.savedTileCount % kSizeEstimateInterval) == 0) {
        const quint64 averageSize = totals.savedTileSize / totals.savedTileCount;
        totals.totalTileSize = averageSize * totals.totalTileCount;
        totals.uniqueTileSize = averageSize * totals.uniqueTileCount;
    }
    setTotals(totals);
}

void CachedTileSet::_downloadFinished()
{
    if (_errorCount == 0) {
        TileSetTotals totals = _totals;
        totals.totalTileCount = totals.savedTileCount;
        totals.totalTileSize = totals.savedTileSize;

        quint64 averageSize = 0;
        if (totals.savedTileCount != 0) {
            averageSize = totals.savedTileSize / totals.savedTileCount;
        } else {
            qCWarning(CachedTileSetLog) << "_savedTileCount=0";
        }
        totals.uniqueTileSize = averageSize * totals.uniqueTileCount;
        setTotals(totals);
    }

    setDownloading(false);
}

void CachedTileSet::setSelected(bool sel)
{
    if (sel != _selected) {
        _selected = sel;
        emit selectedChanged();
    }
}
