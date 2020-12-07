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

double TerrainTile::_swCornerClampedLatitude(double latitude) const
{
    double swCornerLat = _southWest.latitude();
    if (!QGC::fuzzyCompare(latitude, swCornerLat)) {
        latitude = swCornerLat;
    }
    return latitude;
}

double TerrainTile::_swCornerClampedLongitude (double longitude) const
{
    double swCornerLon = _southWest.longitude();
    if (!QGC::fuzzyCompare(longitude, swCornerLon)) {
        longitude = swCornerLon;
    }
    return longitude;
}

bool TerrainTile::isIn(const QGeoCoordinate& coordinate) const
{
    if (!_isValid) {
        qCWarning(TerrainTileLog) << "isIn: Internal Error - invalid tile";
        return false;
    }

    // We have to be careful of double value imprecision for lat/lon values.
    // Don't trust _northEast corner values because of this (they are set from airmap query response).
    // Calculate everything from swCorner values only

    double testLat      = _swCornerClampedLatitude(coordinate.latitude());
    double testLon      = _swCornerClampedLongitude(coordinate.longitude());
    double swCornerLat  = _southWest.latitude();
    double swCornerLon  = _southWest.longitude();
    double neCornerLat  = swCornerLat + (_gridSizeLat * tileSizeDegrees);
    double neCornerLon  = swCornerLon + (_gridSizeLon * tileSizeDegrees);

    bool coordinateIsInTile = testLat >= swCornerLat && testLon >= swCornerLon && testLat <= neCornerLat && testLon <= neCornerLon;
    qCDebug(TerrainTileLog) << "isIn - coordinateIsInTile::coordinate:testLast:testLon:swCornerlat:swCornerLon:neCornerLat:neCornerLon" << coordinateIsInTile << coordinate << testLat << testLon << swCornerLat << swCornerLon << neCornerLat << neCornerLon;

    return coordinateIsInTile;
}

double TerrainTile::elevation(const QGeoCoordinate& coordinate) const
{
    if (_isValid) {
        qCDebug(TerrainTileLog) << "elevation: " << coordinate << " , in sw " << _southWest << " , ne " << _northEast;
        // Get the index at resolution of 1 arc second
        int indexLat = _latToDataIndex(coordinate.latitude());
        int indexLon = _lonToDataIndex(coordinate.longitude());
        if (indexLat == -1 || indexLon == -1) {
            qCWarning(TerrainTileLog) << "elevation: Internal error - indexLat:indexLon == -1" << indexLat << indexLon;
            return qQNaN();
        }
        qCDebug(TerrainTileLog) << "elevation: indexLat:indexLon" << indexLat << indexLon << "elevation" << _data[indexLat][indexLon];
        return static_cast<double>(_data[indexLat][indexLon]);
    } else {
        qCWarning(TerrainTileLog) << "elevation: Internal error - invalid tile";
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

    // We require 1-arc second value spacing
    double neCornerLatExpected = tileInfo.swLat + ((tileInfo.gridSizeLat - 1) * tileValueSpacingDegrees);
    double neCornerLonExpected = tileInfo.swLon + ((tileInfo.gridSizeLon - 1) * tileValueSpacingDegrees);
    if (!QGC::fuzzyCompare(tileInfo.neLat, neCornerLatExpected) || !QGC::fuzzyCompare(tileInfo.neLon, neCornerLonExpected)) {
        qCWarning(TerrainTileLog) << QStringLiteral("serialize: Internal error - distance between values incorrect neExpected(%1, %2) neActual(%3, %4) sw(%5, %6) gridSize(%7, %8)")
                                     .arg(neCornerLatExpected).arg(neCornerLonExpected).arg(tileInfo.neLat).arg(tileInfo.neLon).arg(tileInfo.swLat).arg(tileInfo.swLon).arg(tileInfo.gridSizeLat).arg(tileInfo.gridSizeLon);
        QByteArray emptyArray;
        return emptyArray;
    }

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
    int latIndex = -1;

    // We have to be careful of double value imprecision for lat/lon values.
    // Don't trust _northEast corner values because of this (they are set from airmap query response).
    // Calculate everything from swCorner values only

    if (isValid() && _southWest.isValid() && _northEast.isValid()) {
        double clampedLatitude = _swCornerClampedLatitude(latitude);
        latIndex = qRound((clampedLatitude - _southWest.latitude()) / tileValueSpacingDegrees);
        qCDebug(TerrainTileLog) << "_latToDataIndex: latIndex:latitude:clampedLatitude:_southWest" << latIndex << latitude << clampedLatitude << _southWest;
    } else {
        qCWarning(TerrainTileLog) << "_latToDataIndex: Internal error - isValid:_southWest.isValid:_northEast.isValid" << isValid() << _southWest.isValid() << _northEast.isValid();
    }

    return latIndex;
}

int TerrainTile::_lonToDataIndex(double longitude) const
{
    int lonIndex = -1;

    // We have to be careful of double value imprecision for lat/lon values.
    // Don't trust _northEast corner values because of this (they are set from airmap query response).
    // Calculate everything from swCorner values only

    if (isValid() && _southWest.isValid() && _northEast.isValid()) {
        double clampledLongitude = _swCornerClampedLongitude(longitude);
        lonIndex = qRound((clampledLongitude - _southWest.longitude()) / tileValueSpacingDegrees);
        qCDebug(TerrainTileLog) << "_lonToDataIndex: lonIndex:longitude:clampledLongitude:_southWest" << lonIndex << longitude << clampledLongitude << _southWest;
    } else {
        qCWarning(TerrainTileLog) << "_lonToDataIndex: Internal error - isValid:_southWest.isValid:_northEast.isValid" << isValid() << _southWest.isValid() << _northEast.isValid();
    }

    return lonIndex;
}
