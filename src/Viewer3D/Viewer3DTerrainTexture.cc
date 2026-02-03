#include "Viewer3DTerrainTexture.h"

#include "FlightMapSettings.h"
#include "QGCLoggingCategory.h"
#include "QGCMapEngine.h"
#include "QGCMapUrlEngine.h"
#include "SettingsManager.h"
#include "Viewer3DMapProvider.h"

QGC_LOGGING_CATEGORY(Viewer3DTerrainTextureLog, "Viewer3d.Viewer3DTerrainTexture")

Viewer3DTerrainTexture::Viewer3DTerrainTexture()
    : _flightMapSettings(SettingsManager::instance()->flightMapSettings())
{
    _onMapTypeChanged();

    connect(_flightMapSettings->mapType(), &Fact::rawValueChanged, this, &Viewer3DTerrainTexture::_onMapTypeChanged);
}

void Viewer3DTerrainTexture::loadTexture()
{
    _setTextureLoaded(false);
    setTextureGeometryDone(false);
    setTextureDownloadProgress(0.0f);

    if (!_mapProvider || !_mapProvider->mapLoaded()) {
        qCDebug(Viewer3DTerrainTextureLog) << "loadTexture: no map provider or map not loaded";
        return;
    }

    if (!_terrainTileLoader) {
        _terrainTileLoader = new Viewer3DTileQuery(this);
    }

    connect(_terrainTileLoader, &Viewer3DTileQuery::loadingMapCompleted, this, &Viewer3DTerrainTexture::_updateTexture, Qt::UniqueConnection);
    connect(_terrainTileLoader, &Viewer3DTileQuery::textureGeometryReady, this, &Viewer3DTerrainTexture::setTextureGeometry, Qt::UniqueConnection);
    connect(_terrainTileLoader, &Viewer3DTileQuery::mapTileDownloaded, this, &Viewer3DTerrainTexture::setTextureDownloadProgress, Qt::UniqueConnection);

    const auto [bbMin, bbMax] = _mapProvider->mapBoundingBox();
    _terrainTileLoader->adaptiveMapTilesLoader(_mapType, _mapId, bbMin, bbMax);
}

void Viewer3DTerrainTexture::_updateTexture()
{
    qCDebug(Viewer3DTerrainTextureLog) << "Texture loaded:" << _terrainTileLoader->mapSize();
    setSize(_terrainTileLoader->mapSize());
    setFormat(QQuick3DTextureData::RGBA32F);
    setHasTransparency(false);

    setTextureData(_terrainTileLoader->mapData());
    _setTextureLoaded(true);
    setTextureGeometryDone(true);
    disconnect(_terrainTileLoader, &Viewer3DTileQuery::mapTileDownloaded, this, &Viewer3DTerrainTexture::setTextureDownloadProgress);
    disconnect(_terrainTileLoader, &Viewer3DTileQuery::loadingMapCompleted, this, &Viewer3DTerrainTexture::_updateTexture);

    _terrainTileLoader->deleteLater();
    _terrainTileLoader = nullptr;
    setTextureDownloadProgress(100.0f);
}

void Viewer3DTerrainTexture::_onMapTypeChanged()
{
    _mapType = _flightMapSettings->mapProvider()->rawValue().toString()
             + QStringLiteral(" ")
             + _flightMapSettings->mapType()->rawValue().toString();

    const int mapId = UrlFactory::getQtMapIdFromProviderType(_mapType);

    if (mapId == _mapId) {
        return;
    }

    qCDebug(Viewer3DTerrainTextureLog) << "Map type changed:" << _mapType << "mapId:" << mapId;
    _mapId = mapId;
    loadTexture();
}

void Viewer3DTerrainTexture::setRoiMinCoordinate(const QGeoCoordinate &newRoiMinCoordinate)
{
    if (_roiMinCoordinate == newRoiMinCoordinate) {
        return;
    }
    _roiMinCoordinate = newRoiMinCoordinate;
    emit roiMinCoordinateChanged();
}

void Viewer3DTerrainTexture::setRoiMaxCoordinate(const QGeoCoordinate &newRoiMaxCoordinate)
{
    if (_roiMaxCoordinate == newRoiMaxCoordinate) {
        return;
    }
    _roiMaxCoordinate = newRoiMaxCoordinate;
    emit roiMaxCoordinateChanged();
}

void Viewer3DTerrainTexture::setMapProvider(Viewer3DMapProvider *newMapProvider)
{
    if (_mapProvider == newMapProvider) {
        return;
    }

    if (_mapProvider) {
        disconnect(_mapProvider, &Viewer3DMapProvider::mapChanged, this, &Viewer3DTerrainTexture::loadTexture);
    }

    _mapProvider = newMapProvider;

    if (_mapProvider) {
        connect(_mapProvider, &Viewer3DMapProvider::mapChanged, this, &Viewer3DTerrainTexture::loadTexture);
        // Handle late binding: provider may already have a loaded map before this object is wired.
        loadTexture();
    }

    emit mapProviderChanged();
}

void Viewer3DTerrainTexture::setTileCount(const QSize &newTileCount)
{
    if (_tileCount == newTileCount) {
        return;
    }
    _tileCount = newTileCount;
    emit tileCountChanged();
}

void Viewer3DTerrainTexture::setTextureGeometryDone(bool newTextureGeometryDone)
{
    if (_textureGeometryDone == newTextureGeometryDone) {
        return;
    }
    _textureGeometryDone = newTextureGeometryDone;
    emit textureGeometryDoneChanged();
}

void Viewer3DTerrainTexture::setTextureDownloadProgress(float newTextureDownloadProgress)
{
    if (qFuzzyCompare(_textureDownloadProgress, newTextureDownloadProgress)) {
        return;
    }
    _textureDownloadProgress = newTextureDownloadProgress;
    emit textureDownloadProgressChanged();
}

void Viewer3DTerrainTexture::setTextureGeometry(const Viewer3DTileQuery::TileStatistics_t &tileInfo)
{
    setRoiMinCoordinate(tileInfo.coordinateMin);
    setRoiMaxCoordinate(tileInfo.coordinateMax);
    setTileCount(tileInfo.tileCounts);
}
