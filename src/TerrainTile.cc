#include "TerrainTile.h"
#include "JsonHelper.h"
#include "QGCMapEngine.h"

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
        { _jsonDataKey, QJsonValue::Object, true },
    };
    if (!JsonHelper::validateKeys(rootObject, rootVersionKeyInfoList, errorString)) {
        qCDebug(TerrainTileLog) << "Error in reading json: " << errorString;
        return;
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
        return;
    }

    // Bounds
    const QJsonObject& boundsObject = dataObject[_jsonBoundsKey].toObject();
    QList<JsonHelper::KeyValidateInfo> boundsVersionKeyInfoList = {
        { _jsonSouthWestKey, QJsonValue::Array, true },
        { _jsonNorthEastKey, QJsonValue::Array, true },
    };
    if (!JsonHelper::validateKeys(boundsObject, boundsVersionKeyInfoList, errorString)) {
        qCDebug(TerrainTileLog) << "Error in reading json: " << errorString;
        return;
    }
    const QJsonArray& swArray = boundsObject[_jsonSouthWestKey].toArray();
    const QJsonArray& neArray = boundsObject[_jsonNorthEastKey].toArray();
    if (swArray.count() < 2 || neArray.count() < 2 ) {
        qCDebug(TerrainTileLog) << "Incomplete bounding location";
        return;
    }
    _southWest.setLatitude(swArray[1].toDouble());
    _southWest.setLongitude(swArray[0].toDouble());
    _northEast.setLatitude(neArray[1].toDouble());
    _northEast.setLongitude(neArray[0].toDouble());

    // Stats
    const QJsonObject& statsObject = dataObject[_jsonStatsKey].toObject();
    QList<JsonHelper::KeyValidateInfo> statsVersionKeyInfoList = {
        { _jsonMaxElevationKey, QJsonValue::Double, true },
        { _jsonMinElevationKey, QJsonValue::Double, true },
        { _jsonAvgElevationKey, QJsonValue::Double, true },
    };
    if (!JsonHelper::validateKeys(statsObject, statsVersionKeyInfoList, errorString)) {
        qCDebug(TerrainTileLog) << "Error in reading json: " << errorString;
        return;
    }
    _maxElevation = statsObject[_jsonMaxElevationKey].toInt();
    _minElevation = statsObject[_jsonMinElevationKey].toInt();
    _avgElevation = statsObject[_jsonAvgElevationKey].toInt();

    // Carpet
    const QJsonArray& carpetArray = dataObject[_jsonCarpetKey].toArray();
    if (carpetArray.count() < gridSize) { // TODO (birchera): We always get 91x91 points, figure out why and where the exact location of the elev values are.
        qCDebug(TerrainTileLog) << "Expected array of " << gridSize << ", instead got " << carpetArray.count();
        return;
    }
    for (int i = 0; i < gridSize; i++) {
        const QJsonArray& row = carpetArray[i].toArray();
        if (row.count() < gridSize) { // TODO (birchera): the same as above
            qCDebug(TerrainTileLog) << "Expected row array of " << gridSize << ", instead got " << row.count();
            return;
        }
        for (int j = 0; j < gridSize; j++) {
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
    bool ret = coordinate.latitude() >= _southWest.latitude() && coordinate.longitude() >= _southWest.longitude() &&
               coordinate.latitude() <= _northEast.latitude() && coordinate.longitude() <= _northEast.longitude();
    qCDebug(TerrainTileLog) << "Checking isIn: " << coordinate << " , in sw " << _southWest << " , ne " << _northEast << ": " << ret;
    return ret;
}

float TerrainTile::elevation(const QGeoCoordinate& coordinate) const
{
    if (_isValid) {
        qCDebug(TerrainTileLog) << "elevation: " << coordinate << " , in sw " << _southWest << " , ne " << _northEast;
        // Get the index at resolution of 1 arc second
        int indexLat = round((coordinate.latitude() - _southWest.latitude()) * (gridSize - 1) / QGCMapEngine::srtm1TileSize);
        int indexLon = round((coordinate.longitude() - _southWest.longitude()) * (gridSize - 1) / QGCMapEngine::srtm1TileSize);
        qCDebug(TerrainTileLog) << "indexLat:indexLon" << indexLat << indexLon; // TODO (birchera): Move this down to the next debug output, once this is all properly working.
        Q_ASSERT(indexLat >= 0);
        Q_ASSERT(indexLat < gridSize);
        Q_ASSERT(indexLon >= 0);
        Q_ASSERT(indexLon < gridSize);
        qCDebug(TerrainTileLog) << "elevation" << _data[indexLat][indexLon];
        return _data[indexLat][indexLon];
    } else {
        qCDebug(TerrainTileLog) << "Asking for elevation, but no valid data.";
        return -1.0;
    }
}

QGeoCoordinate TerrainTile::centerCoordinate(void) const
{
    return _southWest.atDistanceAndAzimuth(_southWest.distanceTo(_northEast) / 2.0, _southWest.azimuthTo(_northEast));
}
