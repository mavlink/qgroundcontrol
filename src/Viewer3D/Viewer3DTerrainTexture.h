#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QSize>
#include <QtPositioning/QGeoCoordinate>
#include <QtQmlIntegration/QtQmlIntegration>
#include <QtQuick3D/QQuick3DTextureData>

#include "Viewer3DTileQuery.h"

Q_DECLARE_LOGGING_CATEGORY(Viewer3DTerrainTextureLog)

class FlightMapSettings;
class Viewer3DMapProvider;

class Viewer3DTerrainTexture : public QQuick3DTextureData
{
    Q_OBJECT
    QML_ELEMENT
    Q_MOC_INCLUDE("Viewer3DMapProvider.h")

    Q_PROPERTY(Viewer3DMapProvider *mapProvider              READ mapProvider              WRITE setMapProvider              NOTIFY mapProviderChanged)
    Q_PROPERTY(QGeoCoordinate       roiMinCoordinate         READ roiMinCoordinate         WRITE setRoiMinCoordinate         NOTIFY roiMinCoordinateChanged)
    Q_PROPERTY(QGeoCoordinate       roiMaxCoordinate         READ roiMaxCoordinate         WRITE setRoiMaxCoordinate         NOTIFY roiMaxCoordinateChanged)
    Q_PROPERTY(QSize                tileCount                READ tileCount                                                  NOTIFY tileCountChanged)
    Q_PROPERTY(bool                 textureLoaded            READ textureLoaded                                              NOTIFY textureLoadedChanged)
    Q_PROPERTY(bool                 textureGeometryDone      READ textureGeometryDone                                        NOTIFY textureGeometryDoneChanged)
    Q_PROPERTY(float                textureDownloadProgress  READ textureDownloadProgress                                    NOTIFY textureDownloadProgressChanged)

public:
    explicit Viewer3DTerrainTexture();

    Q_INVOKABLE void loadTexture();

    Viewer3DMapProvider *mapProvider() const { return _mapProvider; }
    void setMapProvider(Viewer3DMapProvider *newMapProvider);

    QGeoCoordinate roiMinCoordinate() const { return _roiMinCoordinate; }
    void setRoiMinCoordinate(const QGeoCoordinate &newRoiMinCoordinate);

    QGeoCoordinate roiMaxCoordinate() const { return _roiMaxCoordinate; }
    void setRoiMaxCoordinate(const QGeoCoordinate &newRoiMaxCoordinate);

    QSize tileCount() const { return _tileCount; }
    void setTileCount(const QSize &newTileCount);

    bool textureLoaded() const { return _textureLoaded; }
    bool textureGeometryDone() const { return _textureGeometryDone; }
    float textureDownloadProgress() const { return _textureDownloadProgress; }

    void setTextureGeometryDone(bool newTextureGeometryDone);
    void setTextureDownloadProgress(float newTextureDownloadProgress);
    void setTextureGeometry(const Viewer3DTileQuery::TileStatistics_t &tileInfo);

signals:
    void mapProviderChanged();
    void roiMinCoordinateChanged();
    void roiMaxCoordinateChanged();
    void tileCountChanged();
    void textureLoadedChanged();
    void textureGeometryDoneChanged();
    void textureDownloadProgressChanged();

private:
    void _updateTexture();
    void _setTextureLoaded(bool loaded) { _textureLoaded = loaded; emit textureLoadedChanged(); }
    void _onMapTypeChanged();

    Viewer3DTileQuery *_terrainTileLoader = nullptr;
    FlightMapSettings *_flightMapSettings = nullptr;
    Viewer3DMapProvider *_mapProvider = nullptr;

    QGeoCoordinate _roiMinCoordinate;
    QGeoCoordinate _roiMaxCoordinate;
    QSize _tileCount;
    QString _mapType;

    float _textureDownloadProgress = 100.0f;
    int _mapId = 0;
    bool _textureLoaded = false;
    bool _textureGeometryDone = false;
};
