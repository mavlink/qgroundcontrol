#pragma once

#include <QtCore/QJsonObject>
#include <QtCore/QStringList>

namespace JsonParsing {

/// Validates that all listed keys are present in the object.
bool validateRequiredKeys(const QJsonObject& jsonObject, const QStringList& keys, QString& errorString);

/// Validates value types for listed keys that are present in the object.
/// `QJsonValue::Null` as expected type means "double value with possible NaN".
bool validateKeyTypes(const QJsonObject& jsonObject, const QStringList& keys, const QList<QJsonValue::Type>& types,
                      QString& errorString);

struct KeyValidateInfo
{
    const char* key;        ///< json key name
    QJsonValue::Type type;  ///< required type for key, QJsonValue::Null specifies double with possible NaN
    bool required;          ///< true: key must be present
};

/// Validates that all required keys are present and that listed keys have the expected type.
bool validateKeys(const QJsonObject& jsonObject, const QList<KeyValidateInfo>& keyInfo, QString& errorString);

/// Validates keys like `validateKeys` but also rejects any keys not listed in `keyInfo`.
bool validateKeysStrict(const QJsonObject& jsonObject, const QList<KeyValidateInfo>& keyInfo, QString& errorString);

}  // namespace JsonParsing
