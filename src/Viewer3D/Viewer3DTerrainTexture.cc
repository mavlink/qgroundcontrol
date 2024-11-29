/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "Viewer3DTerrainTexture.h"

#include "SettingsManager.h"
#include "QGCMapEngine.h"
#include "QGCMapUrlEngine.h"
#include "FlightMapSettings.h"
#include "OsmParser.h"


Viewer3DTerrainTexture::Viewer3DTerrainTexture()
{
    _terrainTileLoader = nullptr;
    _flightMapSettings = SettingsManager::instance()->flightMapSettings();
    mapTypeChangedEvent();

    setTextureGeometryDone(false);
    setTextureLoaded(false);
    setTextureDownloadProgress(100.0);

    // connect(_flightMapSettings->mapProvider(), &Fact::rawValueChanged, this, &Viewer3DTerrainTexture::mapTypeChangedEvent);
    connect(_flightMapSettings->mapType(), &Fact::rawValueChanged, this, &Viewer3DTerrainTexture::mapTypeChangedEvent);
    connect(this, &Viewer3DTerrainTexture::mapProviderIdChanged, this, &Viewer3DTerrainTexture::loadTexture);
}

Viewer3DTerrainTexture::~Viewer3DTerrainTexture()
{
    delete _terrainTileLoader;
}

void Viewer3DTerrainTexture::loadTexture()
{
    setTextureLoaded(false);
    setTextureGeometryDone(false);
    setTextureDownloadProgress(0.0);
    if(_osmParser->mapLoaded()){
        if(!_terrainTileLoader){
            _terrainTileLoader = new MapTileQuery(this);
            connect(_terrainTileLoader, &MapTileQuery::loadingMapCompleted, this, &Viewer3DTerrainTexture::updateTexture);
            connect(_terrainTileLoader, &MapTileQuery::textureGeometryReady, this, &Viewer3DTerrainTexture::setTextureGeometry);
        }
        _terrainTileLoader->adaptiveMapTilesLoader(_mapType, _mapId,
                                                   _osmParser->getMapBoundingBoxCoordinate().first,
                                                   _osmParser->getMapBoundingBoxCoordinate().second);
        connect(_terrainTileLoader, &MapTileQuery::mapTileDownloaded, this, &Viewer3DTerrainTexture::setTextureDownloadProgress);
    }
}

void Viewer3DTerrainTexture::updateTexture()
{
    MapTileQuery* _extureQuery = qobject_cast<MapTileQuery*>(QObject::sender());

    setSize(_terrainTileLoader->getMapSize());
    setFormat(QQuick3DTextureData::RGBA32F);
    setHasTransparency(false);

    setTextureData(_terrainTileLoader->getMapData());
    setTextureLoaded(true);
    setTextureGeometryDone(true);
    disconnect(_terrainTileLoader, &MapTileQuery::mapTileDownloaded, this, &Viewer3DTerrainTexture::setTextureDownloadProgress);
    disconnect(_terrainTileLoader, &MapTileQuery::loadingMapCompleted, this, &Viewer3DTerrainTexture::updateTexture);
    _terrainTileLoader = nullptr;
    setTextureDownloadProgress(100.0);
    _extureQuery->deleteLater();
}

void Viewer3DTerrainTexture::mapTypeChangedEvent(void)
{
    _mapType.clear();
    _mapType = _flightMapSettings->mapProvider()->rawValue().toString() + QString(" ");
    _mapType += _flightMapSettings->mapType()->rawValue().toString();

    int mapId = UrlFactory::getQtMapIdFromProviderType(_mapType);

    if(mapId == _mapId){
        return;
    }

    _mapId = mapId;
    emit mapProviderIdChanged();
}

QGeoCoordinate Viewer3DTerrainTexture::roiMinCoordinate() const
{
    return _roiMinCoordinate;
}

void Viewer3DTerrainTexture::setRoiMinCoordinate(const QGeoCoordinate &newRoiMinCoordinate)
{
    if (_roiMinCoordinate == newRoiMinCoordinate){
        return;
    }
    _roiMinCoordinate = newRoiMinCoordinate;
    emit roiMinCoordinateChanged();
}

QGeoCoordinate Viewer3DTerrainTexture::roiMaxCoordinate() const
{
    return _roiMaxCoordinate;
}

void Viewer3DTerrainTexture::setRoiMaxCoordinate(const QGeoCoordinate &newRoiMaxCoordinate)
{
    if (_roiMaxCoordinate == newRoiMaxCoordinate){
        return;
    }
    _roiMaxCoordinate = newRoiMaxCoordinate;
    emit roiMaxCoordinateChanged();
}

bool Viewer3DTerrainTexture::textureLoaded() const
{
    return _textureLoaded;
}

OsmParser *Viewer3DTerrainTexture::osmParser() const
{
    return _osmParser;
}

void Viewer3DTerrainTexture::setOsmParser(OsmParser *newOsmParser)
{
    if (_osmParser == newOsmParser){
        return;
    }
    _osmParser = newOsmParser;

    if(_osmParser){
        connect(_osmParser, &OsmParser::mapChanged, this, &Viewer3DTerrainTexture::loadTexture);
    }

    emit osmParserChanged();
}

QSize Viewer3DTerrainTexture::tileCount() const
{
    return _tileCount;
}

void Viewer3DTerrainTexture::setTileCount(const QSize &newTileCount)
{
    if (_tileCount == newTileCount){
        return;
    }
    _tileCount = newTileCount;
    emit tileCountChanged();
}

bool Viewer3DTerrainTexture::textureGeometryDone() const
{
    return _textureGeometryDone;
}

void Viewer3DTerrainTexture::setTextureGeometryDone(bool newTextureGeometryDone)
{
    if (_textureGeometryDone == newTextureGeometryDone){
        return;
    }
    _textureGeometryDone = newTextureGeometryDone;
    emit textureGeometryDoneChanged();
}

float Viewer3DTerrainTexture::textureDownloadProgress() const
{
    return _textureDownloadProgress;
}

void Viewer3DTerrainTexture::setTextureDownloadProgress(float newTextureDownloadProgress)
{
    if (qFuzzyCompare(_textureDownloadProgress, newTextureDownloadProgress)){
        return;
    }
    _textureDownloadProgress = newTextureDownloadProgress;
    emit textureDownloadProgressChanged();
}

void Viewer3DTerrainTexture::setTextureGeometry(MapTileQuery::TileStatistics_t tileInfo)
{
    setRoiMinCoordinate(tileInfo.coordinateMin);
    setRoiMaxCoordinate(tileInfo.coordinateMax);
    setTileCount(tileInfo.tileCounts);
}
