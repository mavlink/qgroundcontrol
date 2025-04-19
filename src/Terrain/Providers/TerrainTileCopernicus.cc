/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "TerrainTileCopernicus.h"
#include "JsonHelper.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>

QGC_LOGGING_CATEGORY(TerrainTileCopernicusLog, "qgc.terrain.terraintilecopernicus");

TerrainTileCopernicus::TerrainTileCopernicus(const QByteArray &byteArray)
    : TerrainTile(byteArray)
{
    // qCDebug(TerrainTileCopernicusLog) << Q_FUNC_INFO << this;
}

TerrainTileCopernicus::~TerrainTileCopernicus()
{
    // qCDebug(TerrainTileCopernicusLog) << Q_FUNC_INFO << this;
}

QJsonValue TerrainTileCopernicus::getJsonFromData(const QByteArray &input)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(input, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qCWarning(TerrainTileCopernicusLog) << Q_FUNC_INFO << "Terrain tile json doc parse error" << parseError.errorString();
        return QJsonValue();
    }

    if (!document.isObject()) {
        qCWarning(TerrainTileCopernicusLog) << Q_FUNC_INFO << "Terrain tile json doc is no object";
        return QJsonValue();
    }

    const QJsonObject rootObject = document.object();

    static const QList<JsonHelper::KeyValidateInfo> rootVersionKeyInfoList = {
        { _jsonStatusKey, QJsonValue::String, true },
        { _jsonDataKey, QJsonValue::Object, true },
    };
    QString errorString;
    if (!JsonHelper::validateKeys(rootObject, rootVersionKeyInfoList, errorString)) {
        qCWarning(TerrainTileCopernicusLog) << Q_FUNC_INFO << "Error in reading json: " << errorString;
        return QJsonValue();
    }

    if (rootObject[_jsonStatusKey].toString() != "success") {
        qCWarning(TerrainTileCopernicusLog) << Q_FUNC_INFO << "Invalid terrain tile.";
        return QJsonValue();
    }

    const QJsonValue &jsonData = rootObject[_jsonDataKey];
    const QJsonObject &dataObject = jsonData.toObject();

    QList<JsonHelper::KeyValidateInfo> dataVersionKeyInfoList = {
        { _jsonBoundsKey, QJsonValue::Object, true },
        { _jsonStatsKey,  QJsonValue::Object, true },
        { _jsonCarpetKey, QJsonValue::Array, true },
    };

    if (!JsonHelper::validateKeys(dataObject, dataVersionKeyInfoList, errorString)) {
        qCWarning(TerrainTileCopernicusLog) << Q_FUNC_INFO << "Error in reading json: " << errorString;
        return QJsonValue();
    }

    return jsonData;
}

QByteArray TerrainTileCopernicus::serializeFromData(const QByteArray &input)
{
    const QJsonValue &jsonData = getJsonFromData(input);
    if (jsonData.isNull()) {
        return QByteArray();
    }

    const QJsonObject &dataObject = jsonData.toObject();

    const QJsonObject &boundsObject = dataObject[_jsonBoundsKey].toObject();
    static const QList<JsonHelper::KeyValidateInfo> boundsVersionKeyInfoList = {
        { _jsonSouthWestKey, QJsonValue::Array, true },
        { _jsonNorthEastKey, QJsonValue::Array, true },
    };
    QString errorString;
    if (!JsonHelper::validateKeys(boundsObject, boundsVersionKeyInfoList, errorString)) {
        qCWarning(TerrainTileCopernicusLog) << Q_FUNC_INFO << "Error in reading json: " << errorString;
        return QByteArray();
    }

    const QJsonArray &swArray = boundsObject[_jsonSouthWestKey].toArray();
    const QJsonArray &neArray = boundsObject[_jsonNorthEastKey].toArray();
    if ((swArray.count() < 2) || (neArray.count() < 2 )) {
        qCWarning(TerrainTileCopernicusLog) << Q_FUNC_INFO << "Incomplete bounding location";
        return QByteArray();
    }

    const QJsonObject &statsObject = dataObject[_jsonStatsKey].toObject();
    static const QList<JsonHelper::KeyValidateInfo> statsVersionKeyInfoList = {
        { _jsonMinElevationKey, QJsonValue::Double, true },
        { _jsonMaxElevationKey, QJsonValue::Double, true },
        { _jsonAvgElevationKey, QJsonValue::Double, true },
    };
    if (!JsonHelper::validateKeys(statsObject, statsVersionKeyInfoList, errorString)) {
        qCWarning(TerrainTileCopernicusLog) << Q_FUNC_INFO << "Error in reading json: " << errorString;
        return QByteArray();
    }

    const QJsonArray &carpetArray = dataObject[_jsonCarpetKey].toArray();

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

    qCDebug(TerrainTileCopernicusLog) << "Serialize: TileInfo: south west:" << tileInfo.swLat << tileInfo.swLon;
    qCDebug(TerrainTileCopernicusLog) << "Serialize: TileInfo: north east:" << tileInfo.neLat << tileInfo.neLon;

    constexpr int cTileNumHeaderBytes = static_cast<int>(sizeof(TileInfo_t));
    const int cTileNumDataBytes = static_cast<int>(sizeof(int16_t)) * tileInfo.gridSizeLat * tileInfo.gridSizeLon;

    QByteArray result(cTileNumHeaderBytes + cTileNumDataBytes, Qt::Uninitialized);
    TileInfo_t *pTileInfo = reinterpret_cast<TileInfo_t*>(result.data());
    int16_t* const pTileData = reinterpret_cast<int16_t*>(&reinterpret_cast<uint8_t*>(result.data())[cTileNumHeaderBytes]);

    *pTileInfo = tileInfo;

    int valueIndex = 0;
    for (qsizetype i = 0; i < static_cast<qsizetype>(tileInfo.gridSizeLat); i++) {
        const QJsonArray &row = carpetArray[i].toArray();
        if (row.count() < tileInfo.gridSizeLon) {
            qCDebug(TerrainTileCopernicusLog) << "Expected row array of" << tileInfo.gridSizeLon << ", instead got" << row.count();
            return QByteArray();
        }

        for (qsizetype j = 0; j < static_cast<qsizetype>(tileInfo.gridSizeLon); j++) {
            pTileData[valueIndex++] = static_cast<int16_t>(row[j].toDouble());
        }
    }

    return result;
}
