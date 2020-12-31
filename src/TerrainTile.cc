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
            delete[] _data[i];
        }
        delete[] _data;
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

double TerrainTile::elevation(const QGeoCoordinate& coordinate) const
{
    if (_isValid && _southWest.isValid() && _northEast.isValid()) {
        qCDebug(TerrainTileLog) << "elevation: " << coordinate << " , in sw " << _southWest << " , ne " << _northEast;

        // The lat/lon values in _northEast and _southWest coordinates can have rounding errors such that the coordinate
        // request may be slightly outside the tile box specified by these values. So we clamp the incoming values to the
        // edges of the tile if needed.

        double clampedLon = qMax(coordinate.longitude(), _southWest.longitude());
        double clampedLat = qMax(coordinate.latitude(), _southWest.latitude());

        // Calc the index of the southernmost and westernmost index data value
        int lonIndex = qFloor((clampedLon - _southWest.longitude()) / tileValueSpacingDegrees);
        int latIndex = qFloor((clampedLat - _southWest.latitude()) / tileValueSpacingDegrees);

        // Calc how far along in between the known values the requested lat/lon is fractionally
        double lonIndexLongitude    = _southWest.longitude() + (static_cast<double>(lonIndex) * tileValueSpacingDegrees);
        double lonFraction          = (clampedLon - lonIndexLongitude) / tileValueSpacingDegrees;
        double latIndexLatitude     = _southWest.latitude() + (static_cast<double>(latIndex) * tileValueSpacingDegrees);
        double latFraction          = (clampedLat - latIndexLatitude) / tileValueSpacingDegrees;

        // Calc the elevation as the average across the four known points
        double known00      = _data[latIndex][lonIndex];
        double known01      = _data[latIndex][lonIndex+1];
        double known10      = _data[latIndex+1][lonIndex];
        double known11      = _data[latIndex+1][lonIndex+1];
        double lonValue1    = known00 + ((known01 - known00) * lonFraction);
        double lonValue2    = known10 + ((known11 - known10) * lonFraction);
        double latValue     = lonValue1 + ((lonValue2 - lonValue1) * latFraction);

        return latValue;
    } else {
        qCWarning(TerrainTileLog) << "elevation: Internal error - invalid tile";
        return qQNaN();
    }
}

QGeoCoordinate TerrainTile::centerCoordinate(void) const
{
    return _southWest.atDistanceAndAzimuth(_southWest.distanceTo(_northEast) / 2.0, _southWest.azimuthTo(_northEast));
}

QByteArray TerrainTile::serializeFromAirMapJson(QByteArray input)
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
