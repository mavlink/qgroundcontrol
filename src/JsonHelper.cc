/*===================================================================
QGroundControl Open Source Ground Control Station

(c) 2009, 2010 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

    QGROUNDCONTROL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QGROUNDCONTROL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

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
        errorString = QStringLiteral("The following required keys are missing: %1").arg(missingKeys);
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
        errorString = QStringLiteral("Json array must contains %1 values").arg(requiredCount);
        return false;
    }

    coordinate = QGeoCoordinate(coordinateArray[0].toDouble(), coordinateArray[1].toDouble());
    if (altitudeRequired) {
        coordinate.setAltitude(coordinateArray[2].toDouble());
    }

    if (!coordinate.isValid()) {
        errorString = QStringLiteral("Coordinate is invalid: %1").arg(coordinate.toString());
        return false;
    }

    return true;
}

bool JsonHelper::validateKeyTypes(QJsonObject& jsonObject, const QStringList& keys, const QList<QJsonValue::Type>& types, QString& errorString)
{
    for (int i=0; i<keys.count(); i++) {
        if (jsonObject.contains(keys[i])) {
            if (jsonObject.value(keys[i]).type() != types[i]) {
                errorString  = QStringLiteral("Incorrect type key:type:expected %1 %2 %3").arg(keys[i]).arg(jsonObject.value(keys[i]).type()).arg(types[i]);
                return false;
            }
        }
    }

    return true;
}

bool JsonHelper::parseEnum(QJsonObject& jsonObject, QStringList& enumStrings, QStringList& enumValues, QString& errorString)
{
    enumStrings = jsonObject.value(_enumStringsJsonKey).toString().split(QChar(','), QString::SkipEmptyParts);
    enumValues = jsonObject.value(_enumValuesJsonKey).toString().split(QChar(','), QString::SkipEmptyParts);

    if (enumStrings.count() != enumValues.count()) {
        errorString = QStringLiteral("enum strings/values count mismatch: %1");
        return false;
    }

    return true;
}
