/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "JsonHelper.h"

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

bool JsonHelper::loadGeoCoordinate(const QJsonValue&    jsonValue,
                                   bool                 altitudeRequired,
                                   QGeoCoordinate&      coordinate,
                                   QString&             errorString)
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
        if (jsonValue.type() != QJsonValue::Double) {
            errorString = QObject::tr("Coordinate array may only contain double values, found: %1").arg(jsonValue.type());
            return false;
        }
    }

    coordinate = QGeoCoordinate(coordinateArray[0].toDouble(), coordinateArray[1].toDouble());
    if (altitudeRequired) {
        coordinate.setAltitude(coordinateArray[2].toDouble());
    }

    if (!coordinate.isValid()) {
        errorString = QObject::tr("Coordinate is invalid: %1").arg(coordinate.toString());
        return false;
    }

    return true;
}

void JsonHelper::saveGeoCoordinate(const QGeoCoordinate&    coordinate,
                                   bool                     writeAltitude,
                                   QJsonValue&              jsonValue)
{
    QJsonArray coordinateArray;

    coordinateArray << coordinate.latitude() << coordinate.longitude();
    if (writeAltitude) {
        coordinateArray << coordinate.altitude();
    }

    jsonValue = QJsonValue(coordinateArray);
}

bool JsonHelper::validateKeyTypes(const QJsonObject& jsonObject, const QStringList& keys, const QList<QJsonValue::Type>& types, QString& errorString)
{
    for (int i=0; i<types.count(); i++) {
        QString valueKey = keys[i];
        if (jsonObject.contains(valueKey)) {
            const QJsonValue& jsonValue = jsonObject[valueKey];
            if (jsonValue.type() != types[i]) {
                errorString  = QObject::tr("Incorrect value type - key:type:expected %1:%2:%3").arg(valueKey).arg(_jsonValueTypeToString(jsonValue.type())).arg(_jsonValueTypeToString(types[i]));
                return false;
            }
        }
    }

    return true;
}

bool JsonHelper::parseEnum(const QJsonObject& jsonObject, QStringList& enumStrings, QStringList& enumValues, QString& errorString)
{
    enumStrings = jsonObject.value(_enumStringsJsonKey).toString().split(",", QString::SkipEmptyParts);
    enumValues = jsonObject.value(_enumValuesJsonKey).toString().split(",", QString::SkipEmptyParts);

    if (enumStrings.count() != enumValues.count()) {
        errorString = QObject::tr("enum strings/values count mismatch strings:values %1:%2").arg(enumStrings.count()).arg(enumValues.count());
        return false;
    }

    return true;
}

bool JsonHelper::isJsonFile(const QByteArray& bytes, QJsonDocument& jsonDoc)
{
    QJsonParseError error;

    jsonDoc = QJsonDocument::fromJson(bytes, &error);

    if (error.error == QJsonParseError::NoError) {
        return true;
    }

    if (error.error == QJsonParseError::MissingObject && error.offset == 0) {
        return false;
    }

    return true;
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
