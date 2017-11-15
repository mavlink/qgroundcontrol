#include "TerrainTile.h"
#include "JsonHelper.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

QGC_LOGGING_CATEGORY(TerrainTileLog, "TerrainTileLog")

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
    , _isValid(false)
{

}

TerrainTile::~TerrainTile()
{
}

TerrainTile::TerrainTile(QJsonDocument document)
    : _minElevation(-1.0)
    , _maxElevation(-1.0)
    , _avgElevation(-1.0)
    , _isValid(false)
{
    if (!document.isObject()) {
        qCDebug(TerrainTileLog) << "Terrain tile json doc is no object";
        return;
    }
    QJsonObject rootObject = document.object();

    QString errorString;
    QList<JsonHelper::KeyValidateInfo> rootVersionKeyInfoList = {
        { _jsonStatusKey, QJsonValue::String, true },
        { _jsonDataKey, QJsonValue::String, true },
    };
    if (!JsonHelper::validateKeys(rootObject, rootVersionKeyInfoList, errorString)) {
        qCDebug(TerrainTileLog) << "Error in reading json: " << errorString;
        return false;
    }

    if (rootObject[_jsonStatusKey].toString() != "success") {
        qCDebug(TerrainTileLog) << "Invalid terrain tile.";
        return;
    }
    const QJsonObject& dataObject = rootObject[_jsonDataKey].toObject();
    QList<JsonHelper::KeyValidateInfo> dataVersionKeyInfoList = {
        { _jsonBoundsKey, QJsonValue::Object, true },
        { _jsonStatsKey, QJsonValue::Object, true },
        { _jsonCarpetKey, QJsonValue::Array, true },
    };
    if (!JsonHelper::validateKeys(dataObject, dataVersionKeyInfoList, errorString)) {
        qCDebug(TerrainTileLog) << "Error in reading json: " << errorString;
        return false;
    }

    // Bounds
    const QJsonObject& boundsObject = dataObject[_jsonBoundsKey].toObject();
    QList<JsonHelper::KeyValidateInfo> boundsVersionKeyInfoList = {
        { _jsonSouthWestKey, QJsonValue::Array, true },
        { _jsonNorthEastKey, QJsonValue::Array, true },
    };
    if (!JsonHelper::validateKeys(boundsObject, boundsVersionKeyInfoList, errorString)) {
        qCDebug(TerrainTileLog) << "Error in reading json: " << errorString;
        return false;
    }
    const QJsonArray& swArray = boundsObject[_jsonSouthWestKey].toArray();
    const QJsonArray& neArray = boundsObject[_jsonNorthEastKey].toArray();
    if (swArray.count() < 2 || neArray.count() < 2 ) {
        qCDebug(TerrainTileLog) << "Incomplete bounding location";
        return;
    }
    _southWest.setLatitude(swArray[0].toDouble());
    _southWest.setLongitude(swArray[1].toDouble());
    _northEast.setLatitude(neArray[0].toDouble());
    _northEast.setLongitude(neArray[1].toDouble());

    // Stats
    const QJsonObject& statsObject = dataObject[_jsonBoundsKey].toObject();
    QList<JsonHelper::KeyValidateInfo> statsVersionKeyInfoList = {
        { _jsonMaxElevationKey, QJsonValue::Double, true },
        { _jsonMinElevationKey, QJsonValue::Double, true },
        { _jsonAvgElevationKey, QJsonValue::Double, true },
    };
    if (!JsonHelper::validateKeys(statsObject, statsVersionKeyInfoList, errorString)) {
        qCDebug(TerrainTileLog) << "Error in reading json: " << errorString;
        return false;
    }
    _maxElevation = statsObject[_jsonMaxElevationKey].toInt();
    _minElevation = statsObject[_jsonMinElevationKey].toInt();
    _avgElevation = statsObject[_jsonAvgElevationKey].toInt();

    // Carpet
    const QJsonArray& carpetArray = dataObject[_jsonCarpetKey].toArray();
    if (carpetArray.count() != _gridSize) {
        qCDebug(TerrainTileLog) << "Expected array of " << _gridSize << ", instead got " << carpetArray.count();
        return;
    }
    for (int i = 0; i < _gridSize; i++) {
        const QJsonArray& row = carpetArray[i].toArray();
        if (row.count() != _gridSize) {
            qCDebug(TerrainTileLog) << "Expected row array of " << _gridSize << ", instead got " << row.count();
            return;
        }
        for (int j = 0; j < _gridSize; j++) {
            _data[i][j] = row[j].toDouble();
        }
    }
    _isValid = true;
}

bool TerrainTile::isIn(const QGeoCoordinate& coordinate) const
{
    if (!_isValid) {
        qCDebug(TerrainTileLog) << "isIn requested, but tile not valid";
        return false;
    }
    bool ret = coord.latitude() >= _southWest.longitude() && coord.longitude() >= _southWest.longitude() &&
               coord.latitude() <= _northEast.longitude() && coord.longitude() <= _northEast.longitude();
    qCDebug(TerrainTileLog) << "Checking isIn: " << coord << " , in sw " << _southWest << " , ne " << _northEast << ": " << ret;
    return ret;
}

float TerrainTile::elevation(const QGeoCoordinate& coordinate) const
{
    if (_isValid) {
        qCDebug << "elevation: " << coord << " , in sw " << _southWest << " , ne " << _northEast;
        // Get the index at resolution of 1 arc second
        int indexLat = std::round((coord.latitude() - _southWest.latitude()) / _srtm1Increment);
        int indexLon = std::round((coord.longitude() - _southWest.longitude()) / _srtm1Increment);
        qCDebug << "indexLat:indexLon" << indexLat << indexLon; // TODO (birchera): Move this down to the next debug output, once this is all properly working.
        Q_ASSERT(indexLat >= 0);
        Q_ASSERT(indexLat < _gridSize);
        Q_ASSERT(indexLon >= 0);
        Q_ASSERT(indexLon < _gridSize);
        qCDebug << "elevation" << _data[indexLat][indexLon];
        return _data[indexLat][indexLon];
    } else {
        qCDebug(TerrainTileLog) << "Asking for elevation, but no valid data.";
        return -1.0;
    }
}
