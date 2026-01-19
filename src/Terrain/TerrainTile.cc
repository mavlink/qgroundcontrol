#include "TerrainTile.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QtNumeric>
#include <QtPositioning/QGeoCoordinate>

QGC_LOGGING_CATEGORY(TerrainTileLog, "Terrain.terraintile");

TerrainTile::TerrainTile(const QByteArray &byteArray)
{
    qCDebug(TerrainTileLog) << this;

    constexpr int cTileHeaderBytes = static_cast<int>(sizeof(TileInfo_t));
    const int cTileBytesAvailable = byteArray.size();

    if (cTileBytesAvailable < cTileHeaderBytes) {
        qCWarning(TerrainTileLog) << "Terrain tile binary data too small for TileInfo_t header";
        return;
    }

    _tileInfo = *reinterpret_cast<const TileInfo_t*>(byteArray.constData());

    if (_tileInfo.gridSizeLat <= 0 || _tileInfo.gridSizeLon <= 0) {
        qCWarning(TerrainTileLog) << "Invalid grid dimensions:" << _tileInfo.gridSizeLat << _tileInfo.gridSizeLon;
        return;
    }

    constexpr int16_t kMaxGridSize = 10000;
    if (_tileInfo.gridSizeLat > kMaxGridSize || _tileInfo.gridSizeLon > kMaxGridSize) {
        qCWarning(TerrainTileLog) << "Grid dimensions exceed safety limits:" << _tileInfo.gridSizeLat << _tileInfo.gridSizeLon;
        return;
    }

    const int cTileDataBytes = static_cast<int>(sizeof(int16_t)) * _tileInfo.gridSizeLat * _tileInfo.gridSizeLon;
    if (cTileBytesAvailable < cTileHeaderBytes + cTileDataBytes) {
        qCWarning(TerrainTileLog) << "Terrain tile binary data too small for tile data";
        return;
    }

    if (((_tileInfo.neLon - _tileInfo.swLon) < 0.0) || ((_tileInfo.neLat - _tileInfo.swLat) < 0.0)) {
        qCWarning(TerrainTileLog) << this << "Tile extent is infeasible";
        return;
    }

    _cellSizeLat = (_tileInfo.neLat - _tileInfo.swLat) / _tileInfo.gridSizeLat;
    _cellSizeLon = (_tileInfo.neLon - _tileInfo.swLon) / _tileInfo.gridSizeLon;

    qCDebug(TerrainTileLog) << this << "TileInfo: south west:" << _tileInfo.swLat << _tileInfo.swLon;
    qCDebug(TerrainTileLog) << this << "TileInfo: north east:" << _tileInfo.neLat << _tileInfo.neLon;
    qCDebug(TerrainTileLog) << this << "TileInfo: dimensions:" << _tileInfo.gridSizeLat << "by" << _tileInfo.gridSizeLon;
    qCDebug(TerrainTileLog) << this << "TileInfo: min, max, avg:" << _tileInfo.minElevation << _tileInfo.maxElevation << _tileInfo.avgElevation;
    qCDebug(TerrainTileLog) << this << "TileInfo: cell size:" << _cellSizeLat << _cellSizeLon;

    _elevationData.resize(_tileInfo.gridSizeLat);
    for (int k = 0; k < _tileInfo.gridSizeLat; k++) {
        _elevationData[k].resize(_tileInfo.gridSizeLon);
    }

    int valueIndex = 0;
    const int16_t* const pTileData = reinterpret_cast<const int16_t*>(&reinterpret_cast<const uint8_t*>(byteArray.constData())[cTileHeaderBytes]);
    for (int i = 0; i < _tileInfo.gridSizeLat; i++) {
        for (int j = 0; j < _tileInfo.gridSizeLon; j++) {
            _elevationData[i][j] = pTileData[valueIndex++];
        }
    }

    _isValid = true;
}

TerrainTile::~TerrainTile()
{
    qCDebug(TerrainTileLog) << this;
}

double TerrainTile::elevation(const QGeoCoordinate &coordinate) const
{
    if (!_isValid) {
        qCWarning(TerrainTileLog) << this << "Request for elevation, but tile is invalid.";
        return qQNaN();
    }

    const double latDeltaSw = coordinate.latitude() - _tileInfo.swLat;
    const double lonDeltaSw = coordinate.longitude() - _tileInfo.swLon;

    const int16_t latIndex = qFloor(latDeltaSw / _cellSizeLat);
    const int16_t lonIndex = qFloor(lonDeltaSw / _cellSizeLon);

    const bool latIndexInvalid = (latIndex < 0) || (latIndex > (_tileInfo.gridSizeLat - 1));
    const bool lonIndexInvalid = (lonIndex < 0) || (lonIndex > (_tileInfo.gridSizeLon - 1));

    if (latIndexInvalid || lonIndexInvalid) {
        qCWarning(TerrainTileLog) << this << "Internal error: coordinate" << coordinate << "outside tile bounds";
        return qQNaN();
    }

    if ((latIndex >= _elevationData.size()) || (lonIndex >= _elevationData[latIndex].size())) {
        qCWarning(TerrainTileLog).noquote() << this << "Internal error: _elevationData size inconsistent _tileInfo << coordinate" << coordinate
            << "\n\t_tileInfo.gridSizeLat:" << _tileInfo.gridSizeLat << "_tileInfo.gridSizeLon:" << _tileInfo.gridSizeLon
            << "\n\t_data.size():" << _elevationData.size() << "_elevationData[latIndex].size():" << _elevationData[latIndex].size();
        return qQNaN();
    }

    const int16_t elevation = _elevationData[latIndex][lonIndex];
    if (elevation < _tileInfo.minElevation) {
        qCWarning(TerrainTileLog) << this << "Warning: elevation read is below min elevation in tile:" << elevation << "<" << _tileInfo.minElevation;
    } else if (elevation > _tileInfo.maxElevation) {
        qCWarning(TerrainTileLog) << this << "Warning: elevation read is above max elevation in tile:" << elevation << ">" << _tileInfo.maxElevation;
    }

    qCDebug(TerrainTileLog) << this << "latIndex, lonIndex:" << latIndex << lonIndex << "elevation:" << elevation;

    return static_cast<double>(elevation);
}
