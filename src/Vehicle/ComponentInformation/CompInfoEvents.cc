#include "CompInfoEvents.h"

#include "EventHandler.h"
#include "Vehicle.h"

CompInfoEvents::CompInfoEvents(uint8_t compId_, Vehicle* vehicle_, QObject* parent)
    : CompInfo(COMP_METADATA_TYPE_EVENTS, compId_, vehicle_, parent)
{

}

CompInfo::FileParser CompInfoEvents::fileParser() const
{
    return [](const QString& fileName) {
        ParsedJson parsed;
        const EventMetadata metadata = EventHandler::loadMetadata(fileName);
        parsed.read = (metadata != nullptr);
        parsed.data = metadata;
        return parsed;
    };
}

void CompInfoEvents::setJson([[maybe_unused]] const QString& fileName, const ParsedJson& json)
{
    if (!vehicle) {
        return;
    }
    const EventMetadata* const metadata = std::any_cast<EventMetadata>(&json.data);
    vehicle->setEventsMetadata(compId, metadata ? *metadata : nullptr);
}
