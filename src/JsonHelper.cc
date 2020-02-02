/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "JsonHelper.h"
#include "QGCQGeoCoordinate.h"
#include "QmlObjectListModel.h"

#include <QJsonArray>
#include <QJsonParseError>
#include <QObject>
#include <QRegularExpression>
#include <QRegularExpressionMatch>

const char* JsonHelper::_enumStringsJsonKey =       "enumStrings";
const char* JsonHelper::_enumValuesJsonKey =        "enumValues";
const char* JsonHelper::jsonVersionKey =            "version";
const char* JsonHelper::jsonGroundStationKey =      "groundStation";
const char* JsonHelper::jsonGroundStationValue =    "QGroundControl";
const char* JsonHelper::jsonFileTypeKey =           "fileType";

bool JsonHelper::validateRequiredKeys(const QJsonObject& jsonObject, const QStringList& keys, QString& errorString)
{
    QString missingKeys;

    foreach(const QString& key, keys) {
        if (!jsonObject.contains(key)) {
            if (!missingKeys.isEmpty()) {
                missingKeys += QStringLiteral(", ");
            }
            missingKeys += key;
        }
    }

    if (missingKeys.count() != 0) {
        errorString = QObject::tr("The following required keys are missing: %1").arg(missingKeys);
        return false;
    }

    return true;
}

bool JsonHelper::_loadGeoCoordinate(const QJsonValue&   jsonValue,
                                    bool                altitudeRequired,
                                    QGeoCoordinate&     coordinate,
                                    QString&            errorString,
                                    bool                geoJsonFormat)
{
    if (!jsonValue.isArray()) {
        errorString = QObject::tr("value for coordinate is not array");
        return false;
    }

    QJsonArray coordinateArray = jsonValue.toArray();
    int requiredCount = altitudeRequired ? 3 : 2;
    if (coordinateArray.count() != requiredCount) {
        errorString = QObject::tr("Coordinate array must contain %1 values").arg(requiredCount);
        return false;
    }

    foreach(const QJsonValue& jsonValue, coordinateArray) {
        if (jsonValue.type() != QJsonValue::Double && jsonValue.type() != QJsonValue::Null) {
            errorString = QObject::tr("Coordinate array may only contain double values, found: %1").arg(jsonValue.type());
            return false;
        }
    }

    if (geoJsonFormat) {
        coordinate = QGeoCoordinate(coordinateArray[1].toDouble(), coordinateArray[0].toDouble());
    } else {
        coordinate = QGeoCoordinate(possibleNaNJsonValue(coordinateArray[0]), possibleNaNJsonValue(coordinateArray[1]));
    }
    if (altitudeRequired) {
        coordinate.setAltitude(possibleNaNJsonValue(coordinateArray[2]));
    }

    return true;
}

void JsonHelper::_saveGeoCoordinate(const QGeoCoordinate&   coordinate,
                                    bool                    writeAltitude,
                                    QJsonValue&             jsonValue,
                                    bool                    geoJsonFormat)
{
    QJsonArray coordinateArray;

    if (geoJsonFormat) {
        coordinateArray << coordinate.longitude() << coordinate.latitude();
    } else {
        coordinateArray << coordinate.latitude() << coordinate.longitude();
    }
    if (writeAltitude) {
        coordinateArray << coordinate.altitude();
    }

    jsonValue = QJsonValue(coordinateArray);
}

bool JsonHelper::loadGeoCoordinate(const QJsonValue&    jsonValue,
                                   bool                 altitudeRequired,
                                   QGeoCoordinate&      coordinate,
                                   QString&             errorString,
                                   bool                 geoJsonFormat)
{
    return _loadGeoCoordinate(jsonValue, altitudeRequired, coordinate, errorString, geoJsonFormat);
}

void JsonHelper::saveGeoCoordinate(const QGeoCoordinate&    coordinate,
                                   bool                     writeAltitude,
                                   QJsonValue&              jsonValue)
{
    _saveGeoCoordinate(coordinate, writeAltitude, jsonValue, false /* geoJsonFormat */);
}

