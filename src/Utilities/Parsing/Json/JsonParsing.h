#pragma once

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QStringList>

class QTranslator;

namespace JsonParsing {

constexpr const char *jsonVersionKey = "version";
constexpr const char *jsonFileTypeKey = "fileType";


/// Determines whether a file path contains parseable JSON content.
bool isJsonFile(const QString& fileName, QJsonDocument& jsonDoc, QString& errorString);

/// Determines whether an in-memory byte buffer contains parseable JSON content.
bool isJsonFile(const QByteArray& bytes, QJsonDocument& jsonDoc, QString& errorString);

/// Validates that all listed keys are present in the object.
bool validateRequiredKeys(const QJsonObject& jsonObject, const QStringList& keys, QString& errorString);

/// Validates value types for listed keys that are present in the object.
/// `QJsonValue::Null` as expected type means "double value with possible NaN".
bool validateKeyTypes(const QJsonObject& jsonObject, const QStringList& keys, const QList<QJsonValue::Type>& types,
                      QString& errorString);

struct KeyValidateInfo {
    const char *key;        ///< json key name
    QJsonValue::Type type;  ///< required type for key, QJsonValue::Null specifies double with possible NaN
    bool required;          ///< true: key must be present
};

/// Validates that all required keys are present and that listed keys have the expected type.
bool validateKeys(const QJsonObject& jsonObject, const QList<KeyValidateInfo>& keyInfo, QString& errorString);

/// Validates keys like `validateKeys` but also rejects any keys not listed in `keyInfo`.
bool validateKeysStrict(const QJsonObject& jsonObject, const QList<KeyValidateInfo>& keyInfo, QString& errorString);

/// Returns NaN if the value is null, or the value converted to double otherwise.
double possibleNaNJsonValue(const QJsonValue& value);

/// Translator used by `openInternalQGCJsonFile` for localized strings.
QTranslator *translator();

/// Saves the standard QGC file header (groundStation, fileType, version) into the json object.
void saveQGCJsonFileHeader(QJsonObject &jsonObject, const QString &fileType, int version);

/// Validates the standard parts of an external QGC json file (Plan file, ...).
/// Checks groundStation, fileType, and version keys.
bool validateExternalQGCJsonFile(const QJsonObject &jsonObject,
                                 const QString &expectedFileType,
                                 int minSupportedVersion,
                                 int maxSupportedVersion,
                                 int &version,
                                 QString &errorString);

/// Validates the standard parts of an internal QGC json file (FactMetaData, ...).
/// Checks fileType and version keys.
bool validateInternalQGCJsonFile(const QJsonObject &jsonObject,
                                 const QString &expectedFileType,
                                 int minSupportedVersion,
                                 int maxSupportedVersion,
                                 int &version,
                                 QString &errorString);

/// Opens, validates, and translates an internal QGC json file.
/// @param defaultTranslateKeys  Keys to translate when `translateKeys` is absent from the JSON.
/// @param defaultArrayIDKeys    Array ID keys to use when `_arrayIDKeys` is absent from the JSON.
QJsonObject openInternalQGCJsonFile(const QString &jsonFilename,
                                    const QString &expectedFileType,
                                    int minSupportedVersion,
                                    int maxSupportedVersion,
                                    int &version,
                                    QString &errorString,
                                    const QStringList &defaultTranslateKeys = {},
                                    const QStringList &defaultArrayIDKeys = {});

}  // namespace JsonParsing
