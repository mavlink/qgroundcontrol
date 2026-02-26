#include "JsonParsing.h"

#include <limits>

#include <QtCore/QObject>
#include <QtCore/QJsonParseError>

#include "QGCFileHelper.h"
#include "QGCLoggingCategory.h"
#include "QGCNetworkHelper.h"

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
    jsonDoc = QGCNetworkHelper::parseCompressedJson(bytes, &parseError);

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
    const QByteArray jsonBytes = QGCFileHelper::readFile(fileName, &errorString);
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

}  // namespace JsonParsing