bool JsonHelper::loadGeoJsonCoordinate(const QJsonValue& jsonValue,
                                       bool              altitudeRequired,
                                       QGeoCoordinate&   coordinate,
                                       QString&          errorString)
{
    return _loadGeoCoordinate(jsonValue, altitudeRequired, coordinate, errorString, true /* geoJsonFormat */);
}

void JsonHelper::saveGeoJsonCoordinate(const QGeoCoordinate& coordinate,
                                       bool                  writeAltitude,
                                       QJsonValue&           jsonValue)
{
    _saveGeoCoordinate(coordinate, writeAltitude, jsonValue, true /* geoJsonFormat */);
}

bool JsonHelper::validateKeyTypes(const QJsonObject& jsonObject, const QStringList& keys, const QList<QJsonValue::Type>& types, QString& errorString)
{
    for (int i=0; i<types.count(); i++) {
        QString valueKey = keys[i];
        if (jsonObject.contains(valueKey)) {
            const QJsonValue& jsonValue = jsonObject[valueKey];
            if (jsonValue.type() == QJsonValue::Null &&  types[i] == QJsonValue::Double) {
                // Null type signals a NaN on a double value
                continue;
            }
            if (jsonValue.type() != types[i]) {
                errorString  = QObject::tr("Incorrect value type - key:type:expected %1:%2:%3").arg(valueKey).arg(_jsonValueTypeToString(jsonValue.type())).arg(_jsonValueTypeToString(types[i]));
                return false;
            }
        }
    }

    return true;
}

bool JsonHelper::_parseEnumWorker(const QJsonObject& jsonObject, QMap<QString, QString>& defineMap, QStringList& enumStrings, QStringList& enumValues, QString& errorString, QString valueName)
{
    if(jsonObject.value(_enumStringsJsonKey).isArray()) {
        // "enumStrings": ["Auto" , "Manual", "Shutter Priority", "Aperture Priority"],
        QJsonArray jArray = jsonObject.value(_enumStringsJsonKey).toArray();
        for(int i = 0; i < jArray.count(); ++i) {
            enumStrings << jArray.at(i).toString();
        }
    } else {
        // "enumStrings": "Auto,Manual,Shutter Priority,Aperture Priority",
        QString value = jsonObject.value(_enumStringsJsonKey).toString();
        enumStrings = defineMap.value(value, value).split(",", QString::SkipEmptyParts);
    }

    if(jsonObject.value(_enumValuesJsonKey).isArray()) {
        // "enumValues": [0, 1, 2, 3, 4, 5],
        QJsonArray jArray = jsonObject.value(_enumValuesJsonKey).toArray();
        // This should probably be a variant list and not a string list.
        for(int i = 0; i < jArray.count(); ++i) {
            if(jArray.at(i).isString())
                enumValues << jArray.at(i).toString();
            else
                enumValues << QString::number(jArray.at(i).toDouble());
        }
    } else {
        // "enumValues": "0,1,2,3,4,5",
        QString value = jsonObject.value(_enumValuesJsonKey).toString();
        enumValues = defineMap.value(value, value).split(",", QString::SkipEmptyParts);
    }

    if (enumStrings.count() != enumValues.count()) {
        errorString = QObject::tr("enum strings/values count mismatch in %3 strings:values %1:%2").arg(enumStrings.count()).arg(enumValues.count()).arg(valueName);
        return false;
    }

    return true;
}

bool JsonHelper::parseEnum(const QJsonObject& jsonObject, QMap<QString, QString>& defineMap, QStringList& enumStrings, QStringList& enumValues, QString& errorString, QString valueName)
{
    return _parseEnumWorker(jsonObject, defineMap, enumStrings, enumValues, errorString, valueName);
}

bool JsonHelper::parseEnum(const QJsonObject& jsonObject, QStringList& enumStrings, QStringList& enumValues, QString& errorString, QString valueName)
{
    QMap<QString, QString> defineMap;
    return _parseEnumWorker(jsonObject, defineMap, enumStrings, enumValues, errorString, valueName);
}

