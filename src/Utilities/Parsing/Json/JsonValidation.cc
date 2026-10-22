#include "JsonValidation.h"

#include <algorithm>
#include <optional>

#include <QtCore/QObject>
#include <QtCore/QUtf8StringView>
#include <QtCore/QVarLengthArray>

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

bool validateMissingKeys(const QStringList& missingKeys, QString& errorString)
{
    if (missingKeys.isEmpty()) {
        return true;
    }
    errorString =
        QObject::tr("The following required keys are missing: %1").arg(missingKeys.join(QStringLiteral(", ")));
    return false;
}

bool valueTypeMatches(QJsonValue::Type actual, QJsonValue::Type expected)
{
    // Undefined accepts any type; Null allows a double or a JSON null representing NaN.
    return expected == QJsonValue::Undefined || actual == expected ||
           (actual == QJsonValue::Double && expected == QJsonValue::Null);
}

bool validateValueType(const QString& key, QJsonValue::Type actual, QJsonValue::Type expected, QString& errorString)
{
    if (valueTypeMatches(actual, expected)) {
        return true;
    }
    errorString = QObject::tr("Incorrect value type - key:type:expected %1:%2:%3")
                      .arg(key, jsonValueTypeToString(actual), jsonValueTypeToString(expected));
    return false;
}

using PresentKeyTypes = QVarLengthArray<std::optional<QJsonValue::Type>, 32>;
using KnownJsonKeys = QVarLengthArray<bool, 32>;

/// Finds a UTF-8 key without allocating in the common ASCII case; firmware parameter metadata
/// validates thousands of objects while loading.
QJsonObject::const_iterator findKey(const QJsonObject& jsonObject, const char* key)
{
    const QUtf8StringView utf8Key(key);
    const bool ascii =
        std::all_of(utf8Key.cbegin(), utf8Key.cend(), [](char c) { return static_cast<uchar>(c) < 0x80; });
    if (ascii) {
        return jsonObject.constFind(QLatin1StringView(utf8Key.data(), utf8Key.size()));
    }
    return jsonObject.constFind(utf8Key.toString());
}

/// Returns the value type of each keyInfo entry present in jsonObject and, when requested,
/// marks the JSON entries that keyInfo describes.
PresentKeyTypes findPresentKeyTypes(const QJsonObject& jsonObject, const QList<JsonParsing::KeyValidateInfo>& keyInfo,
                                    KnownJsonKeys* knownJsonKeys = nullptr)
{
    PresentKeyTypes presentTypes(keyInfo.size(), std::nullopt);
    for (qsizetype i = 0; i < keyInfo.size(); ++i) {
        const auto value = findKey(jsonObject, keyInfo[i].key);
        if (value == jsonObject.constEnd()) {
            continue;
        }
        presentTypes[i] = value->type();
        if (knownJsonKeys) {
            (*knownJsonKeys)[value - jsonObject.constBegin()] = true;
        }
    }
    return presentTypes;
}

bool validatePresentKeys(const QList<JsonParsing::KeyValidateInfo>& keyInfo, const PresentKeyTypes& presentTypes,
                         QString& errorString)
{
    QStringList missingKeys;
    for (qsizetype i = 0; i < keyInfo.size(); ++i) {
        if (keyInfo[i].required && !presentTypes[i]) {
            missingKeys.append(QString::fromUtf8(keyInfo[i].key));
        }
    }
    if (!validateMissingKeys(missingKeys, errorString)) {
        return false;
    }
    for (qsizetype i = 0; i < keyInfo.size(); ++i) {
        const std::optional<QJsonValue::Type>& actual = presentTypes[i];
        if (actual && !valueTypeMatches(*actual, keyInfo[i].type)) {
            return validateValueType(QString::fromUtf8(keyInfo[i].key), *actual, keyInfo[i].type, errorString);
        }
    }
    return true;
}

}  // namespace

namespace JsonParsing {

bool validateRequiredKeys(const QJsonObject& jsonObject, const QStringList& keys, QString& errorString)
{
    QStringList missingKeys;
    for (const QString& key : keys) {
        if (!jsonObject.contains(key)) {
            missingKeys.append(key);
        }
    }
    return validateMissingKeys(missingKeys, errorString);
}

bool validateKeyTypes(const QJsonObject& jsonObject, const QStringList& keys, const QList<QJsonValue::Type>& types,
                      QString& errorString)
{
    if (keys.count() != types.count()) {
        errorString =
            QObject::tr("Mismatched key and type list sizes: keys=%1 types=%2").arg(keys.count()).arg(types.count());
        return false;
    }

    for (qsizetype i = 0; i < types.size(); ++i) {
        const auto value = jsonObject.constFind(keys[i]);
        if (value != jsonObject.constEnd() && !validateValueType(keys[i], value->type(), types[i], errorString)) {
            return false;
        }
    }
    return true;
}

bool validateKeys(const QJsonObject& jsonObject, const QList<KeyValidateInfo>& keyInfo, QString& errorString)
{
    return validatePresentKeys(keyInfo, findPresentKeyTypes(jsonObject, keyInfo), errorString);
}

bool validateKeysStrict(const QJsonObject& jsonObject, const QList<KeyValidateInfo>& keyInfo, QString& errorString)
{
    KnownJsonKeys knownJsonKeys(jsonObject.size(), false);
    if (!validatePresentKeys(keyInfo, findPresentKeyTypes(jsonObject, keyInfo, &knownJsonKeys), errorString)) {
        return false;
    }

    const auto unknown = std::find(knownJsonKeys.cbegin(), knownJsonKeys.cend(), false);
    if (unknown != knownJsonKeys.cend()) {
        const auto value = jsonObject.constBegin() + (unknown - knownJsonKeys.cbegin());
        errorString = QObject::tr("Unknown key: %1").arg(value.key());
        return false;
    }

    return true;
}

}  // namespace JsonParsing
