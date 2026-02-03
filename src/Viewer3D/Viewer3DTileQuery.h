#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtGui/QImage>
#include <QtPositioning/QGeoCoordinate>

#include "Viewer3DTileReply.h"

Q_DECLARE_LOGGING_CATEGORY(Viewer3DTileQueryLog)

class Viewer3DTileQuery : public QObject
{
    Q_OBJECT

    friend class Viewer3DTileQueryTest;

public:
    struct TileStatistics_t {
        QGeoCoordinate coordinateMin;
        QGeoCoordinate coordinateMax;
        QSize tileCounts;
        int zoomLevel = 0;
    };

    explicit Viewer3DTileQuery(QObject *parent = nullptr);

    void adaptiveMapTilesLoader(const QString &mapType, int mapId, const QGeoCoordinate &coordinateMin, const QGeoCoordinate &coordinateMax);
    int maxTileCount(int zoomLevel, const QGeoCoordinate &coordinateMin, const QGeoCoordinate &coordinateMax);
    QByteArray mapData() const { return _mapToBeLoaded.mapData(); }
    QSize mapSize() const { return QSize(_mapToBeLoaded.mapWidth, _mapToBeLoaded.mapHeight); }

signals:
    void loadingMapCompleted();
    void mapTileDownloaded(float progress);
    void textureGeometryReady(TileStatistics_t tileInfo);

private:
    void _loadMapTiles(int zoomLevel, QPoint tileMinIndex, QPoint tileMaxIndex);
    TileStatistics_t _findAndLoadMapTiles(int zoomLevel, const QGeoCoordinate &coordinateMin, const QGeoCoordinate &coordinateMax);
    void _tileDone(Viewer3DTileReply::TileInfo_t tileData);
    void _tileGiveUp(Viewer3DTileReply::TileInfo_t tileData);
    void _tileEmpty(Viewer3DTileReply::TileInfo_t tileData);
    void _cleanupReply(Viewer3DTileReply *reply);
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

    QNetworkAccessManager *_networkManager = nullptr;
    MapTileContainer_t _mapToBeLoaded;
    QGeoCoordinate _textureCoordinateMin;
    QGeoCoordinate _textureCoordinateMax;
    QString _mapType;

    int _totalTilesCount = 0;
    int _downloadedTilesCount = 0;
    int _mapId = 0;
    int _zoomLevel = 0;
};

Q_DECLARE_METATYPE(Viewer3DTileQuery::TileStatistics_t)
