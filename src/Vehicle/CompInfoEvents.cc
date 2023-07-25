/****************************************************************************
 *
 * (c) 2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "CompInfoEvents.h"
#include "Vehicle.h"

CompInfoEvents::CompInfoEvents(uint8_t compId, Vehicle* vehicle, QObject* parent)
    : CompInfo(COMP_METADATA_TYPE_EVENTS, compId, vehicle, parent)
{

}

void CompInfoEvents::setJson(const QString& metadataJsonFileName, const QString& translationJsonFileName)
{
    vehicle->setEventsMetadata(compId, metadataJsonFileName, translationJsonFileName);
}

