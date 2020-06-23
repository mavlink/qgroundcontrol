/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "ComponentInformation.h"
#include "Vehicle.h"

QGC_LOGGING_CATEGORY(ComponentInformation, "ComponentInformation")

ComponentInformation::ComponentInformation(Vehicle* vehicle, QObject* parent)
    : QObject   (parent)
    , _vehicle  (vehicle)
{

}

void ComponentInformation::requestVersionMetaData(Vehicle* vehicle)
{
    vehicle->sendMavCommand(MAV_COMP_ID_AUTOPILOT1,
                            MAV_CMD_REQUEST_MESSAGE,
                            false,                                   // No error shown if fails
                            MAVLINK_MSG_ID_COMPONENT_INFORMATION,
                            COMP_METADATA_TYPE_VERSION);
}

bool ComponentInformation::requestParameterMetaData(Vehicle* vehicle)
{
    vehicle->sendMavCommand(MAV_COMP_ID_AUTOPILOT1,
                            MAV_CMD_REQUEST_MESSAGE,
                            false,                                   // No error shown if fails
                            MAVLINK_MSG_ID_COMPONENT_INFORMATION,
                            COMP_METADATA_TYPE_PARAMETER);
    return true;
}

void ComponentInformation::componentInformationReceived(const mavlink_message_t &message)
{
    mavlink_component_information_t componentInformation;
    mavlink_msg_component_information_decode(&message, &componentInformation);    
    qDebug() << componentInformation.metadata_type << componentInformation.metadata_uri;
}

bool ComponentInformation::metaDataTypeSupported(COMP_METADATA_TYPE type)
{
    return _supportedMetaDataTypes.contains(type);
}
