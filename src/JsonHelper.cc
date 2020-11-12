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
#include "MissionCommandList.h"
#include "FactMetaData.h"
#include "QGCApplication.h"

#include <QJsonArray>
#include <QJsonParseError>
#include <QObject>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QFile>
#include <QTranslator>

const char* JsonHelper::jsonVersionKey                      = "version";
const char* JsonHelper::jsonGroundStationKey                = "groundStation";
const char* JsonHelper::jsonGroundStationValue              = "QGroundControl";
const char* JsonHelper::jsonFileTypeKey                     = "fileType";
const char* JsonHelper::_translateKeysKey                   = "translateKeys";
const char* JsonHelper::_arrayIDKeysKey                     = "_arrayIDKeys";

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

bool JsonHelper::isJsonFile(const QByteArray& bytes, QJsonDocument& jsonDoc, QString& errorString)
{
    QJsonParseError parseError;

    jsonDoc = QJsonDocument::fromJson(bytes, &parseError);

    if (parseError.error == QJsonParseError::NoError) {
        return true;
    } else {
        int startPos = qMax(0, parseError.offset - 100);
        int length = qMin(bytes.count() - startPos, 200);
        qDebug() << QStringLiteral("Json read error '%1'").arg(bytes.mid(startPos, length).constData());
        errorString = parseError.errorString();
        return false;
    }
}

bool JsonHelper::isJsonFile(const QString& fileName, QJsonDocument& jsonDoc, QString& errorString)
{
    QFile jsonFile(fileName);
    if (!jsonFile.open(QFile::ReadOnly)) {
        errorString = tr("File open failed: file:error %1 %2").arg(jsonFile.fileName()).arg(jsonFile.errorString());
        return false;
    }
    QByteArray jsonBytes = jsonFile.readAll();
    jsonFile.close();

    return isJsonFile(jsonBytes, jsonDoc, errorString);
}

