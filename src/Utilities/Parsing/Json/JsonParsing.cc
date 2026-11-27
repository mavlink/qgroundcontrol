#include "JsonParsing.h"

#include <limits>

#include <QtCore/QApplicationStatic>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonParseError>
#include <QtCore/QObject>
#include <QtCore/QTranslator>

#include "Compression.h"
#include "QGCFormat.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(JsonParsingLog, "Utilities.Parsing.Json")

namespace JsonParsing {

/// Largest size of a JSON file of QGC's own, a resource or a file such as the MAVLink actions, which are at most a
/// few hundred KiB
static constexpr qint64 kMaxQGCJsonFileBytes = 16LL * 1024 * 1024;

/// Bytes read at once from a JSON file, so that a read never takes much more memory than the file
static constexpr qint64 kReadChunkBytes = 64LL * 1024;

/// Whether data starts like a gzip or an xz stream, the compressions that QGC is sent
static bool looksCompressed(const QByteArray& data)
{
    static constexpr char gzipMagic[] = {'\x1f', '\x8b'};
    static constexpr char xzMagic[] = {'\xfd', '7', 'z', 'X', 'Z', '\0'};
    return data.startsWith(QByteArrayView(gzipMagic, sizeof(gzipMagic))) ||
           data.startsWith(QByteArrayView(xzMagic, sizeof(xzMagic)));
}

bool parseJson(const QByteArray& json, QJsonDocument& jsonDoc, QString& errorString)
{
    QJsonParseError parseError;
    jsonDoc = QJsonDocument::fromJson(json, &parseError);

    if (parseError.error == QJsonParseError::NoError) {
        // The caller may pass the error string of an earlier call
        errorString.clear();
        return true;
    }

    if (looksCompressed(json)) {
        errorString = QObject::tr("The content is compressed. Only uncompressed JSON can be read");
        return false;
    }

    const int startPos = qMax(0, parseError.offset - 100);
    qCDebug(JsonParsingLog) << "Json read error" << json.mid(startPos, 200).constData();
    errorString = QStringLiteral("%1 (offset %2)").arg(parseError.errorString()).arg(parseError.offset);

    return false;
}

bool readJsonText(const QString& fileName, QByteArray& text, QString& errorString, qint64 maxBytes)
{
    text.clear();

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        errorString = QObject::tr("Failed to read %1: %2").arg(fileName, file.errorString());
        return false;
    }

    if (!file.isSequential()) {
        if (file.size() > maxBytes) {
            errorString =
                QObject::tr("The file is larger than %1").arg(QGC::bigSizeToString(static_cast<quint64>(maxBytes)));
            return false;
        }
        text.reserve(file.size());
    }

    // One byte beyond the limit is enough to refuse a file that grows while it is read, or has no size
    while (text.size() <= maxBytes) {
        const QByteArray chunk = file.read(qMin(kReadChunkBytes, maxBytes + 1 - text.size()));
        if (chunk.isEmpty()) {
            break;
        }
        text.append(chunk);
    }

    if (file.error() != QFileDevice::NoError) {
        text.clear();
        errorString = QObject::tr("Failed to read %1: %2").arg(fileName, file.errorString());
        return false;
    }
    if (text.size() > maxBytes) {
        text.clear();
        errorString =
            QObject::tr("The file is larger than %1").arg(QGC::bigSizeToString(static_cast<quint64>(maxBytes)));
        return false;
    }

    return true;
}

bool readJsonFile(const QString& fileName, QJsonDocument& jsonDoc, QString& errorString, qint64 maxBytes)
{
    QByteArray json;
    if (!readJsonText(fileName, json, errorString, maxBytes)) {
        jsonDoc = {};
        return false;
    }

    return parseJson(json, jsonDoc, errorString);
}

bool isJsonFile(const QString& fileName, QJsonDocument& jsonDoc, QString& errorString, qint64 maxBytes)
{
    QByteArray json;
    if (const Compression::Outcome read = Compression::decompressFileToMemory(fileName, json, maxBytes); !read) {
        jsonDoc = {};
        errorString = read.description();
        return false;
    }

    return parseJson(json, jsonDoc, errorString);
}

double possibleNaNJsonValue(const QJsonValue& value)
{
    if (value.type() == QJsonValue::Null) {
        return std::numeric_limits<double>::quiet_NaN();
    }

    return value.toDouble();
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
    QJsonDocument doc;
    if (!readJsonFile(jsonFilename, doc, errorString, kMaxQGCJsonFileBytes)) {
        errorString = QObject::tr("Unable to parse json file: %1 error: %2").arg(jsonFilename, errorString);
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
