#pragma once

#include <array>
#include <chrono>
#include <memory>

#include <QtCore/QCache>
#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QSet>
#include <QtGui/QImage>
#include <QtLocation/private/qgeotiledmapreply_p.h>
#include <QtLocation/private/qgeotilespec_p.h>

class TileReply;
class QGCGeoFileTileCache;
class RuntimeScheduler;
class ScheduledTask;

/// The tiles that their providers do not have, as the replies of a tile fetcher found. A map asks for the tiles of
/// its view whenever the view changes: the server is not asked again for these.
class GeoNoTiles
{
public:
    bool contains(const QGeoTileSpec& spec) const { return _specs.contains(spec); }

    void insert(const QGeoTileSpec& spec);

    void clear() { _specs.clear(); }

private:
    QSet<QGeoTileSpec> _specs;
};

/// What the replies of a tile fetcher share about the requests that failed: the tiles that are not asked for again
/// for a while, and whether the system is online. Its scheduler times the retries of the replies and that while.
class GeoTileFailures : public QObject
{
    Q_OBJECT

public:
    /// How long a tile whose request failed is not asked for again, unless the system comes back online first
    static constexpr std::chrono::minutes kRememberedFor{2};

    /// \a scheduler, which is not owned and must outlive this object, is a timer of the event loop when it is null
    explicit GeoTileFailures(RuntimeScheduler* scheduler = nullptr);

    RuntimeScheduler* scheduler() const { return _scheduler; }

    /// True for a tile whose request failed less than kRememberedFor ago
    bool contains(const QGeoTileSpec& spec);

    void insert(const QGeoTileSpec& spec);

    void clear() { _until.clear(); }

    /// False while the system reports that it cannot reach the internet
    bool isOnline() const { return _online; }

    /// Back online, the failures are forgotten and backOnline() is emitted
    void setOnline(bool online);

signals:
    void backOnline();

private:
    RuntimeScheduler* const _scheduler;
    QHash<QGeoTileSpec, quint64> _until;  ///< The time of the scheduler until which a tile is not asked for, in µs
    bool _online = true;
};

/// The requests for tiles that the replies of a tile fetcher share, and the pictures of the tiles above that are scaled
/// up in place of tiles: the tile above several tiles that the provider lacks is asked for once, and decoded once, for
/// all of them.
class GeoTileRequests
{
public:
    GeoTileRequests();

    /// The request for the tile \a spec that is under way, or else a new one, which is started. It is aborted and
    /// deleted when the last reply lets go of it.
    std::shared_ptr<TileReply> request(const QGeoTileSpec& spec);

    /// The picture of the tile \a spec as decode() gave it lately. Null when it did not.
    QImage image(const QGeoTileSpec& spec) const;

    /// The picture of the tile \a spec, which is \a data. Null for data that is no picture.
    QImage decode(const QGeoTileSpec& spec, const QByteArray& data);

    void clear();

private:
    QHash<QGeoTileSpec, std::weak_ptr<TileReply>> _requests;
    QCache<QGeoTileSpec, QImage> _images;  ///< Their cost is their size in bytes
};