bool JsonHelper::validateInternalQGCJsonFile(const QJsonObject& jsonObject,
                                             const QString&     expectedFileType,
                                             int                minSupportedVersion,
                                             int                maxSupportedVersion,
                                             int&               version,
                                             QString&           errorString)
{
    // Validate required keys
    QList<JsonHelper::KeyValidateInfo> requiredKeys = {
        { jsonFileTypeKey,       QJsonValue::String, true },
        { jsonVersionKey,        QJsonValue::Double, true },
    };
    if (!JsonHelper::validateKeys(jsonObject, requiredKeys, errorString)) {
        return false;
    }

    // Make sure file type is correct
    QString fileTypeValue = jsonObject[jsonFileTypeKey].toString();
    if (fileTypeValue != expectedFileType) {
        errorString = QObject::tr("Incorrect file type key expected:%1 actual:%2").arg(expectedFileType).arg(fileTypeValue);
        return false;
    }

    // Version check
    version = jsonObject[jsonVersionKey].toInt();
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

bool JsonHelper::validateExternalQGCJsonFile(const QJsonObject& jsonObject,
                                             const QString&     expectedFileType,
                                             int                minSupportedVersion,
                                             int                maxSupportedVersion,
                                             int&               version,
                                             QString&           errorString)
{
    // Validate required keys
    QList<JsonHelper::KeyValidateInfo> requiredKeys = {
        { jsonGroundStationKey, QJsonValue::String, true },
    };
    if (!JsonHelper::validateKeys(jsonObject, requiredKeys, errorString)) {
        return false;
    }

    return validateInternalQGCJsonFile(jsonObject, expectedFileType, minSupportedVersion, maxSupportedVersion, version, errorString);
}

QStringList JsonHelper::_addDefaultLocKeys(QJsonObject& jsonObject)
{
    QString translateKeys;
    QString fileType = jsonObject[jsonFileTypeKey].toString();
    if (!fileType.isEmpty()) {
        if (fileType == MissionCommandList::qgcFileType) {
            if (jsonObject.contains(_translateKeysKey)) {
                translateKeys = jsonObject[_translateKeysKey].toString();
            } else {
                translateKeys = "label,enumStrings,friendlyName,description,category";
                jsonObject[_translateKeysKey] = translateKeys;
            }
            if (!jsonObject.contains(_arrayIDKeysKey)) {
                jsonObject[_arrayIDKeysKey] = "rawName,comment";
            }
        } else if (fileType == FactMetaData::qgcFileType) {
            if (jsonObject.contains(_translateKeysKey)) {
                translateKeys = jsonObject[_translateKeysKey].toString();
            } else {
                translateKeys = "shortDescription,longDescription,enumStrings";
                jsonObject[_translateKeysKey] = "shortDescription,longDescription,enumStrings";
            }
            if (!jsonObject.contains(_arrayIDKeysKey)) {
                jsonObject[_arrayIDKeysKey] = "name";
            }
        }
    }
    return translateKeys.split(",");
}

QJsonObject JsonHelper::_translateObject(QJsonObject& jsonObject, const QString& translateContext, const QStringList& translateKeys)
{
    for (const QString& key: jsonObject.keys()) {
        if (jsonObject[key].isString()) {
            QString locString = jsonObject[key].toString();
            if (translateKeys.contains(key)) {
                QString disambiguation;
                QString disambiguationPrefix("#loc.disambiguation#");

                if (locString.startsWith(disambiguationPrefix)) {
                    locString = locString.right(locString.length() - disambiguationPrefix.length());
                    int commentEndIndex = locString.indexOf("#");
                    if (commentEndIndex != -1) {
                        disambiguation = locString.left(commentEndIndex);
                        locString = locString.right(locString.length() - disambiguation.length() - 1);
                    }
                }

                QString xlatString = qgcApp()->qgcJSONTranslator().translate(translateContext.toUtf8().constData(), locString.toUtf8().constData(), disambiguation.toUtf8().constData());
                if (!xlatString.isNull()) {
                    jsonObject[key] = xlatString;
                }
            }
        } else if (jsonObject[key].isArray()) {
            QJsonArray childJsonArray = jsonObject[key].toArray();
            jsonObject[key] = _translateArray(childJsonArray, translateContext, translateKeys);
        } else if (jsonObject[key].isObject()) {
            QJsonObject childJsonObject = jsonObject[key].toObject();
            jsonObject[key] = _translateObject(childJsonObject, translateContext, translateKeys);
        }
    }

    return jsonObject;
}

QJsonArray JsonHelper::_translateArray(QJsonArray& jsonArray, const QString& translateContext, const QStringList& translateKeys)
{
    for (int i=0; i<jsonArray.count(); i++) {
        QJsonObject childJsonObject = jsonArray[i].toObject();
        jsonArray[i] = _translateObject(childJsonObject, translateContext, translateKeys);
    }

    return jsonArray;
}

QJsonObject JsonHelper::_translateRoot(QJsonObject& jsonObject, const QString& translateContext, const QStringList& translateKeys)
{
    return _translateObject(jsonObject, translateContext, translateKeys);
}

QJsonObject JsonHelper::openInternalQGCJsonFile(const QString&  jsonFilename,
                                                const QString&  expectedFileType,
                                                int             minSupportedVersion,
                                                int             maxSupportedVersion,
                                                int             &version,
                                                QString&        errorString)
{
    QFile jsonFile(jsonFilename);
    if (!jsonFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        errorString = tr("Unable to open file: '%1', error: %2").arg(jsonFilename).arg(jsonFile.errorString());
        return QJsonObject();
    }

    QByteArray bytes = jsonFile.readAll();
    jsonFile.close();
    QJsonParseError jsonParseError;
    QJsonDocument doc = QJsonDocument::fromJson(bytes, &jsonParseError);
    if (jsonParseError.error != QJsonParseError::NoError) {
        errorString = tr("Unable to parse json file: %1 error: %2 offset: %3").arg(jsonFilename).arg(jsonParseError.errorString()).arg(jsonParseError.offset);
        return QJsonObject();
    }

    if (!doc.isObject()) {
        errorString = tr("Root of json file is not object: %1").arg(jsonFilename);
        return QJsonObject();
    }

    QJsonObject jsonObject = doc.object();
    bool success = validateInternalQGCJsonFile(jsonObject, expectedFileType, minSupportedVersion, maxSupportedVersion, version, errorString);
    if (!success) {
        errorString = tr("Json file: '%1'. %2").arg(jsonFilename).arg(errorString);
        return QJsonObject();
    }

    QStringList translateKeys = _addDefaultLocKeys(jsonObject);
    QString context = QFileInfo(jsonFile).fileName();
    return _translateRoot(jsonObject, context, translateKeys);
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
