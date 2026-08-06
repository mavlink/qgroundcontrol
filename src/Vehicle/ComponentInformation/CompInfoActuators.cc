#include "CompInfoActuators.h"
#include "JsonParsing.h"
#include "JsonSchemaValidator.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"

#include <QtCore/QJsonDocument>

QGC_LOGGING_CATEGORY(CompInfoActuatorsLog, "ComponentInformation.CompInfoActuators")

CompInfoActuators::CompInfoActuators(uint8_t compId_, Vehicle* vehicle_, QObject* parent)
    : CompInfo(COMP_METADATA_TYPE_ACTUATORS, compId_, vehicle_, parent)
{

}

void CompInfoActuators::setJson(const QString& metadataJsonFileName)
{
    if (metadataJsonFileName.isEmpty()) {
        return;
    }

    QString errorString;
    QJsonDocument jsonDoc;
    if (!JsonParsing::isJsonFile(metadataJsonFileName, jsonDoc, errorString)) {
        qCWarning(CompInfoActuatorsLog) << "Metadata json file open failed: compid:" << compId << errorString;
        vehicle->setActuatorsMetadata(compId, metadataJsonFileName, QJsonDocument());
        return;
    }

    QString schemaError;
    if (!JsonSchemaValidator::validate(jsonDoc, QStringLiteral(":/json/component_metadata/actuators.schema.json"), schemaError)) {
        qCWarning(CompInfoActuatorsLog) << "Metadata json schema validation failed: compid:" << compId << schemaError;
    }

    vehicle->setActuatorsMetadata(compId, metadataJsonFileName, jsonDoc);
}
