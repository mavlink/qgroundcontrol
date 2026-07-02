#pragma once

#include <QtCore/QString>

class QJsonDocument;

namespace JsonSchemaValidator {

/// Validates `doc` against the JSON-Schema bundled at `schemaResourcePath`
/// (e.g. ":/json/component_metadata/parameter.schema.json").
/// @return true if `doc` conforms; on failure returns false and fills
///         `errorString` with the validation failures.
bool validate(const QJsonDocument &doc, const QString &schemaResourcePath, QString &errorString);

}  // namespace JsonSchemaValidator
