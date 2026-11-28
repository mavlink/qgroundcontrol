#include "CompInfoActuators.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"

#include <QtCore/QJsonDocument>

QGC_LOGGING_CATEGORY(CompInfoActuatorsLog, "ComponentInformation.CompInfoActuators")

CompInfoActuators::CompInfoActuators(uint8_t compId_, Vehicle* vehicle_, QObject* parent)
    : CompInfo(COMP_METADATA_TYPE_ACTUATORS, compId_, vehicle_, parent)
{

}

QString CompInfoActuators::jsonSchema() const
{
    return QStringLiteral(":/json/component_metadata/actuators.schema.json");
}

void CompInfoActuators::setJson(const QString& fileName, const ParsedJson& json)
{
    if (fileName.isEmpty()) {
        return;
    }

    if (json.document.isNull()) {
        qCWarning(CompInfoActuatorsLog) << "Metadata json file open failed: compid:" << compId << json.readError;
        vehicle->setActuatorsMetadata(compId, fileName, QJsonDocument());
        return;
    }

    if (!json.schemaError.isEmpty()) {
        qCWarning(CompInfoActuatorsLog) << "Metadata json schema validation failed: compid:" << compId
                                        << json.schemaError;
    }

    vehicle->setActuatorsMetadata(compId, fileName, json.document);
}
