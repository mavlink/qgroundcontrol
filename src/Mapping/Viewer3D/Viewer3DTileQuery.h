#pragma once

#include <chrono>

#include <QtCore/QByteArray>
#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtCore/QPoint>
#include <QtCore/QSize>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtGui/QImage>
#include <QtPositioning/QGeoCoordinate>

#include "Viewer3DTileInfo.h"
#include "Viewer3DTileStatistics.h"

class QGCTileReply;
class QNetworkAccessManager;
class QTimer;

class Viewer3DTileQuery : public QObject
{
    Q_OBJECT

    friend class Viewer3DTileQueryTest;

public:
    using TileStatistics_t = Viewer3DTileStatistics;

    explicit Viewer3DTileQuery(QObject* parent = nullptr);

    void adaptiveMapTilesLoader(int mapId, const QGeoCoordinate& coordinateMin, const QGeoCoordinate& coordinateMax);
    int maxTileCount(int zoomLevel, const QGeoCoordinate& coordinateMin, const QGeoCoordinate& coordinateMax);

    QByteArray mapData() const { return _mapToBeLoaded.mapData(); }

    QSize mapSize() const { return QSize(_mapToBeLoaded.mapWidth, _mapToBeLoaded.mapHeight); }

signals:
    void loadingMapCompleted();
    void textureGeometryReady(TileStatistics_t tileInfo);

private:
    void _loadMapTiles(int zoomLevel, QPoint tileMinIndex, QPoint tileMaxIndex);
    TileStatistics_t _findAndLoadMapTiles(int zoomLevel, const QGeoCoordinate& coordinateMin,
                                          const QGeoCoordinate& coordinateMax);
    void _requestTile(const Viewer3DTileInfo& tile);
    void _abortRequests();
    void _tileReplyFinished(QGCTileReply* reply);
    void _retryUnansweredTiles();
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

    /// The tiles of a load that are not answered after this time are asked for again, kMaxRetries times: they are
    /// requested all at once, so one can wait this long for its turn at the server. Shorter than the transfer
    /// timeout of a request (QGCTileReply, 10 s), which would end such a request with an error before it is asked
    /// again.
    static constexpr std::chrono::seconds kAnswerTimeout{9};
    static constexpr int kMaxRetries = 6;

    QNetworkAccessManager* _networkManager = nullptr;  ///< null: the network manager of the map engine
    QHash<QGCTileReply*, Viewer3DTileInfo> _requests;  ///< the tiles of the load that are not answered yet
    QTimer* _answerTimer = nullptr;
    int _retryCount = 0;
    MapTileContainer_t _mapToBeLoaded;
    QGeoCoordinate _textureCoordinateMin;
    QGeoCoordinate _textureCoordinateMax;

    int _totalTilesCount = 0;
    int _mapId = 0;
    int _zoomLevel = 0;
};
