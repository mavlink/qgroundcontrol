#pragma once

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QLoggingCategory>
#include <QtCore/QVariantList>
#include <QtPositioning/QGeoCoordinate>

class QTranslator;

Q_DECLARE_LOGGING_CATEGORY(JsonHelperLog)

/// Json manipulation helper class.
/// Primarily used for parsing and processing Fact metadata.
namespace JsonHelper
{
    QTranslator *translator();

    /// Saves the standard file header the json object
    void saveQGCJsonFileHeader(QJsonObject &jsonObject,  ///< root json object
                               const QString &fileType,  ///< file type for file
                               int version);             ///< version number for file

    /// Validates the standard parts of an external QGC json file (Plan file, ...):
    ///     jsonFileTypeKey - Required and checked to be equal to expectedFileType
    ///     jsonVersionKey - Required and checked to be below supportedMajorVersion, supportedMinorVersion
    ///     jsonGroundStationKey - Required and checked to be string type
    /// @return false: validation failed, errorString set
    bool validateExternalQGCJsonFile(const QJsonObject &jsonObject,      ///< json object to validate
                                     const QString &expectedFileType,    ///< correct file type for file
                                     int minSupportedVersion,            ///< minimum supported version
                                     int maxSupportedVersion,            ///< maximum supported major version
                                     int &version,                       ///< returned file version
                                     QString &errorString);              ///< returned error string if validation fails

    /// Validates the standard parts of a internal QGC json file (FactMetaData, ...):
    ///     jsonFileTypeKey - Required and checked to be equal to expectedFileType
    ///     jsonVersionKey - Required and checked to be below supportedMajorVersion, supportedMinorVersion
    /// @return false: validation failed, errorString set
    bool validateInternalQGCJsonFile(const QJsonObject &jsonObject,      ///< json object to validate
                                     const QString &expectedFileType,    ///< correct file type for file
                                     int minSupportedVersion,            ///< minimum supported version
                                     int maxSupportedVersion,            ///< maximum supported major version
                                     int &version,                       ///< returned file version
                                     QString &errorString);              ///< returned error string if validation fails

    // Opens, validates and translates an internal QGC json file.
    // @return Json root object for file. Empty QJsonObject if error.
    QJsonObject openInternalQGCJsonFile(const QString &jsonFilename,     ///< Json file to open
                                        const QString &expectedFileType, ///< correct file type for file
                                        int minSupportedVersion,         ///< minimum supported version
                                        int maxSupportedVersion,         ///< maximum supported major version
                                        int &version,                    ///< returned file version
                                        QString &errorString);           ///< returned error string if validation fails

    struct KeyValidateInfo {
        const char *key;        ///< json key name
        QJsonValue::Type type;  ///< required type for key, QJsonValue::Null specifies double with possible NaN
        bool required;          ///< true: key must be present
    };

    bool validateKeys(const QJsonObject& jsonObject, const QList<KeyValidateInfo>& keyInfo, QString& errorString);

    /// Loads a QGeoCoordinate
    ///     Stored as array [ lat, lon, alt ]
    /// @return false: validation failed
    bool loadGeoCoordinate(const QJsonValue &jsonValue, ///< json value to load from
                           bool altitudeRequired,       ///< true: altitude must be specified
                           QGeoCoordinate &coordinate,  ///< returned QGeoCordinate
                           QString &errorString,        ///< returned error string if load failure
                           bool geoJsonFormat = false); ///< if true, use [lon, lat], [lat, lon] otherwise

    /// Saves a QGeoCoordinate
    ///     Stored as array [ lat, lon, alt ]
    void saveGeoCoordinate(const QGeoCoordinate &coordinate,    ///< QGeoCoordinate to save
                           bool writeAltitude,                  ///< true: write altitude to json
                           QJsonValue &jsonValue,               ///< json value to save to
                           bool geoJsonFormat = false);

    /// Loads a list of QGeoCoordinates from a json array
    /// @return false: validation failed
    bool loadGeoCoordinateArray(const QJsonValue &jsonValue,        ///< json value which contains points
                                bool altitudeRequired,              ///< true: altitude field must be specified
                                QVariantList &rgVarPoints,          ///< returned points
                                QString &errorString,               ///< returned error string if load failure
                                bool geoJsonFormat = false);        ///< if true, use [lon, lat], [lat, lon] otherwise
    bool loadGeoCoordinateArray(const QJsonValue &jsonValue,        ///< json value which contains points
                                bool altitudeRequired,              ///< true: altitude field must be specified
                                QList<QGeoCoordinate> &rgPoints,    ///< returned points
                                QString &errorString,               ///< returned error string if load failure
                                bool geoJsonFormat = false);        ///< if true, use [lon, lat], [lat, lon] otherwise

    /// Saves a list of QGeoCoordinates to a json array
    void saveGeoCoordinateArray(const QVariantList &rgVarPoints,        ///< points to save
                                bool writeAltitude,                     ///< true: write altitide value
                                QJsonValue &jsonValue,                  ///< json value to save to
                                bool geoJsonFormat = false);            ///< if true, use [lon, lat], [lat, lon] otherwise
    void saveGeoCoordinateArray(const QList<QGeoCoordinate> &rgPoints,  ///< points to save
                                bool writeAltitude,                     ///< true: write altitide value
                                QJsonValue &jsonValue,                  ///< json value to save to
                                bool geoJsonFormat = false);            ///< if true, use [lon, lat], [lat, lon] otherwise

    constexpr const char *jsonVersionKey = "version";
    constexpr const char *jsonFileTypeKey = "fileType";
};
