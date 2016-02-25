/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
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

#ifndef JsonHelper_H
#define JsonHelper_H

#include <QJsonObject>
#include <QGeoCoordinate>

class JsonHelper
{
public:
    static bool validateRequiredKeys(const QJsonObject& jsonObject, const QStringList& keys, QString& errorString);
    static bool validateKeyTypes(const QJsonObject& jsonObject, const QStringList& keys, const QList<QJsonValue::Type>& types, QString& errorString);
    static bool toQGeoCoordinate(const QJsonValue& jsonValue, QGeoCoordinate& coordinate, bool altitudeRequired, QString& errorString);
    static bool parseEnum(QJsonObject& jsonObject, QStringList& enumStrings, QStringList& enumValues, QString& errorString);

    static void writeQGeoCoordinate(QJsonValue& jsonValue, const QGeoCoordinate& coordinate, bool writeAltitude);

    static const char*  _enumStringsJsonKey;
    static const char*  _enumValuesJsonKey;
};

#endif
