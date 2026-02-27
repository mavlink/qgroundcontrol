#include "CompInfoActuators.h"
#include "Vehicle.h"

CompInfoActuators::CompInfoActuators(uint8_t compId_, Vehicle* vehicle_, QObject* parent)
    : CompInfo(COMP_METADATA_TYPE_ACTUATORS, compId_, vehicle_, parent)
{

}

void CompInfoActuators::setJson(const QString& metadataJsonFileName)
{
    if (!metadataJsonFileName.isEmpty()) {
        vehicle->setActuatorsMetadata(compId, metadataJsonFileName);
    }
}
