#include "TerrainTile.h"
#include "JsonHelper.h"
#include "QGCMapEngine.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDataStream>

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


TerrainTile::TerrainTile(QByteArray byteArray)
    : _minElevation(-1.0)
    , _maxElevation(-1.0)
    , _avgElevation(-1.0)
    , _data(NULL)
    , _gridSizeLat(-1)
    , _gridSizeLon(-1)
    , _isValid(false)
{
    QDataStream stream(byteArray);

    float lat,lon;
    if (stream.atEnd()) {
        qWarning() << "Terrain tile binary data does not contain all data";
        return;
    }
    stream >> lat;
    if (stream.atEnd()) {
        qWarning() << "Terrain tile binary data does not contain all data";
        return;
    }
    stream >> lon;
    _southWest.setLatitude(lat);
    _southWest.setLongitude(lon);

    if (stream.atEnd()) {
        qWarning() << "Terrain tile binary data does not contain all data";
        return;
    }
    stream >> lat;
    if (stream.atEnd()) {
        qWarning() << "Terrain tile binary data does not contain all data";
        return;
    }
    stream >> lon;
    _northEast.setLatitude(lat);
    _northEast.setLongitude(lon);

    if (stream.atEnd()) {
        qWarning() << "Terrain tile binary data does not contain all data";
        return;
    }
    stream >> _minElevation;
    if (stream.atEnd()) {
        qWarning() << "Terrain tile binary data does not contain all data";
        return;
    }
    stream >> _maxElevation;
    if (stream.atEnd()) {
        qWarning() << "Terrain tile binary data does not contain all data";
        return;
    }
    stream >> _avgElevation;
    if (stream.atEnd()) {
        qWarning() << "Terrain tile binary data does not contain all data";
        return;
    }
    stream >> _gridSizeLat;
    if (stream.atEnd()) {
        qWarning() << "Terrain tile binary data does not contain all data";
        return;
    }
    stream >> _gridSizeLon;

    qCDebug(TerrainTileLog) << "Loading terrain tile: " << _southWest << " - " << _northEast;
    qCDebug(TerrainTileLog) << "min:max:avg:sizeLat:sizeLon" << _minElevation << _maxElevation << _avgElevation << _gridSizeLat << _gridSizeLon;

    for (int i = 0; i < _gridSizeLat; i++) {
        if (i == 0) {
            _data = new int16_t*[_gridSizeLat];
            for (int k = 0; k < _gridSizeLat; k++) {
                _data[k] = new int16_t[_gridSizeLon];
            }
        }
        for (int j = 0; j < _gridSizeLon; j++) {
            if (stream.atEnd()) {
                qWarning() << "Terrain tile binary data does not contain all data";
                return;
            }
            stream >> _data[i][j];
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
        if (indexLat == -1 || indexLon == -1) {
            qCWarning(TerrainTileLog) << "Internal error indexLat:indexLon == -1" << indexLat << indexLon;
            return -1.0;
        }
        qCDebug(TerrainTileLog) << "indexLat:indexLon" << indexLat << indexLon << "elevation" << _data[indexLat][indexLon];
        return static_cast<double>(_data[indexLat][indexLon]);
    } else {
        qCDebug(TerrainTileLog) << "Asking for elevation, but no valid data.";
        return -1.0;
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

    QByteArray byteArray;
    QDataStream stream(&byteArray, QIODevice::WriteOnly);
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
    stream << static_cast<float>(swArray[0].toDouble());
    stream << static_cast<float>(swArray[1].toDouble());
    stream << static_cast<float>(neArray[0].toDouble());
    stream << static_cast<float>(neArray[1].toDouble());

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
    stream << static_cast<int16_t>(statsObject[_jsonMinElevationKey].toInt());
    stream << static_cast<int16_t>(statsObject[_jsonMaxElevationKey].toInt());
    stream << static_cast<float>(statsObject[_jsonAvgElevationKey].toDouble());

    // Carpet
    const QJsonArray& carpetArray = dataObject[_jsonCarpetKey].toArray();
    int gridSizeLat = carpetArray.count();
    stream << static_cast<int16_t>(gridSizeLat);
    int gridSizeLon = 0;
    qCDebug(TerrainTileLog) << "Received tile has size in latitude direction: " << carpetArray.count();
    for (int i = 0; i < gridSizeLat; i++) {
        const QJsonArray& row = carpetArray[i].toArray();
        if (i == 0) {
            gridSizeLon = row.count();
            stream << static_cast<int16_t>(gridSizeLon);
            qCDebug(TerrainTileLog) << "Received tile has size in longitued direction: " << row.count();
        }
        if (row.count() < gridSizeLon) {
            qCDebug(TerrainTileLog) << "Expected row array of " << gridSizeLon << ", instead got " << row.count();
            QByteArray emptyArray;
            return emptyArray;
        }
        for (int j = 0; j < gridSizeLon; j++) {
            stream << static_cast<int16_t>(row[j].toDouble());
        }
    }

    return byteArray;
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
