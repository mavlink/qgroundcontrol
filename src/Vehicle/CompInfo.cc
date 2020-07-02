/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "CompInfo.h"

CompInfo::CompInfo(COMP_METADATA_TYPE type, uint8_t compId, Vehicle* vehicle, QObject* parent)
    : QObject   (parent)
    , type      (type)
    , vehicle   (vehicle)
    , compId    (compId)
{

}

void CompInfo::setMessage(const mavlink_message_t& message)
{
    mavlink_component_information_t componentInformation;

    mavlink_msg_component_information_decode(&message, &componentInformation);

    available      = true;
    uidMetaData    = componentInformation.metadata_uid;
    uidTranslation = componentInformation.translation_uid;
    uriMetaData    = componentInformation.metadata_uri;
    uriTranslation = componentInformation.translation_uri;
}
