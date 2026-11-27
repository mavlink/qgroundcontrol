#include "CopernicusTileFormat.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

#include "JsonParsing.h"
#include "QGCCompression.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(CopernicusTileFormatLog, "MapTiles.CopernicusTileFormat")

namespace CopernicusTileFormat {

namespace {

constexpr const char* kStatusKey = "status";
constexpr const char* kDataKey = "data";
constexpr const char* kBoundsKey = "bounds";
constexpr const char* kSouthWestKey = "sw";
constexpr const char* kNorthEastKey = "ne";
constexpr const char* kStatsKey = "stats";
constexpr const char* kMaxElevationKey = "max";
constexpr const char* kMinElevationKey = "min";
constexpr const char* kAvgElevationKey = "avg";
constexpr const char* kCarpetKey = "carpet";

}  // namespace

QJsonValue responseData(const QByteArray& response)
{
    QJsonParseError parseError;
    const QJsonDocument document = QGCCompression::parseCompressedJson(response, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qCWarning(CopernicusTileFormatLog) << "Terrain tile json doc parse error" << parseError.errorString();
        return QJsonValue();
    }

    if (!document.isObject()) {
        qCWarning(CopernicusTileFormatLog) << "Terrain tile json doc is no object";
        return QJsonValue();
    }

    const QJsonObject rootObject = document.object();

    static const QList<JsonParsing::KeyValidateInfo> rootVersionKeyInfoList = {
        {kStatusKey, QJsonValue::String, true},
        {kDataKey, QJsonValue::Object, true},
    };
    QString errorString;
    if (!JsonParsing::validateKeys(rootObject, rootVersionKeyInfoList, errorString)) {
        qCWarning(CopernicusTileFormatLog) << "Error in reading json: " << errorString;
        return QJsonValue();
    }

    if (rootObject[kStatusKey].toString() != "success") {
        qCWarning(CopernicusTileFormatLog) << "Invalid terrain tile.";
        return QJsonValue();
    }

    const QJsonValue& jsonData = rootObject[kDataKey];
    const QJsonObject& dataObject = jsonData.toObject();

    QList<JsonParsing::KeyValidateInfo> dataVersionKeyInfoList = {
        {kBoundsKey, QJsonValue::Object, true},
        {kStatsKey, QJsonValue::Object, true},
        {kCarpetKey, QJsonValue::Array, true},
    };

    if (!JsonParsing::validateKeys(dataObject, dataVersionKeyInfoList, errorString)) {
        qCWarning(CopernicusTileFormatLog) << "Error in reading json: " << errorString;
        return QJsonValue();
    }

    return jsonData;
}

QByteArray serialize(const QByteArray& response)
{
    const QJsonValue& jsonData = responseData(response);
    if (jsonData.isNull()) {
        return QByteArray();
    }

    const QJsonObject& dataObject = jsonData.toObject();

    const QJsonObject& boundsObject = dataObject[kBoundsKey].toObject();
    static const QList<JsonParsing::KeyValidateInfo> boundsVersionKeyInfoList = {
        {kSouthWestKey, QJsonValue::Array, true},
        {kNorthEastKey, QJsonValue::Array, true},
    };
    QString errorString;
    if (!JsonParsing::validateKeys(boundsObject, boundsVersionKeyInfoList, errorString)) {
        qCWarning(CopernicusTileFormatLog) << "Error in reading json: " << errorString;
        return QByteArray();
    }

    const QJsonArray& swArray = boundsObject[kSouthWestKey].toArray();
    const QJsonArray& neArray = boundsObject[kNorthEastKey].toArray();
    if ((swArray.count() < 2) || (neArray.count() < 2)) {
        qCWarning(CopernicusTileFormatLog) << "Incomplete bounding location";
        return QByteArray();
    }

    const QJsonObject& statsObject = dataObject[kStatsKey].toObject();
    static const QList<JsonParsing::KeyValidateInfo> statsVersionKeyInfoList = {
        {kMinElevationKey, QJsonValue::Double, true},
        {kMaxElevationKey, QJsonValue::Double, true},
        {kAvgElevationKey, QJsonValue::Double, true},
    };
    if (!JsonParsing::validateKeys(statsObject, statsVersionKeyInfoList, errorString)) {
        qCWarning(CopernicusTileFormatLog) << "Error in reading json: " << errorString;
        return QByteArray();
    }

    const QJsonArray& carpetArray = dataObject[kCarpetKey].toArray();

    if (carpetArray.isEmpty()) {
        qCWarning(CopernicusTileFormatLog) << "Empty carpet array in terrain data";
        return QByteArray();
    }

    const QJsonArray& firstRow = carpetArray[0].toArray();
    if (firstRow.isEmpty()) {
        qCWarning(CopernicusTileFormatLog) << "Empty first row in carpet array";
        return QByteArray();
    }

    Header tileInfo;
    tileInfo.swLat = swArray[0].toDouble();
    tileInfo.swLon = swArray[1].toDouble();
    tileInfo.neLat = neArray[0].toDouble();
    tileInfo.neLon = neArray[1].toDouble();
    tileInfo.minElevation = static_cast<int16_t>(statsObject[kMinElevationKey].toInt());
    tileInfo.maxElevation = static_cast<int16_t>(statsObject[kMaxElevationKey].toInt());
    tileInfo.avgElevation = statsObject[kAvgElevationKey].toDouble();
    tileInfo.gridSizeLat = static_cast<int16_t>(carpetArray.count());
    tileInfo.gridSizeLon = static_cast<int16_t>(firstRow.count());

    qCDebug(CopernicusTileFormatLog) << "Serialize: TileInfo: south west:" << tileInfo.swLat << tileInfo.swLon;
    qCDebug(CopernicusTileFormatLog) << "Serialize: TileInfo: north east:" << tileInfo.neLat << tileInfo.neLon;

    constexpr int cTileNumHeaderBytes = static_cast<int>(sizeof(Header));
    const int cTileNumDataBytes = static_cast<int>(sizeof(int16_t)) * tileInfo.gridSizeLat * tileInfo.gridSizeLon;

    QByteArray result(cTileNumHeaderBytes + cTileNumDataBytes, Qt::Uninitialized);
    Header* pTileInfo = reinterpret_cast<Header*>(result.data());
    int16_t* const pTileData =
        reinterpret_cast<int16_t*>(&reinterpret_cast<uint8_t*>(result.data())[cTileNumHeaderBytes]);

    *pTileInfo = tileInfo;

    int valueIndex = 0;
    for (qsizetype i = 0; i < static_cast<qsizetype>(tileInfo.gridSizeLat); i++) {
        const QJsonArray& row = carpetArray[i].toArray();
        if (row.count() < tileInfo.gridSizeLon) {
            qCDebug(CopernicusTileFormatLog)
                << "Expected row array of" << tileInfo.gridSizeLon << ", instead got" << row.count();
            return QByteArray();
        }

        for (qsizetype j = 0; j < static_cast<qsizetype>(tileInfo.gridSizeLon); j++) {
            pTileData[valueIndex++] = static_cast<int16_t>(row[j].toDouble());
        }
    }

    return result;
}

}  // namespace CopernicusTileFormat
