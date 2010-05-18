#include "SlugsMAV.h"

SlugsMAV::SlugsMAV(MAVLinkProtocol* mavlink, int id) :
        UAS(mavlink, id)//,
        // Place other initializers here
{
}


void SlugsMAV::receiveMessage(LinkInterface* link, mavlink_message_t message)
{
    switch (message.msgid)
    {
    case MAVLINK_MSG_ID_HEARTBEAT:
        {
            printf("RECEIVED HEARTBEAT");
            break;
        }
    default:
        printf("\nSLUGS RECEIVED MESSAGE WITH ID %d", message.msgid);
        break;
    }
}
