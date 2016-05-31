/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


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
