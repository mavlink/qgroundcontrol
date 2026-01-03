#include "CompInfoActuators.h"
#include "Vehicle.h"

CompInfoActuators::CompInfoActuators(uint8_t compId, Vehicle* vehicle, QObject* parent)
    : CompInfo(COMP_METADATA_TYPE_ACTUATORS, compId, vehicle, parent)
{

}

void CompInfoActuators::setJson(const QString& metadataJsonFileName)
{
    if (!metadataJsonFileName.isEmpty()) {
        vehicle->setActuatorsMetadata(compId, metadataJsonFileName);
    }
}
