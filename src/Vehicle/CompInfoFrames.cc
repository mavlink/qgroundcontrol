/****************************************************************************
 *
 * (c) 2022 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "CompInfoFrames.h"
#include "Vehicle.h"

CompInfoFrames::CompInfoFrames(uint8_t compId, Vehicle* vehicle, QObject* parent)
    : CompInfo(COMP_METADATA_TYPE_ACTUATORS, compId, vehicle, parent)
{

}

void CompInfoFrames::setJson(const QString& metadataJsonFileName, const QString& translationJsonFileName)
{
    if (!metadataJsonFileName.isEmpty()) {
        vehicle->setFramesMetadata(compId, metadataJsonFileName, translationJsonFileName);
    }
}

