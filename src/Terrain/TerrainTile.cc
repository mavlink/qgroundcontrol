/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "TerrainTile.h"
#include "JsonHelper.h"
#include "QGCMapEngine.h"
#include "QGC.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDataStream>
#include <QtMath>

QGC_LOGGING_CATEGORY(TerrainTileLog, "TerrainTileLog");

const char*  TerrainTile::_jsonStatusKey        = "status";
const char*  TerrainTile::_jsonDataKey          = "data";
const char*  TerrainTile::_jsonBoundsKey        = "bounds";
const char*  TerrainTile::_jsonSouthWestKey     = "sw";
const char*  TerrainTile::_jsonNorthEastKey     = "ne";
const char*  TerrainTile::_jsonStatsKey         = "stats";
const char*  TerrainTile::_jsonMaxElevationKey  = "max";
const char*  TerrainTile::_jsonMinElevationKey  = "min";
const char*  TerrainTile::_jsonAvgElevationKey  = "avg";
const char*  TerrainTile::_jsonCarpetKey        = "carpet";

TerrainTile::TerrainTile(const QByteArray& byteArray)
{
    // Copy tile info
    _tileInfo = *reinterpret_cast<const TileInfo_t*>(byteArray.constData());

    // Check feasibility
    if ((_tileInfo.neLon - _tileInfo.swLon) < 0.0 || (_tileInfo.neLat - _tileInfo.swLat) < 0.0) {
        qCWarning(TerrainTileLog) << this << "Tile extent is infeasible";
        _isValid = false;
        return;
    }

    _cellSizeLat = (_tileInfo.neLat - _tileInfo.swLat) / _tileInfo.gridSizeLat;
    _cellSizeLon = (_tileInfo.neLon - _tileInfo.swLon) / _tileInfo.gridSizeLon;

    qCDebug(TerrainTileLog) << this << "TileInfo: south west:    " << _tileInfo.swLat << _tileInfo.swLon;
    qCDebug(TerrainTileLog) << this << "TileInfo: north east:    " << _tileInfo.neLat << _tileInfo.neLon;
    qCDebug(TerrainTileLog) << this << "TileInfo: dimensions:    " << _tileInfo.gridSizeLat << "by" << _tileInfo.gridSizeLat;
    qCDebug(TerrainTileLog) << this << "TileInfo: min, max, avg: " << _tileInfo.minElevation << _tileInfo.maxElevation << _tileInfo.avgElevation;
    qCDebug(TerrainTileLog) << this << "TileInfo: cell size:     " << _cellSizeLat << _cellSizeLon;

    int cTileHeaderBytes = static_cast<int>(sizeof(TileInfo_t));
    int cTileBytesAvailable = byteArray.size();

    if (cTileBytesAvailable < cTileHeaderBytes) {
        qCWarning(TerrainTileLog) << "Terrain tile binary data too small for TileInfo_s header";
        return;
    }

    int cTileDataBytes = static_cast<int>(sizeof(int16_t)) * _tileInfo.gridSizeLat * _tileInfo.gridSizeLon;
    if (cTileBytesAvailable < cTileHeaderBytes + cTileDataBytes) {
        qCWarning(TerrainTileLog) << "Terrain tile binary data too small for tile data";
        return;
    }

    _data = new int16_t*[_tileInfo.gridSizeLat];
    for (int k = 0; k < _tileInfo.gridSizeLat; k++) {
        _data[k] = new int16_t[_tileInfo.gridSizeLon];
    }

    int valueIndex = 0;
    const int16_t* pTileData = reinterpret_cast<const int16_t*>(&reinterpret_cast<const uint8_t*>(byteArray.constData())[cTileHeaderBytes]);
    for (int i = 0; i < _tileInfo.gridSizeLat; i++) {
        for (int j = 0; j < _tileInfo.gridSizeLon; j++) {
            _data[i][j] = pTileData[valueIndex++];
        }
    }

    _isValid = true;
}

TerrainTile::~TerrainTile()
{
    if (!_data) {
        return;
    }

    for (unsigned i = 0; i < static_cast<unsigned>(_tileInfo.gridSizeLat); i++) {
        delete[] _data[i];
    }

    delete[] _data;
}

