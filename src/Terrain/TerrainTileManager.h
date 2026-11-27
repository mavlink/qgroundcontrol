#pragma once

#include <QtCore/QCache>
#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QQueue>
#include <QtCore/QSet>
#include <QtPositioning/QGeoCoordinate>

#include "TerrainQueryInterface.h"

class TerrainTile;
class TileReply;

/// The elevations at coordinates, from the tiles of the elevation provider that the settings name. A tile that is not
/// in memory is fetched (TileReply), kMaxTileFetches at a time: a query waits for the tiles it needs, and fails when
/// one of them cannot be had.
///
/// For the main thread only, as the requests for tiles are.
class TerrainTileManager : public QObject
{
    Q_OBJECT
public:
    explicit TerrainTileManager(QObject* parent = nullptr);
    ~TerrainTileManager();

    static TerrainTileManager *instance();

    /// Either returns altitudes from the tiles in memory or fetches the tiles that are missing
    ///     @param[out] error true: altitude not returned due to error, false: altitudes returned
    ///     @return true: altitude returned (check error as well), false: tiles being fetched (altitudes not returned)
    bool getAltitudesForCoordinates(const QList<QGeoCoordinate> &coordinates, QList<double> &altitudes, bool &error);

    void addCoordinateQuery(TerrainQueryInterface *terrainQueryInterface, const QList<QGeoCoordinate> &coordinates);
    void addPathQuery(TerrainQueryInterface *terrainQueryInterface, const QGeoCoordinate &startPoint, const QGeoCoordinate &endPoint);
    void addCarpetQuery(TerrainQueryInterface *terrainQueryInterface, const QGeoCoordinate &swCoord, const QGeoCoordinate &neCoord, bool statsOnly);

private:
    /// Returns a list of individual coordinates along the requested path spaced according to the terrain tile value spacing
    static QList<QGeoCoordinate> _pathQueryToCoords(const QGeoCoordinate &fromCoord, const QGeoCoordinate &toCoord, double &distanceBetween, double &finalDistanceBetween);
    bool _isFailedTile(const QString &hash);
    bool _recordFailedTile(const QString &hash);    ///< Records a failed fetch; returns true if this is the first failure for the tile
    void _clearFailedTile(const QString &hash);
    static void _processCarpetResults(const QList<double> &altitudes, int gridSizeLat, int gridSizeLon,
                                      bool statsOnly, double &minHeight, double &maxHeight, QList<QList<double>> &carpet);

    /// The coordinates of a query that lie in the tiles being fetched for it: their indices, by the hash of the tile
    using PendingTiles = QHash<QString, QList<qsizetype>>;

    struct QueuedRequestInfo_t {
        QPointer<TerrainQueryInterface> terrainQueryInterface;
        TerrainQuery::QueryMode queryMode;
        double distanceBetween;                         ///< Distance between each returned height
        double finalDistanceBetween;                    ///< Distance between for final height
        QList<QGeoCoordinate> coordinates;
        bool carpetStatsOnly;                           ///< For carpet queries: return only stats
        int carpetGridSizeLat;                          ///< For carpet queries: number of rows
        int carpetGridSizeLon;                          ///< For carpet queries: number of columns
        QList<double> altitudes{};                      ///< NaN for a coordinate whose tile is pending
        bool error = false;                             ///< A coordinate has no altitude
        PendingTiles pendingTiles{};
    };

    /// A tile of the elevation provider \a mapId, at zoom level 1
    struct TileFetch
    {
        QString hash;
        int mapId = 0;
        int x = 0;
        int y = 0;
    };

    /// Looks the altitudes of \a coordinates up in the tiles in memory, and fetches the tiles that are missing: the
    /// altitudes of their coordinates, which \a pendingTiles lists, are NaN
    void _lookUpAltitudes(const QList<QGeoCoordinate>& coordinates, QList<double>& altitudes, bool& error,
                          PendingTiles& pendingTiles);
    static double _elevation(const TerrainTile& tile, const QGeoCoordinate& coordinate, bool& error);
    /// Answers the query at once, or once the tiles it needs are fetched
    void _addQuery(QueuedRequestInfo_t requestInfo);
    void _answerQuery(const QueuedRequestInfo_t& requestInfo);
    void _fetchTile(const TileFetch& fetch);
    void _startTileFetches();
    /// \a tile is the one that the validator of \a reply read last, or null
    void _tileReplyFinished(TileReply* reply, const QString& hash, const TerrainTile* tile);
    /// Fills in the altitudes of the queries that wait for the tile \a hash, and answers those that wait for no other
    void _tileReceived(const QString& hash, const TerrainTile& tile);
    /// The tile \a hash cannot be had: the queries that wait for it fail
    void _tileFailed(const QString& hash);

    QList<QueuedRequestInfo_t> _requestQueue;  ///< The queries that wait for tiles
    QCache<QString, TerrainTile> _tiles;       ///< By hash
    QHash<QString, TileReply*> _tileReplies;   ///< The tiles being fetched, by hash
    QQueue<TileFetch> _tileFetchQueue;         ///< The tiles that wait for a fetch to end before theirs starts
    QSet<QString> _queuedTileHashes;           ///< Those of _tileFetchQueue
    QHash<QString, qint64> _failedTiles;  ///< Tile hash -> ms since epoch of last failed fetch; suppresses immediate retries
    qint64 _lastFailedTileSweepMs = 0;      ///< ms since epoch of last expired-entry sweep of _failedTiles

    /// As many as the network manager asks one server at once: more would only wait in its queue
    static constexpr int kMaxTileFetches = 6;
    /// Each covers 0.01 degrees by 0.01 degrees. A query does not need its tiles to stay in memory: it keeps their
    /// altitudes.
    static constexpr int kMaxCachedTiles = 4096;
    static constexpr qint64 kFailedTileBackoffMs = 5000;
};
