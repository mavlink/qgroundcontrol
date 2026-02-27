#include "CompInfoEvents.h"
#include "Vehicle.h"

CompInfoEvents::CompInfoEvents(uint8_t compId_, Vehicle* vehicle_, QObject* parent)
    : CompInfo(COMP_METADATA_TYPE_EVENTS, compId_, vehicle_, parent)
{

}

void CompInfoEvents::setJson(const QString& metadataJsonFileName)
{
    vehicle->setEventsMetadata(compId, metadataJsonFileName);
}