double TerrainTile::elevation(const QGeoCoordinate& coordinate) const
{
    if (!_isValid || !_data) {
        qCWarning(TerrainTileLog) << this << "Request for elevation, but tile is invalid.";
        return qQNaN();
    }

    const double latDeltaSw = coordinate.latitude() - _tileInfo.swLat;
    const double lonDeltaSw = coordinate.longitude() - _tileInfo.swLon;

    const int16_t latIndex = qFloor(latDeltaSw / _cellSizeLat);
    const int16_t lonIndex = qFloor(lonDeltaSw / _cellSizeLon);

    const bool latIndexInvalid = latIndex < 0 || latIndex > (_tileInfo.gridSizeLat - 1);
    const bool lonIndexInvalid = lonIndex < 0 || lonIndex > (_tileInfo.gridSizeLon - 1);

    if (latIndexInvalid || lonIndexInvalid) {
        qCWarning(TerrainTileLog) << this << "Internal error: coordinate" << coordinate << "outside tile bounds";
        return qQNaN();
    }

    const auto elevation = _data[latIndex][lonIndex];

    // Print warning if elevation is outside min/max of tile meta data
    if (elevation < _tileInfo.minElevation) {
        qCWarning(TerrainTileLog) << this << "Warning: elevation read is below min elevation in tile:" << elevation << "<" << _tileInfo.minElevation;
    }
    else if (elevation > _tileInfo.maxElevation) {
        qCWarning(TerrainTileLog) << this << "Warning: elevation read is above max elevation in tile:" << elevation << ">" << _tileInfo.maxElevation;
    }

#ifdef QT_DEBUG
    qCDebug(TerrainTileLog) << this << "latIndex, lonIndex:" << latIndex << lonIndex << "elevation:" << elevation;
#endif

    return static_cast<double>(elevation);
}

