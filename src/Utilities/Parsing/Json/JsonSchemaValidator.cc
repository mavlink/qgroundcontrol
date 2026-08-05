#include "JsonSchemaValidator.h"

#include <QtCore/QFile>
#include <QtCore/QHash>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonParseError>
#include <QtCore/QStringList>
#include <valijson/adapters/qtjson_adapter.hpp>
#include <valijson/schema.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/validation_results.hpp>
#include <valijson/validator.hpp>

#include <memory>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(JsonSchemaValidatorLog, "Utilities.Parsing.Json.Schema")

namespace JsonSchemaValidator {

namespace {

std::shared_ptr<const valijson::Schema> loadSchema(const QString& schemaResourcePath, QString& errorString)
{
    // Unguarded static: component-metadata validation runs on the main thread only.
    static QHash<QString, std::shared_ptr<const valijson::Schema>> schemaCache;

    if (const auto cached = schemaCache.value(schemaResourcePath)) {
        return cached;
    }

    QFile schemaFile(schemaResourcePath);
    if (!schemaFile.open(QIODevice::ReadOnly)) {
        errorString = QStringLiteral("Failed to open schema resource: %1").arg(schemaResourcePath);
        return nullptr;
    }

    QJsonParseError parseError;
    const QJsonDocument schemaDoc = QJsonDocument::fromJson(schemaFile.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        errorString = QStringLiteral("Failed to parse schema %1: %2").arg(schemaResourcePath, parseError.errorString());
        return nullptr;
    }

    // QGC parsers pin the supported metadata version per component (CompInfoParam: version==1) while
    // the upstream schemas require newer ones, so drop the version range and validate structure only.
    QJsonObject schemaObj = schemaDoc.object();
    QJsonObject schemaProps = schemaObj.value(QStringLiteral("properties")).toObject();
    if (schemaProps.contains(QStringLiteral("version"))) {
        QJsonObject versionSchema = schemaProps.value(QStringLiteral("version")).toObject();
        versionSchema.remove(QStringLiteral("minimum"));
        versionSchema.remove(QStringLiteral("maximum"));
        schemaProps[QStringLiteral("version")] = versionSchema;
        schemaObj[QStringLiteral("properties")] = schemaProps;
    }

    auto schema = std::make_shared<valijson::Schema>();
    try {
        valijson::SchemaParser parser;
        const valijson::adapters::QtJsonAdapter schemaAdapter(schemaObj);
        parser.populateSchema(schemaAdapter, *schema);
    } catch (const std::exception& e) {
        errorString =
            QStringLiteral("Failed to load schema %1: %2").arg(schemaResourcePath, QString::fromUtf8(e.what()));
        return nullptr;
    }

    schemaCache.insert(schemaResourcePath, schema);
    return schema;
}

}  // namespace

bool validate(const QJsonDocument& doc, const QString& schemaResourcePath, QString& errorString)
{
    errorString.clear();

    const auto schema = loadSchema(schemaResourcePath, errorString);
    if (!schema) {
        return false;
    }

    valijson::Validator validator;
    valijson::ValidationResults results;
    const valijson::adapters::QtJsonAdapter docAdapter(doc.isArray() ? QJsonValue(doc.array())
                                                                     : QJsonValue(doc.object()));

    if (validator.validate(*schema, docAdapter, &results)) {
        return true;
    }

    QStringList failures;
    valijson::ValidationResults::Error error;
    while (results.popError(error)) {
        failures.append(QStringLiteral("%1: %2").arg(QString::fromStdString(error.jsonPointer),
                                                     QString::fromStdString(error.description)));
    }
    errorString = failures.join(QStringLiteral("; "));
    return false;
}

}  // namespace JsonSchemaValidator
