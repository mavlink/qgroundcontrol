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
#include <QVariantList>
#include <QGeoCoordinate>

class JsonHelper
{
public:
    /// Determines is the specified data is a json file
    ///     @param jsonDoc Returned json document if json file
    /// @return true: file is json, false: file is not json
    static bool isJsonFile(const QByteArray& bytes, QJsonDocument& jsonDoc);

    /// Validates the standard parts of a QGC json file:
    ///     jsonFileTypeKey - Required and checked to be equal to expectedFileType
    ///     jsonVersionKey - Required and checked to be below supportedMajorVersion, supportedMinorVersion
    ///     jsonGroundStationKey - Required and checked to be string type
    /// @return false: validation failed
    static bool validateQGCJsonFile(const QJsonObject&  jsonObject,             ///< root json object
                                    const QString&      expectedFileType,       ///< correct file type for file
                                    int                 minSupportedVersion,    ///< minimum supported version
                                    int                 maxSupportedVersion,    ///< maximum supported major version
                                    int                 &version,               ///< returned file version
                                    QString&            errorString);           ///< returned error string if validation fails

    static bool validateRequiredKeys(const QJsonObject& jsonObject, const QStringList& keys, QString& errorString);
    static bool validateKeyTypes(const QJsonObject& jsonObject, const QStringList& keys, const QList<QJsonValue::Type>& types, QString& errorString);

    typedef struct {
        const char*         key;        ///< json key name
        QJsonValue::Type    type;       ///< type of key
        bool                required;   ///< true: key must be present
    } KeyValidateInfo;

    static bool validateKeys(const QJsonObject& jsonObject, const QList<KeyValidateInfo>& keyInfo, QString& errorString);

    /// Loads a QGeoCoordinate
    /// @return false: validation failed
    static bool loadGeoCoordinate(const QJsonValue& jsonValue,          ///< json value to load from
                                  bool              altitudeRequired,   ///< true: altitude must be specified
                                  QGeoCoordinate&   coordinate,         ///< returned QGeoCordinate
                                  QString&          errorString);       ///< returned error string if load failure

    /// Saves a QGeoCoordinate
    static void saveGeoCoordinate(const QGeoCoordinate& coordinate,     ///< QGeoCoordinate to save
                                  bool                  writeAltitude,  ///< true: write altitude to json
                                  QJsonValue&           jsonValue);     ///< json value to save to

    /// Loads a list of QGeoCoordinates from a json array
    /// @return false: validation failed
    static bool loadGeoCoordinateArray(const QJsonValue&    jsonValue,              ///< json value which contains points
                                       bool                 altitudeRequired,       ///< true: altitude field must be specified
                                       QVariantList&        rgVarPoints,            ///< returned points
                                       QString&             errorString);           ///< returned error string if load failure
    static bool loadGeoCoordinateArray(const QJsonValue&        jsonValue,          ///< json value which contains points
                                       bool                     altitudeRequired,   ///< true: altitude field must be specified
                                       QList<QGeoCoordinate>&   rgPoints,           ///< returned points
                                       QString&                 errorString);       ///< returned error string if load failure

    /// Saves a list of QGeoCoordinates to a json array
    static void saveGeoCoordinateArray(const QVariantList&  rgVarPoints,            ///< points to save
                                       bool                 writeAltitude,          ///< true: write altitide value
                                       QJsonValue&          jsonValue);             ///< json value to save to
    static void saveGeoCoordinateArray(const QList<QGeoCoordinate>& rgPoints,       ///< points to save
                                       bool                         writeAltitude,  ///< true: write altitide value
                                       QJsonValue&                  jsonValue);     ///< json value to save to

    static bool parseEnum(const QJsonObject& jsonObject, QStringList& enumStrings, QStringList& enumValues, QString& errorString);


    static const char* jsonVersionKey;
    static const char* jsonGroundStationKey;
    static const char* jsonGroundStationValue;
    static const char* jsonFileTypeKey;

private:
    static QString _jsonValueTypeToString(QJsonValue::Type type);

    static const char*  _enumStringsJsonKey;
    static const char*  _enumValuesJsonKey;
};

#endif