bool JsonHelper::isJsonFile(const QByteArray& bytes, QJsonDocument& jsonDoc, QString& errorString)
{
    QJsonParseError parseError;

    jsonDoc = QJsonDocument::fromJson(bytes, &parseError);

    if (parseError.error == QJsonParseError::NoError) {
        return true;
    } else {
        errorString = parseError.errorString();
        return false;
    }
}

bool JsonHelper::validateQGCJsonFile(const QJsonObject& jsonObject,
                                     const QString&     expectedFileType,
                                     int                minSupportedVersion,
                                     int                maxSupportedVersion,
                                     int&               version,
                                     QString&           errorString)
{
    // Check for required keys
    QStringList requiredKeys = { jsonFileTypeKey, jsonGroundStationKey, jsonVersionKey };
    if (!validateRequiredKeys(jsonObject, requiredKeys, errorString)) {
        return false;
    }

    // Validate base key types
    QList<QJsonValue::Type> typeList = { QJsonValue::String, QJsonValue::String };
    if (!validateKeyTypes(jsonObject, requiredKeys, typeList, errorString)) {
        return false;
    }

    // Make sure file type is correct
    QString fileTypeValue = jsonObject[jsonFileTypeKey].toString();
    if (fileTypeValue != expectedFileType) {
        errorString = QObject::tr("Incorrect file type key expected:%1 actual:%2").arg(expectedFileType).arg(fileTypeValue);
        return false;
    }

    // Check version - support both old style v1 string and new style integer

    QJsonValue versionValue = jsonObject[jsonVersionKey];
    if (versionValue.type() == QJsonValue::String && versionValue.toString() == QStringLiteral("1.0")) {
        version = 1;
    } else {
        if (versionValue.type() != QJsonValue::Double) {
            errorString = QObject::tr("Incorrect type for version value, must be integer");
            return false;
        }
        version = versionValue.toInt();
    }
    if (version < minSupportedVersion) {
        errorString = QObject::tr("File version %1 is no longer supported").arg(version);
        return false;
    }
    if (version > maxSupportedVersion) {
        errorString = QObject::tr("File version %1 is newer than current supported version %2").arg(version).arg(maxSupportedVersion);
        return false;
    }

    return true;
}

void JsonHelper::saveQGCJsonFileHeader(QJsonObject&     jsonObject,
                                       const QString&   fileType,
                                       int              version)
{
    jsonObject[jsonGroundStationKey] = jsonGroundStationValue;
    jsonObject[jsonFileTypeKey] = fileType;
    jsonObject[jsonVersionKey] = version;
}

bool JsonHelper::loadGeoCoordinateArray(const QJsonValue&   jsonValue,
                                        bool                altitudeRequired,
                                        QVariantList&       rgVarPoints,
                                        QString&            errorString)
{
    if (!jsonValue.isArray()) {
        errorString = QObject::tr("value for coordinate array is not array");
        return false;
    }
    QJsonArray rgJsonPoints = jsonValue.toArray();

    rgVarPoints.clear();
    for (int i=0; i<rgJsonPoints.count(); i++) {
        QGeoCoordinate coordinate;

        if (!JsonHelper::loadGeoCoordinate(rgJsonPoints[i], altitudeRequired, coordinate, errorString)) {
            return false;
        }
        rgVarPoints.append(QVariant::fromValue(coordinate));
    }

    return true;
}

bool JsonHelper::loadGeoCoordinateArray(const QJsonValue&       jsonValue,
                                        bool                    altitudeRequired,
                                        QList<QGeoCoordinate>&  rgPoints,
                                        QString&                errorString)
{
    QVariantList rgVarPoints;

    if (!loadGeoCoordinateArray(jsonValue, altitudeRequired, rgVarPoints, errorString)) {
        return false;
    }

    rgPoints.clear();
    for (int i=0; i<rgVarPoints.count(); i++) {
        rgPoints.append(rgVarPoints[i].value<QGeoCoordinate>());
    }

    return true;
}

