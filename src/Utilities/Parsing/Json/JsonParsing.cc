#include "JsonParsing.h"

#include <limits>

#include <QtCore/QApplicationStatic>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonParseError>
#include <QtCore/QObject>
#include <QtCore/QSet>
#include <QtCore/QTranslator>

#include "QGCCompression.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(JsonParsingLog, "Utilities.Parsing.Json")

namespace {

QString jsonValueTypeToString(QJsonValue::Type type)
{
    struct TypeToString
    {
        QJsonValue::Type type;
        const char* string;
    };

    static constexpr const TypeToString typeToStringMap[] = {
        {QJsonValue::Null, "NULL"},           {QJsonValue::Bool, "Bool"},   {QJsonValue::Double, "Double"},
        {QJsonValue::String, "String"},       {QJsonValue::Array, "Array"}, {QJsonValue::Object, "Object"},
        {QJsonValue::Undefined, "Undefined"},
    };

    for (const TypeToString& entry : typeToStringMap) {
        if (type == entry.type) {
            return entry.string;
        }
    }

    return QObject::tr("Unknown type: %1").arg(type);
}

}  // namespace

namespace JsonParsing {

bool validateRequiredKeys(const QJsonObject& jsonObject, const QStringList& keys, QString& errorString)
{
    QString missingKeys;

    for (const QString& key : keys) {
        if (!jsonObject.contains(key)) {
            if (!missingKeys.isEmpty()) {
                missingKeys += QStringLiteral(", ");
            }
            missingKeys += key;
        }
    }

    if (!missingKeys.isEmpty()) {
        errorString = QObject::tr("The following required keys are missing: %1").arg(missingKeys);
        return false;
    }

    return true;
}

bool validateKeyTypes(const QJsonObject& jsonObject, const QStringList& keys, const QList<QJsonValue::Type>& types,
                      QString& errorString)
{
    if (keys.count() != types.count()) {
        errorString = QObject::tr("Mismatched key and type list sizes: keys=%1 types=%2")
                          .arg(keys.count()).arg(types.count());
        return false;
    }

    for (qsizetype i = 0; i < types.count(); i++) {
        const QString& valueKey = keys[i];
        if (jsonObject.contains(valueKey)) {
            const QJsonValue& jsonValue = jsonObject[valueKey];
            if ((jsonValue.type() == QJsonValue::Double) && (types[i] == QJsonValue::Null)) {
                // Null type signals a possible NaN on a double value.
                continue;
            }
            if (jsonValue.type() != types[i]) {
                errorString = QObject::tr("Incorrect value type - key:type:expected %1:%2:%3")
                                  .arg(valueKey, jsonValueTypeToString(jsonValue.type()),
                                       jsonValueTypeToString(types[i]));
                return false;
            }
        }
    }

    return true;
}

bool isJsonFile(const QByteArray& bytes, QJsonDocument& jsonDoc, QString& errorString)
{
    QJsonParseError parseError;
    jsonDoc = QGCCompression::parseCompressedJson(bytes, &parseError);

    if (parseError.error == QJsonParseError::NoError) {
        return true;
    }

    const int startPos = qMax(0, parseError.offset - 100);
    const int length = qMin(bytes.length() - startPos, 200);
    qCDebug(JsonParsingLog) << "Json read error" << bytes.mid(startPos, length).constData();
    errorString = parseError.errorString();

    return false;
}

bool isJsonFile(const QString& fileName, QJsonDocument& jsonDoc, QString& errorString)
{
    const QByteArray jsonBytes = QGCCompression::readFile(fileName, &errorString);
    if (jsonBytes.isEmpty() && !errorString.isEmpty()) {
        return false;
    }

    return isJsonFile(jsonBytes, jsonDoc, errorString);
}

double possibleNaNJsonValue(const QJsonValue& value)
{
    if (value.type() == QJsonValue::Null) {
        return std::numeric_limits<double>::quiet_NaN();
    }

    return value.toDouble();
}

bool validateKeys(const QJsonObject& jsonObject, const QList<KeyValidateInfo>& keyInfo, QString& errorString)
{
    QStringList keyList;
    QList<QJsonValue::Type> typeList;

    for (const KeyValidateInfo& info : keyInfo) {
        if (info.required) {
            keyList.append(info.key);
        }
    }
    if (!validateRequiredKeys(jsonObject, keyList, errorString)) {
        return false;
    }

    keyList.clear();
    for (const KeyValidateInfo& info : keyInfo) {
        keyList.append(info.key);
        typeList.append(info.type);
    }

    return validateKeyTypes(jsonObject, keyList, typeList, errorString);
}

bool validateKeysStrict(const QJsonObject& jsonObject, const QList<KeyValidateInfo>& keyInfo, QString& errorString)
{
    if (!validateKeys(jsonObject, keyInfo, errorString)) {
        return false;
    }

    QSet<QString> expectedKeys;
    expectedKeys.reserve(keyInfo.size());
    for (const KeyValidateInfo &info : keyInfo) {
        expectedKeys.insert(QLatin1String(info.key));
    }

    for (const QString &key : jsonObject.keys()) {
        if (!expectedKeys.contains(key)) {
            errorString = QStringLiteral("Unknown key: %1").arg(key);
            return false;
        }
    }

    return true;
}

}  // namespace JsonParsing

