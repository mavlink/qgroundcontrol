#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>

#include "TileCacheTypes.h"

class CacheTask;
class TileSetDownloader;

/// An offline tile set of the tile cache, for the UI: what it covers, how much of it is saved, and its download,
/// which TileSetDownloader does
class CachedTileSet : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString name READ name NOTIFY nameChanged FINAL)
    Q_PROPERTY(QString mapTypeStr READ mapTypeStr CONSTANT FINAL)
    Q_PROPERTY(double topleftLon READ topleftLon CONSTANT FINAL)
    Q_PROPERTY(double topleftLat READ topleftLat CONSTANT FINAL)
    Q_PROPERTY(double bottomRightLon READ bottomRightLon CONSTANT FINAL)
    Q_PROPERTY(double bottomRightLat READ bottomRightLat CONSTANT FINAL)
    Q_PROPERTY(int minZoom READ minZoom CONSTANT FINAL)
    Q_PROPERTY(int maxZoom READ maxZoom CONSTANT FINAL)
    Q_PROPERTY(quint32 totalTileCount READ totalTileCount NOTIFY totalTileCountChanged FINAL)
    Q_PROPERTY(quint64 totalTilesSize READ totalTilesSize NOTIFY totalTilesSizeChanged FINAL)
    Q_PROPERTY(quint32 uniqueTileCount READ uniqueTileCount NOTIFY uniqueTileCountChanged FINAL)
    Q_PROPERTY(quint64 uniqueTileSize READ uniqueTileSize NOTIFY uniqueTileSizeChanged FINAL)
    Q_PROPERTY(quint32 savedTileCount READ savedTileCount NOTIFY savedTileCountChanged FINAL)
    Q_PROPERTY(quint64 savedTileSize READ savedTileSize NOTIFY savedTileSizeChanged FINAL)
    Q_PROPERTY(QString downloadStatus READ downloadStatus NOTIFY downloadStatusChanged FINAL)
    Q_PROPERTY(bool complete READ complete NOTIFY completeChanged FINAL)
    Q_PROPERTY(bool defaultSet READ defaultSet CONSTANT FINAL)
    Q_PROPERTY(quint64 id READ id CONSTANT FINAL)
    Q_PROPERTY(bool deleting READ deleting NOTIFY deletingChanged FINAL)
    Q_PROPERTY(bool downloading READ downloading NOTIFY downloadingChanged FINAL)
    Q_PROPERTY(quint32 errorCount READ errorCount NOTIFY errorCountChanged FINAL)
    Q_PROPERTY(bool selected READ selected WRITE setSelected NOTIFY selectedChanged FINAL)

public:
    /// \a record tells what the set covers. Its tile count is the total of the set until setTotals() tells another.
    explicit CachedTileSet(const TileSetRecord& record, QObject* parent = nullptr);
    ~CachedTileSet();

    Q_INVOKABLE void createDownloadTask();
    /// Downloads the tiles that the set lacks: those that failed, and those a cancel left
    Q_INVOKABLE void resumeDownloadTask();
    /// Stops the download at once: the requests in flight are aborted, and are no errors of the set
    Q_INVOKABLE void cancelDownloadTask();

    /// What the set covers, as the tile cache stores it, with the total of its tiles as it is now
    TileSetRecord record() const;

    /// The counts and sizes of the tiles of the set
    const TileSetTotals& totals() const { return _totals; }

    const QString& name() const { return _record.name; }

    const QString& mapTypeStr() const { return _record.mapName; }

    double topleftLat() const { return _record.topleftLat; }

    double topleftLon() const { return _record.topleftLon; }

    double bottomRightLat() const { return _record.bottomRightLat; }

    double bottomRightLon() const { return _record.bottomRightLon; }

    int minZoom() const { return _record.minZoom; }

    int maxZoom() const { return _record.maxZoom; }

    quint64 id() const { return _record.setID; }

    bool defaultSet() const { return _record.defaultSet; }

    quint32 totalTileCount() const { return _totals.totalTileCount; }

    quint64 totalTilesSize() const { return _totals.totalTileSize; }

    quint32 uniqueTileCount() const { return _totals.uniqueTileCount; }

    quint64 uniqueTileSize() const { return _totals.uniqueTileSize; }

    quint32 savedTileCount() const { return _totals.savedTileCount; }

    quint64 savedTileSize() const { return _totals.savedTileSize; }

    QString downloadStatus() const;

    bool complete() const { return (_record.defaultSet || (_totals.totalTileCount <= _totals.savedTileCount)); }

    bool deleting() const { return _deleting; }

    bool downloading() const { return _downloading; }

    quint32 errorCount() const { return _errorCount; }

    bool selected() const { return _selected; }

    void setSelected(bool sel);

    void setName(const QString& name);

    /// The id that the tile cache gave the set when it saved it
    void setId(quint64 id) { _record.setID = id; }

    /// Emits the signal of each count and size that changed
    void setTotals(const TileSetTotals& totals);

    void setDeleting(bool deleting);
    void setDownloading(bool downloading);
    void setErrorCount(quint32 count);

signals:
    void deletingChanged();
    void downloadingChanged();
    void totalTileCountChanged();
    void uniqueTileCountChanged();
    void uniqueTileSizeChanged();
    void totalTilesSizeChanged();
    void savedTileCountChanged();
    void savedTileSizeChanged();
    void downloadStatusChanged();
    void completeChanged();
    void errorCountChanged();
    void selectedChanged();
    void nameChanged();
    /// The set is about to queue \a task for the tile cache, for its download: for who shows the user the task that
    /// fails
    void cacheTaskQueued(CacheTask* task);

private:
    void _tileSaved(qint64 size);
    void _downloadFinished();

    TileSetRecord _record;
    TileSetTotals _totals;
    quint32 _errorCount = 0;
    bool _deleting = false;
    bool _downloading = false;
    bool _selected = false;

    TileSetDownloader* const _downloader;

    /// The tiles saved between two estimates of the size of the set from the average size of its saved tiles
    static constexpr quint32 kSizeEstimateInterval = 10;
};
