#include <QtCore/QObject>
#include <QtCore/QSet>

#include "JsonParsing.h"

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
        errorString =
            QObject::tr("Mismatched key and type list sizes: keys=%1 types=%2").arg(keys.count()).arg(types.count());
        return false;
    }

    for (qsizetype i = 0; i < types.count(); i++) {
        const QString& valueKey = keys[i];
        if (jsonObject.contains(valueKey)) {
            const QJsonValue& jsonValue = jsonObject[valueKey];
            if (types[i] == QJsonValue::Undefined) {
                // Undefined signals any type is acceptable (e.g. "default" whose type follows the fact type).
                continue;
            }
            if ((jsonValue.type() == QJsonValue::Double) && (types[i] == QJsonValue::Null)) {
                // Null type signals a possible NaN on a double value.
                continue;
            }
            if (jsonValue.type() != types[i]) {
                errorString =
                    QObject::tr("Incorrect value type - key:type:expected %1:%2:%3")
                        .arg(valueKey, jsonValueTypeToString(jsonValue.type()), jsonValueTypeToString(types[i]));
                return false;
            }
        }
    }

    return true;
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
    for (const KeyValidateInfo& info : keyInfo) {
        expectedKeys.insert(QLatin1String(info.key));
    }

    for (const QString& key : jsonObject.keys()) {
        if (!expectedKeys.contains(key)) {
            errorString = QObject::tr("Unknown key: %1").arg(key);
            return false;
        }
    }

    return true;
}

}  // namespace JsonParsing
