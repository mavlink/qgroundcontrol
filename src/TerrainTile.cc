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

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDataStream>

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

TerrainTile::TerrainTile()
    : _minElevation(-1.0)
    , _maxElevation(-1.0)
    , _avgElevation(-1.0)
    , _data(nullptr)
    , _gridSizeLat(-1)
    , _gridSizeLon(-1)
    , _isValid(false)
{

}

TerrainTile::~TerrainTile()
{
    if (_data) {
        for (int i = 0; i < _gridSizeLat; i++) {
            delete _data[i];
        }
        delete _data;
        _data = nullptr;
    }
}

TerrainTile::TerrainTile(QByteArray byteArray)
    : _minElevation(-1.0)
    , _maxElevation(-1.0)
    , _avgElevation(-1.0)
    , _data(nullptr)
    , _gridSizeLat(-1)
    , _gridSizeLon(-1)
    , _isValid(false)
{
    int cTileHeaderBytes = static_cast<int>(sizeof(TileInfo_t));
    int cTileBytesAvailable = byteArray.size();

    if (cTileBytesAvailable < cTileHeaderBytes) {
        qWarning() << "Terrain tile binary data too small for TileInfo_s header";
        return;
    }

    const TileInfo_t* tileInfo = reinterpret_cast<const TileInfo_t*>(byteArray.constData());
    _southWest.setLatitude(tileInfo->swLat);
    _southWest.setLongitude(tileInfo->swLon);
    _northEast.setLatitude(tileInfo->neLat);
    _northEast.setLongitude(tileInfo->neLon);
    _minElevation = tileInfo->minElevation;
    _maxElevation = tileInfo->maxElevation;
    _avgElevation = tileInfo->avgElevation;
    _gridSizeLat = tileInfo->gridSizeLat;
    _gridSizeLon = tileInfo->gridSizeLon;

    qCDebug(TerrainTileLog) << "Loading terrain tile: " << _southWest << " - " << _northEast;
    qCDebug(TerrainTileLog) << "min:max:avg:sizeLat:sizeLon" << _minElevation << _maxElevation << _avgElevation << _gridSizeLat << _gridSizeLon;

    int cTileDataBytes = static_cast<int>(sizeof(int16_t)) * _gridSizeLat * _gridSizeLon;
    if (cTileBytesAvailable < cTileHeaderBytes + cTileDataBytes) {
        qWarning() << "Terrain tile binary data too small for tile data";
        return;
    }

    _data = new int16_t*[_gridSizeLat];
    for (int k = 0; k < _gridSizeLat; k++) {
        _data[k] = new int16_t[_gridSizeLon];
    }

    int valueIndex = 0;
    const int16_t* pTileData = reinterpret_cast<const int16_t*>(&reinterpret_cast<const uint8_t*>(byteArray.constData())[cTileHeaderBytes]);
    for (int i = 0; i < _gridSizeLat; i++) {
        for (int j = 0; j < _gridSizeLon; j++) {
            _data[i][j] = pTileData[valueIndex++];
        }
    }

    _isValid = true;

    return;
}


bool TerrainTile::isIn(const QGeoCoordinate& coordinate) const
{
    if (!_isValid) {
        qCWarning(TerrainTileLog) << "isIn requested, but tile not valid";
        return false;
    }
    bool ret = coordinate.latitude() >= _southWest.latitude() && coordinate.longitude() >= _southWest.longitude() &&
            coordinate.latitude() <= _northEast.latitude() && coordinate.longitude() <= _northEast.longitude();
    qCDebug(TerrainTileLog) << "Checking isIn: " << coordinate << " , in sw " << _southWest << " , ne " << _northEast << ": " << ret;
    return ret;
}

double TerrainTile::elevation(const QGeoCoordinate& coordinate) const
{
    if (_isValid) {
        qCDebug(TerrainTileLog) << "elevation: " << coordinate << " , in sw " << _southWest << " , ne " << _northEast;
        // Get the index at resolution of 1 arc second
        int indexLat = _latToDataIndex(coordinate.latitude());
        int indexLon = _lonToDataIndex(coordinate.longitude());
        if (indexLat == -1 || indexLon == -1) {
            qCWarning(TerrainTileLog) << "Internal error indexLat:indexLon == -1" << indexLat << indexLon;
            return qQNaN();
        }
        qCDebug(TerrainTileLog) << "indexLat:indexLon" << indexLat << indexLon << "elevation" << _data[indexLat][indexLon];
        return static_cast<double>(_data[indexLat][indexLon]);
    } else {
        qCWarning(TerrainTileLog) << "Asking for elevation, but no valid data.";
        return qQNaN();
    }
}

QGeoCoordinate TerrainTile::centerCoordinate(void) const
{
    return _southWest.atDistanceAndAzimuth(_southWest.distanceTo(_northEast) / 2.0, _southWest.azimuthTo(_northEast));
}

