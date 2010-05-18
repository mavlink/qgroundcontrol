#include "SlugsMAV.h"

#include <QDebug>

SlugsMAV::SlugsMAV(MAVLinkProtocol* mavlink, int id) :
        UAS(mavlink, id)//,
        // Place other initializers here
{
}


void SlugsMAV::receiveMessage(LinkInterface* link, mavlink_message_t message)
{
    // Let UAS handle the default message set
    UAS::receiveMessage(link, message);

    // Handle your special messages
    switch (message.msgid)
    {
    case MAVLINK_MSG_ID_HEARTBEAT:
        {
            qDebug() << "RECEIVED HEARTBEAT";
            break;
        }
    default:
        qDebug() << "\nSLUGS RECEIVED MESSAGE WITH ID" << message.msgid;
        break;
    }
}