// ---------------------------------------------------------------------------
// QGC json file header / translation utilities
// ---------------------------------------------------------------------------

namespace {

constexpr const char *_translateKeysKey = "translateKeys";
constexpr const char *_arrayIDKeysKey = "_arrayIDKeys";
constexpr const char *_jsonGroundStationKey = "groundStation";
constexpr const char *_jsonGroundStationValue = "QGroundControl";

Q_APPLICATION_STATIC(QTranslator, s_jsonTranslator);

QJsonObject translateObject(QJsonObject &jsonObject, const QString &translateContext, const QStringList &translateKeys);

QJsonArray translateArray(QJsonArray &jsonArray, const QString &translateContext, const QStringList &translateKeys)
{
    for (qsizetype i = 0; i < jsonArray.count(); i++) {
        QJsonObject childJsonObject = jsonArray[i].toObject();
        jsonArray[i] = translateObject(childJsonObject, translateContext, translateKeys);
    }
    return jsonArray;
}

QJsonObject translateObject(QJsonObject &jsonObject, const QString &translateContext, const QStringList &translateKeys)
{
    for (const QString &key : jsonObject.keys()) {
        if (jsonObject[key].isString()) {
            QString locString = jsonObject[key].toString();
            if (!translateKeys.contains(key)) {
                continue;
            }

            QString disambiguation;
            const QString disambiguationPrefix("#loc.disambiguation#");
            if (locString.startsWith(disambiguationPrefix)) {
                locString = locString.right(locString.length() - disambiguationPrefix.length());
                const int commentEndIndex = locString.indexOf("#");
                if (commentEndIndex != -1) {
                    disambiguation = locString.left(commentEndIndex);
                    locString = locString.right(locString.length() - disambiguation.length() - 1);
                }
            }

            const QString xlatString = JsonParsing::translator()->translate(
                translateContext.toUtf8().constData(),
                locString.toUtf8().constData(),
                disambiguation.toUtf8().constData());
            if (!xlatString.isNull()) {
                jsonObject[key] = xlatString;
            }
        } else if (jsonObject[key].isArray()) {
            QJsonArray childJsonArray = jsonObject[key].toArray();
            jsonObject[key] = translateArray(childJsonArray, translateContext, translateKeys);
        } else if (jsonObject[key].isObject()) {
            QJsonObject childJsonObject = jsonObject[key].toObject();
            jsonObject[key] = translateObject(childJsonObject, translateContext, translateKeys);
        }
    }
    return jsonObject;
}

/// Resolves the translate-keys list for `jsonObject`. If the JSON omits `translateKeys`
/// and the caller supplied non-empty defaults, those defaults are written in.
/// Same logic for `_arrayIDKeys` and `defaultArrayIDKeys`.
QStringList resolveTranslateKeys(QJsonObject &jsonObject,
                                 const QStringList &defaultTranslateKeys,
                                 const QStringList &defaultArrayIDKeys)
{
    QString translateKeys;
    if (jsonObject.contains(_translateKeysKey)) {
        translateKeys = jsonObject[_translateKeysKey].toString();
    } else if (!defaultTranslateKeys.isEmpty()) {
        translateKeys = defaultTranslateKeys.join(",");
        jsonObject[_translateKeysKey] = translateKeys;
    }

    if (!jsonObject.contains(_arrayIDKeysKey) && !defaultArrayIDKeys.isEmpty()) {
        jsonObject[_arrayIDKeysKey] = defaultArrayIDKeys.join(",");
    }

    if (translateKeys.isEmpty()) {
        return {};
    }
    return translateKeys.split(",");
}

}  // namespace

