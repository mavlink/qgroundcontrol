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

const char* JsonHelper::_enumStringsJsonKey =   "enumStrings";
const char* JsonHelper::_enumValuesJsonKey =    "enumValues";

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
        errorString = QString("The following required keys are missing: %1").arg(missingKeys);
        return false;
    }

    return true;
}

bool JsonHelper::toQGeoCoordinate(const QJsonValue& jsonValue, QGeoCoordinate& coordinate, bool altitudeRequired, QString& errorString)
{
    if (!jsonValue.isArray()) {
        errorString = QStringLiteral("JSon value for coordinate is not array");
        return false;
    }

    QJsonArray coordinateArray = jsonValue.toArray();
    int requiredCount = altitudeRequired ? 3 : 2;
    if (coordinateArray.count() != requiredCount) {
        errorString = QString("Coordinate array must contain %1 values").arg(requiredCount);
        return false;
    }

    foreach(const QJsonValue& jsonValue, coordinateArray) {
        if (jsonValue.type() != QJsonValue::Double) {
            errorString = QString("Coordinate array may only contain double values, found: %1").arg(jsonValue.type());
            return false;
        }
    }

    coordinate = QGeoCoordinate(coordinateArray[0].toDouble(), coordinateArray[1].toDouble());
    if (altitudeRequired) {
        coordinate.setAltitude(coordinateArray[2].toDouble());
    }

    if (!coordinate.isValid()) {
        errorString = QString("Coordinate is invalid: %1").arg(coordinate.toString());
        return false;
    }

    return true;
}

void JsonHelper::writeQGeoCoordinate(QJsonValue& jsonValue, const QGeoCoordinate& coordinate, bool writeAltitude)
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
    for (int i=0; i<keys.count(); i++) {
        if (jsonObject.contains(keys[i])) {
            if (jsonObject.value(keys[i]).type() != types[i]) {
                errorString  = QString("Incorrect type key:type:expected %1 %2 %3").arg(keys[i]).arg(jsonObject.value(keys[i]).type()).arg(types[i]);
                return false;
            }
        }
    }

    return true;
}

bool JsonHelper::parseEnum(QJsonObject& jsonObject, QStringList& enumStrings, QStringList& enumValues, QString& errorString)
{
    enumStrings = jsonObject.value(_enumStringsJsonKey).toString().split(",", QString::SkipEmptyParts);
    enumValues = jsonObject.value(_enumValuesJsonKey).toString().split(",", QString::SkipEmptyParts);

    if (enumStrings.count() != enumValues.count()) {
        errorString = QString("enum strings/values count mismatch: %1");
        return false;
    }

    return true;
}
