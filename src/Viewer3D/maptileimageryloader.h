#ifndef MAPTILEIMAGERYLOADER_H
#define MAPTILEIMAGERYLOADER_H

#include "qvectornd.h"
#include <QPixmap>
#include <QObject>
#include <QImage>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QPainter>
#include <QDebug>
#include <QGeoCoordinate>

class MapTileImageryLoader : public QObject
{
    struct MapTileCOntainer
    {
        int L = 256; // length of each square image downloaded tile

        std::vector<QPoint> tilesIndexArray;
        int zoomLevel;
        QPoint tileMinIndex;
        QPoint tileMaxIndex;
        int currentTileStat;

        QPoint currentTileIndex;
        QByteArray currentTileData;

        QImage mapTileMerged;
        int mapWidth, mapHeight;
        void init(){
            mapWidth = (tileMaxIndex.x() - tileMinIndex.x() + 1) * L;
            mapHeight = (tileMaxIndex.y() - tileMinIndex.y() + 1) * L;
            mapTileMerged = QImage(mapWidth, mapHeight, QImage::Format_RGB32);
        }

        void setMapTile(){
            QPixmap tmpImage;
            tmpImage.loadFromData(currentTileData);

            QPainter painter(&mapTileMerged);
            painter.drawImage((currentTileIndex.x() - tileMinIndex.x()) * L,
                              (currentTileIndex.y() - tileMinIndex.y()) * L,
                              tmpImage.toImage());
        }

        QByteArray getMapData(){
            QImage tmpImage = mapTileMerged.convertToFormat(QImage::Format_RGBA32FPx4);
            QByteArray tmpImageData =  QByteArray::fromRawData((const char*)tmpImage.bits(), tmpImage.sizeInBytes());
            tmpImageData.data();

            return tmpImageData;
        }

        void clear(){
            tilesIndexArray.clear();
        }
    };


    Q_OBJECT
public:
    explicit MapTileImageryLoader(QObject *parent = nullptr);

    void loadMapTiles(int zoomLevel, QPoint tileMinIndex, QPoint tileMaxIndex);
    std::pair<QGeoCoordinate, QGeoCoordinate> findAndLoadMapTiles(int zoomLevel, QGeoCoordinate coordinate_1, QGeoCoordinate coordinate_2);
    QByteArray getMapData(){ return _mapToBeLoaded.getMapData();}
    QSize getMapSize(){ return QSize(_mapToBeLoaded.mapWidth, _mapToBeLoaded.mapHeight);}

private:
    QNetworkAccessManager* _manager;
    QNetworkReply* _reply;
    int _mapTileLoadStat;
    MapTileCOntainer _mapToBeLoaded;

    double valueClip(double n, double _minValue, double _maxValue);
    QPoint latLonToPixelXY(QGeoCoordinate pointCoordinate, int zoomLevel);
    QPoint pixelXYToTileXY(QPoint pixel);
    QPoint tileXYToPixelXY(QPoint tile);
    QGeoCoordinate pixelXYToLatLong(QPoint pixel, int zoomLevel);
    void _loadNextMaptile();
    void httpServerReplyFinished();
    void httpReadyRead();

    QString _tileXYToQuadKey(const int tileX, const int tileY, const int zoomLevel);
    int _getServerNum(const int x, const int y, const int max);
    QString bingMapProvider(const int tileX, const int tileY, const int zoomLevel, QString language="en-US");

signals:
    void loadingMapCompleted();
};

#endif // MAPTILEIMAGERYLOADER_H