/// Presents a TileReply as the reply QtLocation's tile fetcher expects.
///
/// Where the provider has no tile, the map shows the part of the nearest tile above that shows the same ground, scaled
/// up to a whole tile: a coarse picture, not a hole. QtLocation itself fills a hole only from the tiles it still holds
/// in memory. The reply gives the tile above to the tile cache, which scales the part up when the map looks the tile
/// up (QGCGeoFileTileCache::insertFromAbove()), and finishes without bytes. Below the deepest zoom level of the
/// provider (MapProvider::maxZoom()), the reply starts at the tile of that level without asking for a deeper one.
///
/// A request that failed in a way that can clear up is made again by the reply, which stays unfinished meanwhile:
/// after the next of kRetryDelays; for a server that was asked too often, at its turn after the pause of the server
/// (TileServerPauses), during which no request asks that server; and, for a server that did not answer while the
/// system is offline, once the system is back online or after the next of kOfflineRetryDelays, whichever comes first.
/// A server that is unavailable for a time it named is paused too: the request made after the next of kRetryDelays
/// waits for its turn after that pause.
/// Any other failure, and one with no retry left, finishes the reply without a tile and without an error, and its tile
/// is not asked for again for GeoTileFailures::kRememberedFor. QtLocation itself asks again five times, within
/// 16 s, for the tile of a reply with an error.
class QGCGeoTiledMapReply : public QGeoTiledMapReply
{
    Q_OBJECT

public:
    /// How many zoom levels above its own a reply looks for a tile. A tile from this far above has 1 of its 256
    /// pixels along the edge of the part that is shown: one from further above has nothing more to show.
    static constexpr int kMaxAncestorLevels = 8;

    static constexpr std::array<std::chrono::milliseconds, 3> kRetryDelays{
        std::chrono::seconds{1}, std::chrono::seconds{4}, std::chrono::seconds{16}};
    static constexpr std::array<std::chrono::milliseconds, kRetryDelays.size()> kOfflineRetryDelays{
        std::chrono::seconds{30}, std::chrono::minutes{2}, std::chrono::minutes{8}};

    /// The replies that share \a noTiles ask the server once for a tile it lacks, those that share \a failures do
    /// not ask for a tile that failed lately, and those that share \a requests ask once for a tile above that
    /// several of them are shown from. A reply that is given none remembers only what it finds itself. \a tileCache,
    /// which is not owned, is given the tiles above: without it, a tile that the provider lacks is not shown.
    explicit QGCGeoTiledMapReply(const QGeoTileSpec& spec, std::shared_ptr<GeoNoTiles> noTiles = nullptr,
                                 std::shared_ptr<GeoTileFailures> failures = nullptr,
                                 std::shared_ptr<GeoTileRequests> requests = nullptr,
                                 QGCGeoFileTileCache* tileCache = nullptr, QObject* parent = nullptr);
    ~QGCGeoTiledMapReply();

    /// Starts the request. The reply finishes from within init() when neither its tile nor a tile above needs to be
    /// asked for. While it waits to ask again, it stays unfinished.
    void init();

    /// Also stops waiting to ask again, and lets go of the request under way
    void abort() final;

    /// True for a reply that finished without a tile, and without an error, as the provider has none here and none
    /// within kMaxAncestorLevels above
    bool isNoTile() const { return _noTile; }

private:
    /// Asks for the nearest tile, from _ancestorLevels above on, that is not known to be missing
    void _requestTile();
    void _tileReplyFinished();
    /// Finishes once the tile cache has \a above, the picture of the tile _ancestorLevels above, and \a aboveData,
    /// its bytes when this reply fetched them. \a cached is false for a tile above that came from the server.
    void _finishFromAbove(const QImage& above, const QByteArray& aboveData, const QString& aboveFormat, bool cached);
    void _dropTileReply();
    /// Makes the request that failed as \a tileReply did again later
    /// \return False when the request is not made again
    bool _retryLater(const TileReply& tileReply);
    void _retry();
    /// Finishes without a tile, as the tile _ancestorLevels above could not be fetched
    void _giveUp(const QString& errorString);

    const std::shared_ptr<GeoNoTiles> _noTiles;
    const std::shared_ptr<GeoTileFailures> _failures;
    const std::shared_ptr<GeoTileRequests> _requests;
    const QPointer<QGCGeoFileTileCache> _tileCache;
    const std::unique_ptr<ScheduledTask> _retryTask;
    QMetaObject::Connection _backOnline;
    std::shared_ptr<TileReply> _tileReply;  ///< The request for the tile _ancestorLevels above
    int _ancestorLevels = 0;
    int _retries = 0;
    bool _noTile = false;
};
