/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtQuick3D/QQuick3DTextureData>

#include "Viewer3DTileQuery.h"

class FlightMapSettings;
class OsmParser;

///     @author Omid Esrafilian <esrafilian.omid@gmail.com>


class Viewer3DTerrainTexture : public QQuick3DTextureData
{
    Q_OBJECT
    Q_MOC_INCLUDE("OsmParser.h")

    Q_PROPERTY(OsmParser* osmParser READ osmParser WRITE setOsmParser NOTIFY osmParserChanged)
    Q_PROPERTY(QGeoCoordinate roiMinCoordinate READ roiMinCoordinate WRITE setRoiMinCoordinate NOTIFY roiMinCoordinateChanged)
    Q_PROPERTY(QGeoCoordinate roiMaxCoordinate READ roiMaxCoordinate WRITE setRoiMaxCoordinate NOTIFY roiMaxCoordinateChanged)
    Q_PROPERTY(QSize tileCount READ tileCount NOTIFY tileCountChanged)
    Q_PROPERTY(bool textureLoaded READ textureLoaded NOTIFY textureLoadedChanged)
    Q_PROPERTY(bool textureGeometryDone READ textureGeometryDone NOTIFY textureGeometryDoneChanged)
    Q_PROPERTY(float textureDownloadProgress READ textureDownloadProgress NOTIFY textureDownloadProgressChanged)


public:
    explicit Viewer3DTerrainTexture();
    ~Viewer3DTerrainTexture();

    Q_INVOKABLE void loadTexture();

    int _height, _width;

    QGeoCoordinate roiMinCoordinate() const;
    void setRoiMinCoordinate(const QGeoCoordinate &newRoiMinCoordinate);

    QGeoCoordinate roiMaxCoordinate() const;
    void setRoiMaxCoordinate(const QGeoCoordinate &newRoiMaxCoordinate);

    bool textureLoaded() const;

    OsmParser *osmParser() const;
    void setOsmParser(OsmParser *newOsmParser);

    QSize tileCount() const;
    void setTileCount(const QSize &newTileCount);

    bool textureGeometryDone() const;
    void setTextureGeometryDone(bool newTextureGeometryDone);

    float textureDownloadProgress() const;
    void setTextureDownloadProgress(float newTextureDownloadProgress);
    void setTextureGeometry(MapTileQuery::TileStatistics_t tileInfo);

private:

    MapTileQuery* _terrainTileLoader;
    FlightMapSettings* _flightMapSettings;
    QString _mapType;
    int _mapId;

    void updateTexture();
    void setTextureLoaded(bool laoded){_textureLoaded = laoded; emit textureLoadedChanged();}
    void mapTypeChangedEvent(void);

    QGeoCoordinate _roiMinCoordinate;
    QGeoCoordinate _roiMaxCoordinate;

    bool _textureLoaded;


    OsmParser *_osmParser = nullptr;

    QSize _tileCount;

    bool _textureGeometryDone;

    float _textureDownloadProgress;

signals:
    void roiMinCoordinateChanged();
    void roiMaxCoordinateChanged();
    void textureLoadedChanged();
    void osmParserChanged();
    void tileCountChanged();
    void textureGeometryDoneChanged();
    void mapProviderIdChanged();
    void textureDownloadProgressChanged();
};
