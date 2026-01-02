#include "CompInfoEvents.h"
#include "Vehicle.h"

CompInfoEvents::CompInfoEvents(uint8_t compId, Vehicle* vehicle, QObject* parent)
    : CompInfo(COMP_METADATA_TYPE_EVENTS, compId, vehicle, parent)
{

}

void CompInfoEvents::setJson(const QString& metadataJsonFileName)
{
    vehicle->setEventsMetadata(compId, metadataJsonFileName);
}