void JsonHelper::saveGeoCoordinateArray(const QVariantList& rgVarPoints,
                                        bool                writeAltitude,
                                        QJsonValue&         jsonValue)
{
    QJsonArray rgJsonPoints;

    // Add all points to the array
    for (int i=0; i<rgVarPoints.count(); i++) {
        QJsonValue jsonPoint;

        JsonHelper::saveGeoCoordinate(rgVarPoints[i].value<QGeoCoordinate>(), writeAltitude, jsonPoint);
        rgJsonPoints.append(jsonPoint);
    }

    jsonValue = rgJsonPoints;
}

void JsonHelper::saveGeoCoordinateArray(const QList<QGeoCoordinate>&    rgPoints,
                                        bool                            writeAltitude,
                                        QJsonValue&                     jsonValue)
{
    QVariantList rgVarPoints;

    for (int i=0; i<rgPoints.count(); i++) {
        rgVarPoints.append(QVariant::fromValue(rgPoints[i]));
    }
    return saveGeoCoordinateArray(rgVarPoints, writeAltitude, jsonValue);
}

bool JsonHelper::validateKeys(const QJsonObject& jsonObject, const QList<JsonHelper::KeyValidateInfo>& keyInfo, QString& errorString)
{
    QStringList             keyList;
    QList<QJsonValue::Type> typeList;

    for (int i=0; i<keyInfo.count(); i++) {
        if (keyInfo[i].required) {
            keyList.append(keyInfo[i].key);
        }
    }
    if (!validateRequiredKeys(jsonObject, keyList, errorString)) {
        return false;
    }

    keyList.clear();
    for (int i=0; i<keyInfo.count(); i++) {
        keyList.append(keyInfo[i].key);
        typeList.append(keyInfo[i].type);
    }
    return validateKeyTypes(jsonObject, keyList, typeList, errorString);
}

QString JsonHelper::_jsonValueTypeToString(QJsonValue::Type type)
{
    const struct {
        QJsonValue::Type    type;
        const char*         string;
    } rgTypeToString[] = {
    { QJsonValue::Null,         "NULL" },
    { QJsonValue::Bool,         "Bool" },
    { QJsonValue::Double,       "Double" },
    { QJsonValue::String,       "String" },
    { QJsonValue::Array,        "Array" },
    { QJsonValue::Object,       "Object" },
    { QJsonValue::Undefined,    "Undefined" },
};

    for (size_t i=0; i<sizeof(rgTypeToString)/sizeof(rgTypeToString[0]); i++) {
        if (type == rgTypeToString[i].type) {
            return rgTypeToString[i].string;
        }
    }

    return QObject::tr("Unknown type: %1").arg(type);
}

bool JsonHelper::loadPolygon(const QJsonArray& polygonArray, QmlObjectListModel& list, QObject* parent, QString& errorString)
{
    for (int i=0; i<polygonArray.count(); i++) {
        const QJsonValue& pointValue = polygonArray[i];

        QGeoCoordinate pointCoord;
        if (!JsonHelper::loadGeoCoordinate(pointValue, false /* altitudeRequired */, pointCoord, errorString, true)) {
            list.clearAndDeleteContents();
            return false;
        }
        list.append(new QGCQGeoCoordinate(pointCoord, parent));
    }

    return true;
}

void JsonHelper::savePolygon(QmlObjectListModel& list, QJsonArray& polygonArray)
{
    for (int i=0; i<list.count(); i++) {
        QGeoCoordinate vertex = list.value<QGCQGeoCoordinate*>(i)->coordinate();

        QJsonValue jsonValue;
        JsonHelper::saveGeoCoordinate(vertex, false /* writeAltitude */, jsonValue);
        polygonArray.append(jsonValue);
    }
}

double JsonHelper::possibleNaNJsonValue(const  QJsonValue& value)
{
    if (value.type() == QJsonValue::Null) {
        return std::numeric_limits<double>::quiet_NaN();
    } else {
        return value.toDouble();
    }
}
