#include "Viewer3DTerrainTexture.h"

#include <tuple>

#include "Fact.h"
#include "FlightMapSettings.h"
#include "QGCLoggingCategory.h"
#include "QGCMapEngine.h"
#include "QGCMapUrlEngine.h"
#include "SettingsManager.h"
#include "Viewer3DMapProvider.h"
#include "Viewer3DTileQuery.h"
#include "Viewer3DTileStatistics.h"

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

    QGeoCoordinate bbMin;
    QGeoCoordinate bbMax;

    if (_mapProvider && _mapProvider->mapLoaded()) {
        std::tie(bbMin, bbMax) = _mapProvider->mapBoundingBox();
    } else if (_fallbackCenter.isValid()) {
        // No OSM map loaded: load a small default tile area around the
        // fallback center (vehicle home position).
        std::tie(bbMin, bbMax) = _fallbackBoundingBox(_fallbackCenter);
    } else {
        qCDebug(Viewer3DTerrainTextureLog) << "loadTexture: no map loaded and no fallback center";
        return;
    }

    if (!_terrainTileLoader) {
        _terrainTileLoader = new Viewer3DTileQuery(this);
    }

    connect(_terrainTileLoader, &Viewer3DTileQuery::loadingMapCompleted, this, &Viewer3DTerrainTexture::_updateTexture, Qt::UniqueConnection);
    connect(_terrainTileLoader, &Viewer3DTileQuery::textureGeometryReady, this, &Viewer3DTerrainTexture::setTextureGeometry, Qt::UniqueConnection);

    _terrainTileLoader->adaptiveMapTilesLoader(_mapType, _mapId, bbMin, bbMax);
}

std::pair<QGeoCoordinate, QGeoCoordinate> Viewer3DTerrainTexture::_fallbackBoundingBox(const QGeoCoordinate &center)
{
    constexpr double fallbackRadiusMeters = 500.0;
    const QGeoCoordinate south = center.atDistanceAndAzimuth(fallbackRadiusMeters, 180);
    const QGeoCoordinate west = center.atDistanceAndAzimuth(fallbackRadiusMeters, 270);
    const QGeoCoordinate north = center.atDistanceAndAzimuth(fallbackRadiusMeters, 0);
    const QGeoCoordinate east = center.atDistanceAndAzimuth(fallbackRadiusMeters, 90);
    return {QGeoCoordinate(south.latitude(), west.longitude(), 0),
            QGeoCoordinate(north.latitude(), east.longitude(), 0)};
}

void Viewer3DTerrainTexture::setFallbackCenter(const QGeoCoordinate &newFallbackCenter)
{
    if (_fallbackCenter == newFallbackCenter) {
        return;
    }
    _fallbackCenter = newFallbackCenter;
    emit fallbackCenterChanged();

    // The fallback region only drives the texture when no map is loaded.
    if (!_mapProvider || !_mapProvider->mapLoaded()) {
        if (!_fallbackCenter.isValid() && _terrainTileLoader) {
            // Fallback center went invalid: drop any in-flight tile query so
            // stale results can't resurrect texture state for a region that
            // no longer has meaning. Disconnect first: the loader stays alive
            // until the deferred delete runs and could still emit.
            disconnect(_terrainTileLoader, nullptr, this, nullptr);
            _terrainTileLoader->deleteLater();
            _terrainTileLoader = nullptr;
            _setTextureLoaded(false);
            setTextureGeometryDone(false);
        } else {
            loadTexture();
        }
    }
}

void Viewer3DTerrainTexture::_updateTexture()
{
    if (!_terrainTileLoader) {
        return;
    }

    qCDebug(Viewer3DTerrainTextureLog) << "Texture loaded:" << _terrainTileLoader->mapSize();
    setSize(_terrainTileLoader->mapSize());
    setFormat(QQuick3DTextureData::RGBA32F);
    setHasTransparency(false);

    setTextureData(_terrainTileLoader->mapData());
    _setTextureLoaded(true);
    setTextureGeometryDone(true);
    disconnect(_terrainTileLoader, &Viewer3DTileQuery::loadingMapCompleted, this, &Viewer3DTerrainTexture::_updateTexture);

    _terrainTileLoader->deleteLater();
    _terrainTileLoader = nullptr;
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

void Viewer3DTerrainTexture::setTextureGeometry(const Viewer3DTileStatistics &tileInfo)
{
    setRoiMinCoordinate(tileInfo.coordinateMin);
    setRoiMaxCoordinate(tileInfo.coordinateMax);
    setTileCount(tileInfo.tileCounts);
}