QByteArray TerrainTile::serializeFromAirMapJson(const QByteArray& input)
{
    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(input, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qCWarning(TerrainTileLog) << "TerrainTile::serializeFromAirMapJson: Terrain tile json doc parse error" << parseError.errorString();
        return QByteArray();
    }

    if (!document.isObject()) {
        qCWarning(TerrainTileLog) << "TerrainTile::serializeFromAirMapJson: Terrain tile json doc is no object";
        return QByteArray();
    }
    QJsonObject rootObject = document.object();

    QString errorString;
    QList<JsonHelper::KeyValidateInfo> rootVersionKeyInfoList = {
        { _jsonStatusKey, QJsonValue::String, true },
        { _jsonDataKey,   QJsonValue::Object, true },
    };
    if (!JsonHelper::validateKeys(rootObject, rootVersionKeyInfoList, errorString)) {
        qCWarning(TerrainTileLog) << "TerrainTile::serializeFromAirMapJson: Error in reading json: " << errorString;
        return QByteArray();
    }

    if (rootObject[_jsonStatusKey].toString() != "success") {
        qCWarning(TerrainTileLog) << "TerrainTile::serializeFromAirMapJson: Invalid terrain tile.";
        return QByteArray();
    }
    const QJsonObject& dataObject = rootObject[_jsonDataKey].toObject();
    QList<JsonHelper::KeyValidateInfo> dataVersionKeyInfoList = {
        { _jsonBoundsKey, QJsonValue::Object, true },
        { _jsonStatsKey,  QJsonValue::Object, true },
        { _jsonCarpetKey, QJsonValue::Array, true },
    };
    if (!JsonHelper::validateKeys(dataObject, dataVersionKeyInfoList, errorString)) {
        qCWarning(TerrainTileLog) << "TerrainTile::serializeFromAirMapJson: Error in reading json: " << errorString;
        return QByteArray();
    }

    // Bounds
    const QJsonObject& boundsObject = dataObject[_jsonBoundsKey].toObject();
    QList<JsonHelper::KeyValidateInfo> boundsVersionKeyInfoList = {
        { _jsonSouthWestKey, QJsonValue::Array, true },
        { _jsonNorthEastKey, QJsonValue::Array, true },
    };
    if (!JsonHelper::validateKeys(boundsObject, boundsVersionKeyInfoList, errorString)) {
        qCWarning(TerrainTileLog) << "TerrainTile::serializeFromAirMapJson: Error in reading json: " << errorString;
        return QByteArray();
    }
    const QJsonArray& swArray = boundsObject[_jsonSouthWestKey].toArray();
    const QJsonArray& neArray = boundsObject[_jsonNorthEastKey].toArray();
    if (swArray.count() < 2 || neArray.count() < 2 ) {
        qCWarning(TerrainTileLog) << "TerrainTile::serializeFromAirMapJson: Incomplete bounding location";
        return QByteArray();
    }

    const double swLat = swArray[0].toDouble();
    const double swLon = swArray[1].toDouble();
    const double neLat = neArray[0].toDouble();
    const double neLon = neArray[1].toDouble();

    qCDebug(TerrainTileLog) << "Serialize: swArray: south west:    " << (40.42 - swLat) << (-3.23 - swLon);
    qCDebug(TerrainTileLog) << "Serialize: neArray: north east:    " << neLat << neLon;

    // Stats
    const QJsonObject& statsObject = dataObject[_jsonStatsKey].toObject();
    QList<JsonHelper::KeyValidateInfo> statsVersionKeyInfoList = {
        { _jsonMinElevationKey, QJsonValue::Double, true },
        { _jsonMaxElevationKey, QJsonValue::Double, true },
        { _jsonAvgElevationKey, QJsonValue::Double, true },
    };
    if (!JsonHelper::validateKeys(statsObject, statsVersionKeyInfoList, errorString)) {
        qCWarning(TerrainTileLog) << "TerrainTile::serializeFromAirMapJson: Error in reading json: " << errorString;
        return QByteArray();
    }

    const QJsonArray& carpetArray = dataObject[_jsonCarpetKey].toArray();

    // Tile meta data
    TerrainTile::TileInfo_t tileInfo;
    tileInfo.swLat = swArray[0].toDouble();
    tileInfo.swLon = swArray[1].toDouble();
    tileInfo.neLat = neArray[0].toDouble();
    tileInfo.neLon = neArray[1].toDouble();
    tileInfo.minElevation = static_cast<int16_t>(statsObject[_jsonMinElevationKey].toInt());
    tileInfo.maxElevation = static_cast<int16_t>(statsObject[_jsonMaxElevationKey].toInt());
    tileInfo.avgElevation = statsObject[_jsonAvgElevationKey].toDouble();
    tileInfo.gridSizeLat = static_cast<int16_t>(carpetArray.count());
    tileInfo.gridSizeLon = static_cast<int16_t>(carpetArray[0].toArray().count());

    qCDebug(TerrainTileLog) << "Serialize: TileInfo: south west:    " << tileInfo.swLat << tileInfo.swLon;
    qCDebug(TerrainTileLog) << "Serialize: TileInfo: north east:    " << tileInfo.neLat << tileInfo.neLon;

    const auto cTileNumHeaderBytes = static_cast<int>(sizeof(TileInfo_t));
    const auto cTileNumDataBytes = static_cast<int>(sizeof(int16_t)) * tileInfo.gridSizeLat * tileInfo.gridSizeLon;

    QByteArray res;
    res.resize(cTileNumHeaderBytes + cTileNumDataBytes);

    TileInfo_t* pTileInfo = reinterpret_cast<TileInfo_t*>(res.data());
    int16_t*    pTileData = reinterpret_cast<int16_t*>(&reinterpret_cast<uint8_t*>(res.data())[cTileNumHeaderBytes]);

    *pTileInfo = tileInfo;

    int valueIndex = 0;
    for (unsigned i = 0; i < static_cast<unsigned>(tileInfo.gridSizeLat); i++) {
        const QJsonArray& row = carpetArray[i].toArray();
        if (row.count() < tileInfo.gridSizeLon) {
            qCDebug(TerrainTileLog) << "Expected row array of " << tileInfo.gridSizeLon << ", instead got " << row.count();
            return QByteArray();
        }
        for (unsigned j = 0; j < static_cast<unsigned>(tileInfo.gridSizeLon); j++) {
            pTileData[valueIndex++] = static_cast<int16_t>(row[j].toDouble());
        }
    }

    return res;
}
