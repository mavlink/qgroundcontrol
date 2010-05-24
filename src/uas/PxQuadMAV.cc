#include "PxQuadMAV.h"

PxQuadMAV::PxQuadMAV(MAVLinkProtocol* mavlink, int id) :
        UAS(mavlink, id)
{
}

/**
 * This function is called by MAVLink once a complete, uncorrupted (CRC check valid)
 * mavlink packet is received.
 *
 * @param link Hardware link the message came from (e.g. /dev/ttyUSB0 or UDP port).
 *             messages can be sent back to the system via this link
 * @param message MAVLink message, as received from the MAVLink protocol stack
 */
void PxQuadMAV::receiveMessage(LinkInterface* link, mavlink_message_t message)
{
    // Let UAS handle the default message set
    UAS::receiveMessage(link, message);
    mavlink_message_t* msg = &message;

    // Handle your special messages
    switch (msg->msgid)
    {
    case MAVLINK_MSG_ID_WATCHDOG_HEARTBEAT:
        {
            mavlink_watchdog_heartbeat_t payload;
            mavlink_msg_watchdog_heartbeat_decode(msg, &payload);
            
            emit watchdogReceived(this->uasId, payload.watchdog_id, payload.process_count);
        }
        break;
        
    case MAVLINK_MSG_ID_WATCHDOG_PROCESS_INFO:
        {
            mavlink_watchdog_process_info_t payload;
            mavlink_msg_watchdog_process_info_decode(msg, &payload);
            
            emit processReceived(this->uasId, payload.watchdog_id, payload.process_id, QString((const char*)payload.name), QString((const char*)payload.arguments), payload.timeout);
        }
        break;
        
    case MAVLINK_MSG_ID_WATCHDOG_PROCESS_STATUS:
        {
            mavlink_watchdog_process_status_t payload;
            mavlink_msg_watchdog_process_status_decode(msg, &payload);
            emit processChanged(this->uasId, payload.watchdog_id, payload.process_id, payload.state, (payload.muted == 1) ? true : false, payload.crashes, payload.pid);
        }
        break;
    default:
        // Do nothing
        break;
    }
}
