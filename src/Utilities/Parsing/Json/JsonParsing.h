#pragma once

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QStringList>

namespace JsonParsing {

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

/// Returns NaN if the value is null, or the value converted to double otherwise.
double possibleNaNJsonValue(const QJsonValue& value);

}  // namespace JsonParsing
