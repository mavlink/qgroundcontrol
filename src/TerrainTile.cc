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
    , _data(NULL)
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
        _data = NULL;
    }
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
    _southWest.setLatitude(swArray[0].toDouble());
    _southWest.setLongitude(swArray[1].toDouble());
    _northEast.setLatitude(neArray[0].toDouble());
    _northEast.setLongitude(neArray[1].toDouble());

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
    _gridSizeLat = carpetArray.count();
    qCDebug(TerrainTileLog) << "Received tile has size in latitude direction: " << carpetArray.count();
    for (int i = 0; i < _gridSizeLat; i++) {
        const QJsonArray& row = carpetArray[i].toArray();
        if (i == 0) {
            _gridSizeLon = row.count();
            qCDebug(TerrainTileLog) << "Received tile has size in longitued direction: " << row.count();
            if (_gridSizeLon > 0) {
                _data = new double*[_gridSizeLat];
            }
            for (int k = 0; k < _gridSizeLat; k++) {
                _data[k] = new double[_gridSizeLon];
            }
        }
        if (row.count() < _gridSizeLon) {
            qCDebug(TerrainTileLog) << "Expected row array of " << _gridSizeLon << ", instead got " << row.count();
            return;
        }
        for (int j = 0; j < _gridSizeLon; j++) {
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

double TerrainTile::elevation(const QGeoCoordinate& coordinate) const
{
    if (_isValid) {
        qCDebug(TerrainTileLog) << "elevation: " << coordinate << " , in sw " << _southWest << " , ne " << _northEast;
        // Get the index at resolution of 1 arc second
        int indexLat = _latToDataIndex(coordinate.latitude());
        int indexLon = _lonToDataIndex(coordinate.longitude());
        qCDebug(TerrainTileLog) << "indexLat:indexLon" << indexLat << indexLon << "elevation" << _data[indexLat][indexLon];
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

int TerrainTile::_latToDataIndex(double latitude) const
{
    if (isValid() && _southWest.isValid() && _northEast.isValid()) {
        return qRound((latitude - _southWest.latitude()) / (_northEast.latitude() - _southWest.latitude()) * (_gridSizeLat - 1));
    } else {
        return -1;
    }
}

int TerrainTile::_lonToDataIndex(double longitude) const
{
    if (isValid() && _southWest.isValid() && _northEast.isValid()) {
        return qRound((longitude - _southWest.longitude()) / (_northEast.longitude() - _southWest.longitude()) * (_gridSizeLon - 1));
    } else {
        return -1;
    }
}