QByteArray TerrainTile::serialize(QByteArray input)
{
    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(input, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        QByteArray emptyArray;
        return emptyArray;
    }

    if (!document.isObject()) {
        qCDebug(TerrainTileLog) << "Terrain tile json doc is no object";
        QByteArray emptyArray;
        return emptyArray;
    }
    QJsonObject rootObject = document.object();

    QString errorString;
    QList<JsonHelper::KeyValidateInfo> rootVersionKeyInfoList = {
        { _jsonStatusKey, QJsonValue::String, true },
        { _jsonDataKey,   QJsonValue::Object, true },
    };
    if (!JsonHelper::validateKeys(rootObject, rootVersionKeyInfoList, errorString)) {
        qCDebug(TerrainTileLog) << "Error in reading json: " << errorString;
        QByteArray emptyArray;
        return emptyArray;
    }

    if (rootObject[_jsonStatusKey].toString() != "success") {
        qCDebug(TerrainTileLog) << "Invalid terrain tile.";
        QByteArray emptyArray;
        return emptyArray;
    }
    const QJsonObject& dataObject = rootObject[_jsonDataKey].toObject();
    QList<JsonHelper::KeyValidateInfo> dataVersionKeyInfoList = {
        { _jsonBoundsKey, QJsonValue::Object, true },
        { _jsonStatsKey,  QJsonValue::Object, true },
        { _jsonCarpetKey, QJsonValue::Array, true },
    };
    if (!JsonHelper::validateKeys(dataObject, dataVersionKeyInfoList, errorString)) {
        qCDebug(TerrainTileLog) << "Error in reading json: " << errorString;
        QByteArray emptyArray;
        return emptyArray;
    }

    // Bounds
    const QJsonObject& boundsObject = dataObject[_jsonBoundsKey].toObject();
    QList<JsonHelper::KeyValidateInfo> boundsVersionKeyInfoList = {
        { _jsonSouthWestKey, QJsonValue::Array, true },
        { _jsonNorthEastKey, QJsonValue::Array, true },
    };
    if (!JsonHelper::validateKeys(boundsObject, boundsVersionKeyInfoList, errorString)) {
        qCDebug(TerrainTileLog) << "Error in reading json: " << errorString;
        QByteArray emptyArray;
        return emptyArray;
    }
    const QJsonArray& swArray = boundsObject[_jsonSouthWestKey].toArray();
    const QJsonArray& neArray = boundsObject[_jsonNorthEastKey].toArray();
    if (swArray.count() < 2 || neArray.count() < 2 ) {
        qCDebug(TerrainTileLog) << "Incomplete bounding location";
        QByteArray emptyArray;
        return emptyArray;
    }

    // Stats
    const QJsonObject& statsObject = dataObject[_jsonStatsKey].toObject();
    QList<JsonHelper::KeyValidateInfo> statsVersionKeyInfoList = {
        { _jsonMinElevationKey, QJsonValue::Double, true },
        { _jsonMaxElevationKey, QJsonValue::Double, true },
        { _jsonAvgElevationKey, QJsonValue::Double, true },
    };
    if (!JsonHelper::validateKeys(statsObject, statsVersionKeyInfoList, errorString)) {
        qCDebug(TerrainTileLog) << "Error in reading json: " << errorString;
        QByteArray emptyArray;
        return emptyArray;
    }

    // Carpet
    const QJsonArray& carpetArray = dataObject[_jsonCarpetKey].toArray();
    int gridSizeLat = carpetArray.count();
    int gridSizeLon = carpetArray[0].toArray().count();
    qCDebug(TerrainTileLog) << "Received tile has size in latitude direction: " << gridSizeLat;
    qCDebug(TerrainTileLog) << "Received tile has size in longitued direction: " << gridSizeLon;

    TileInfo_t tileInfo;

    tileInfo.swLat = swArray[0].toDouble();
    tileInfo.swLon = swArray[1].toDouble();
    tileInfo.neLat = neArray[0].toDouble();
    tileInfo.neLon = neArray[1].toDouble();
    tileInfo.minElevation = static_cast<int16_t>(statsObject[_jsonMinElevationKey].toInt());
    tileInfo.maxElevation = static_cast<int16_t>(statsObject[_jsonMaxElevationKey].toInt());
    tileInfo.avgElevation = statsObject[_jsonAvgElevationKey].toDouble();
    tileInfo.gridSizeLat = static_cast<int16_t>(gridSizeLat);
    tileInfo.gridSizeLon = static_cast<int16_t>(gridSizeLon);

    int cTileHeaderBytes = static_cast<int>(sizeof(TileInfo_t));
    int cTileDataBytes = static_cast<int>(sizeof(int16_t)) * gridSizeLat * gridSizeLon;

    QByteArray byteArray(cTileHeaderBytes + cTileDataBytes, 0);

    TileInfo_t* pTileInfo = reinterpret_cast<TileInfo_t*>(byteArray.data());
    int16_t*    pTileData = reinterpret_cast<int16_t*>(&reinterpret_cast<uint8_t*>(byteArray.data())[cTileHeaderBytes]);

    *pTileInfo = tileInfo;

    int valueIndex = 0;
    for (int i = 0; i < gridSizeLat; i++) {
        const QJsonArray& row = carpetArray[i].toArray();
        if (row.count() < gridSizeLon) {
            qCDebug(TerrainTileLog) << "Expected row array of " << gridSizeLon << ", instead got " << row.count();
            QByteArray emptyArray;
            return emptyArray;
        }
        for (int j = 0; j < gridSizeLon; j++) {
            pTileData[valueIndex++] = static_cast<int16_t>(row[j].toDouble());
        }
    }

    return byteArray;
}


int TerrainTile::_latToDataIndex(double latitude) const
{
    if (isValid() && _southWest.isValid() && _northEast.isValid()) {
        return qRound((latitude - _southWest.latitude()) / (_northEast.latitude() - _southWest.latitude()) * (_gridSizeLat - 1));
    } else {
        qCWarning(TerrainTileLog) << "TerrainTile::_latToDataIndex internal error" << isValid() << _southWest.isValid() << _northEast.isValid();
        return -1;
    }
}

int TerrainTile::_lonToDataIndex(double longitude) const
{
    if (isValid() && _southWest.isValid() && _northEast.isValid()) {
        return qRound((longitude - _southWest.longitude()) / (_northEast.longitude() - _southWest.longitude()) * (_gridSizeLon - 1));
    } else {
        qCWarning(TerrainTileLog) << "TerrainTile::_lonToDataIndex internal error" << isValid() << _southWest.isValid() << _northEast.isValid();
        return -1;
    }
}
