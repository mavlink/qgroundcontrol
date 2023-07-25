/****************************************************************************
 *
 * (c) 2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "CompInfoActuators.h"
#include "Vehicle.h"

CompInfoActuators::CompInfoActuators(uint8_t compId, Vehicle* vehicle, QObject* parent)
    : CompInfo(COMP_METADATA_TYPE_ACTUATORS, compId, vehicle, parent)
{

}

void CompInfoActuators::setJson(const QString& metadataJsonFileName, const QString& translationJsonFileName)
{
    if (!metadataJsonFileName.isEmpty()) {
        vehicle->setActuatorsMetadata(compId, metadataJsonFileName, translationJsonFileName);
    }
}

