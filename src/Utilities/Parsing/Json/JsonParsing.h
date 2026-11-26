#pragma once

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QStringList>

#include "JsonValidation.h"

class QTranslator;

namespace JsonParsing {

constexpr const char *jsonVersionKey = "version";
constexpr const char *jsonFileTypeKey = "fileType";

/// Reads and parses a JSON file as it is. Compressed content is refused, not decompressed: use this for every file
/// that is not known to arrive compressed.
/// @param errorString Why it could not; empty when it could
/// @param maxBytes Largest size of the file. A larger file is refused before it is read
[[nodiscard]] bool readJsonFile(const QString& fileName, QJsonDocument& jsonDoc, QString& errorString, qint64 maxBytes);

/// Reads the text of a JSON file as it is, without parsing it. Compressed content is not decompressed.
/// @param text Receives the content; empty after a failure
/// @param errorString Why it could not
/// @param maxBytes Largest size of the file. A larger file is refused before it is read
[[nodiscard]] bool readJsonText(const QString& fileName, QByteArray& text, QString& errorString, qint64 maxBytes);

/// Parses JSON text. Compressed content is refused, not decompressed.
/// @param errorString Why it could not; empty when it could
[[nodiscard]] bool parseJson(const QByteArray& json, QJsonDocument& jsonDoc, QString& errorString);

/// Reads and parses a JSON file that may be compressed (gzip, xz, ...): compressed content is decompressed first.
/// Only for files that arrive compressed: those a vehicle sends or points to.
/// Any other file is read with readJsonFile().
/// @param errorString Why it could not; empty when it could
/// @param maxBytes Largest size of the content, once decompressed. A larger file is not parsed
bool isJsonFile(const QString& fileName, QJsonDocument& jsonDoc, QString& errorString, qint64 maxBytes);

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
