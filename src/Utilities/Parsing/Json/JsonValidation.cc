#include "JsonValidation.h"

#include <QtCore/QObject>
#include <QtCore/QSet>

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

bool validateValueType(const QString& key, QJsonValue::Type actual, QJsonValue::Type expected, QString& errorString)
{
    // Undefined accepts any type; Null allows a double or a JSON null representing NaN.
    if (expected == QJsonValue::Undefined || actual == expected ||
        (actual == QJsonValue::Double && expected == QJsonValue::Null)) {
        return true;
    }
    errorString = QObject::tr("Incorrect value type - key:type:expected %1:%2:%3")
                      .arg(key, jsonValueTypeToString(actual), jsonValueTypeToString(expected));
    return false;
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
    QStringList missingKeys;
    for (const KeyValidateInfo& info : keyInfo) {
        const QString key = QString::fromUtf8(info.key);
        if (info.required && !jsonObject.contains(key)) {
            missingKeys.append(key);
        }
    }
    if (!validateMissingKeys(missingKeys, errorString)) {
        return false;
    }
    for (const KeyValidateInfo& info : keyInfo) {
        const QString key = QString::fromUtf8(info.key);
        const auto value = jsonObject.constFind(key);
        if (value != jsonObject.constEnd() && !validateValueType(key, value->type(), info.type, errorString)) {
            return false;
        }
    }
    return true;
}

bool validateKeysStrict(const QJsonObject& jsonObject, const QList<KeyValidateInfo>& keyInfo, QString& errorString)
{
    if (!validateKeys(jsonObject, keyInfo, errorString)) {
        return false;
    }

    QSet<QString> expectedKeys;
    expectedKeys.reserve(keyInfo.size());
    for (const KeyValidateInfo& info : keyInfo) {
        expectedKeys.insert(QString::fromUtf8(info.key));
    }

    for (auto it = jsonObject.constBegin(); it != jsonObject.constEnd(); ++it) {
        if (!expectedKeys.contains(it.key())) {
            errorString = QObject::tr("Unknown key: %1").arg(it.key());
            return false;
        }
    }

    return true;
}

}  // namespace JsonParsing
