#pragma once

#include <array>
#include <chrono>

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QPoint>
#include <QtCore/QSet>
#include <QtCore/QSize>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtGui/QImage>
#include <QtPositioning/QGeoCoordinate>

#include "RuntimeScheduler.h"
#include "Viewer3DTileInfo.h"
#include "Viewer3DTileStatistics.h"

class TileReply;

/// Loads the tiles of a region into one picture, at the deepest zoom level that takes fewer than 200 of them.
///
/// A tile whose request failed in a way that can clear up is asked for again, as the tiles of the map views are
/// (QGCGeoTiledMapReply): after the next of kRetryDelays for a server that did not answer or cannot serve the tile
/// now, and at its turn after the pause of a server that was asked too often. The map is not complete until then.
/// A tile that failed for good, or as often as there are delays, is left out of the picture.
class Viewer3DTileQuery : public QObject
{
    Q_OBJECT

    friend class Viewer3DTileQueryTest;

public:
    using TileStatistics_t = Viewer3DTileStatistics;

    static constexpr std::array<std::chrono::milliseconds, 3> kRetryDelays{
        std::chrono::seconds{1}, std::chrono::seconds{4}, std::chrono::seconds{16}};

    /// \a scheduler times the requests that are made again: one of its own when it is null
    explicit Viewer3DTileQuery(QObject* parent = nullptr, RuntimeScheduler* scheduler = nullptr);

    void adaptiveMapTilesLoader(int mapId, const QGeoCoordinate& coordinateMin, const QGeoCoordinate& coordinateMax);
    int maxTileCount(int zoomLevel, const QGeoCoordinate &coordinateMin, const QGeoCoordinate &coordinateMax);
    QByteArray mapData() const { return _mapToBeLoaded.mapData(); }
    QSize mapSize() const { return QSize(_mapToBeLoaded.mapWidth, _mapToBeLoaded.mapHeight); }

signals:
    void loadingMapCompleted();
    void textureGeometryReady(TileStatistics_t tileInfo);

private:
    void _loadMapTiles(int zoomLevel, QPoint tileMinIndex, QPoint tileMaxIndex);
    TileStatistics_t _findAndLoadMapTiles(int zoomLevel, const QGeoCoordinate &coordinateMin, const QGeoCoordinate &coordinateMax);
    /// \a retries counts the requests for the tile that failed before this one
    void _requestTile(const Viewer3DTileInfo& tile, int retries);
    void _tileReplyFinished(TileReply* reply, Viewer3DTileInfo tileData, int retries);
    /// Asks for \a tile again, now or later, when the failure of \a reply can clear up and a retry is left
    bool _retryLater(const TileReply& reply, const Viewer3DTileInfo& tile, int retries);
    /// Also drops the requests that wait to be made again
    void _abortTileReplies();
    void _tileDone(Viewer3DTileInfo tileData);
    void _tileGiveUp(Viewer3DTileInfo tileData);
    void _tileEmpty(Viewer3DTileInfo tileData);
    static QString _tileKey(int mapId, int x, int y, int zoomLevel);

    struct MapTileContainer_t
    {
        static constexpr int tileSize = 256;

        QStringList tileList;
        QPoint tileMinIndex;
        QPoint tileMaxIndex;
        QPoint currentTileIndex;
        QByteArray currentTileData;
        QImage mapTextureImage;

        int zoomLevel = 0;
        int mapWidth = 0;
        int mapHeight = 0;

        void init();
        void setMapTile();
        QByteArray mapData() const;
        void clear();
    };

    RuntimeScheduler* const _scheduler;
    QList<TileReply*> _tileReplies;              ///< Those of the latest load that did not finish
    QSet<RuntimeScheduler::TaskId> _retryTasks;  ///< Those of the latest load that wait to ask again
    MapTileContainer_t _mapToBeLoaded;
    QGeoCoordinate _textureCoordinateMin;
    QGeoCoordinate _textureCoordinateMax;

    int _totalTilesCount = 0;
    int _mapId = 0;
    int _zoomLevel = 0;
};