namespace JsonParsing {

QTranslator *translator()
{
    return s_jsonTranslator();
}

void saveQGCJsonFileHeader(QJsonObject &jsonObject, const QString &fileType, int version)
{
    jsonObject[_jsonGroundStationKey] = _jsonGroundStationValue;
    jsonObject[jsonFileTypeKey] = fileType;
    jsonObject[jsonVersionKey] = version;
}

bool validateInternalQGCJsonFile(const QJsonObject &jsonObject, const QString &expectedFileType,
                                 int minSupportedVersion, int maxSupportedVersion, int &version,
                                 QString &errorString)
{
    static const QList<KeyValidateInfo> requiredKeys = {
        {jsonFileTypeKey, QJsonValue::String, true},
        {jsonVersionKey, QJsonValue::Double, true},
    };

    if (!validateKeys(jsonObject, requiredKeys, errorString)) {
        return false;
    }

    const QString fileTypeValue = jsonObject[jsonFileTypeKey].toString();
    if (fileTypeValue != expectedFileType) {
        errorString = QObject::tr("Incorrect file type key expected:%1 actual:%2").arg(expectedFileType, fileTypeValue);
        return false;
    }

    version = jsonObject[jsonVersionKey].toInt();
    if (version < minSupportedVersion) {
        errorString = QObject::tr("File version %1 is no longer supported").arg(version);
        return false;
    }

    if (version > maxSupportedVersion) {
        errorString = QObject::tr("File version %1 is newer than current supported version %2")
                          .arg(version)
                          .arg(maxSupportedVersion);
        return false;
    }

    return true;
}

bool validateExternalQGCJsonFile(const QJsonObject &jsonObject, const QString &expectedFileType,
                                 int minSupportedVersion, int maxSupportedVersion, int &version,
                                 QString &errorString)
{
    static const QList<KeyValidateInfo> requiredKeys = {
        {_jsonGroundStationKey, QJsonValue::String, true},
    };

    if (!validateKeys(jsonObject, requiredKeys, errorString)) {
        return false;
    }

    return validateInternalQGCJsonFile(jsonObject, expectedFileType, minSupportedVersion, maxSupportedVersion, version,
                                       errorString);
}

QJsonObject openInternalQGCJsonFile(const QString &jsonFilename, const QString &expectedFileType,
                                    int minSupportedVersion, int maxSupportedVersion, int &version,
                                    QString &errorString,
                                    const QStringList &defaultTranslateKeys,
                                    const QStringList &defaultArrayIDKeys)
{
    const QByteArray bytes = QGCCompression::readFile(jsonFilename, &errorString);
    if (bytes.isEmpty() && !errorString.isEmpty()) {
        return {};
    }

    QJsonParseError jsonParseError;
    const QJsonDocument doc = QGCCompression::parseCompressedJson(bytes, &jsonParseError);
    if (jsonParseError.error != QJsonParseError::NoError) {
        errorString = QObject::tr("Unable to parse json file: %1 error: %2 offset: %3")
                          .arg(jsonFilename, jsonParseError.errorString())
                          .arg(jsonParseError.offset);
        return {};
    }

    if (!doc.isObject()) {
        errorString = QObject::tr("Root of json file is not object: %1").arg(jsonFilename);
        return {};
    }

    QJsonObject jsonObject = doc.object();
    const bool success = validateInternalQGCJsonFile(jsonObject, expectedFileType, minSupportedVersion,
                                                     maxSupportedVersion, version, errorString);
    if (!success) {
        errorString = QObject::tr("Json file: '%1'. %2").arg(jsonFilename, errorString);
        return {};
    }

    const QStringList translateKeys = resolveTranslateKeys(jsonObject, defaultTranslateKeys, defaultArrayIDKeys);
    const QString context = QFileInfo(jsonFilename).fileName();
    return translateObject(jsonObject, context, translateKeys);
}

}  // namespace JsonParsing
