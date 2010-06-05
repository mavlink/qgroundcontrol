#include "PxMAV.h"

#include <QtCore>

PxMAV::PxMAV() :
        UAS(NULL, 0)
{
}

PxMAV::PxMAV(MAVLinkProtocol* mavlink, int id) :
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
void PxMAV::receiveMessage(LinkInterface* link, mavlink_message_t message)
{
    // Let UAS handle the default message set
    UAS::receiveMessage(link, message);
    mavlink_message_t* msg = &message;

        //qDebug() << "PX RECEIVED" << msg->sysid << msg->compid << msg->msgid;


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
    case MAVLINK_MSG_ID_DEBUG_VECT:
        {
            mavlink_debug_vect_t vect;
            mavlink_msg_debug_vect_decode(msg, &vect);
            QString str((const char*)vect.name);
            emit valueChanged(uasId, str+".x", vect.x, MG::TIME::getGroundTimeNow());
            emit valueChanged(uasId, str+".y", vect.y, MG::TIME::getGroundTimeNow());
            emit valueChanged(uasId, str+".z", vect.z, MG::TIME::getGroundTimeNow());
        }
        break;
    case MAVLINK_MSG_ID_VISION_POSITION_ESTIMATE:
        {
            mavlink_vision_position_estimate_t pos;
            mavlink_msg_vision_position_estimate_decode(&message, &pos);
            quint64 time = getUnixTime(pos.usec);
            emit valueChanged(uasId, "vis. time", pos.usec, time);
            emit valueChanged(uasId, "vis. roll", pos.roll, time);
            emit valueChanged(uasId, "vis. pitch", pos.pitch, time);
            emit valueChanged(uasId, "vis. yaw", pos.yaw, time);
            emit valueChanged(uasId, "vis. x", pos.x, time);
            emit valueChanged(uasId, "vis. y", pos.y, time);
            emit valueChanged(uasId, "vis. z", pos.z, time);
            emit valueChanged(uasId, "vis. vx", pos.vx, time);
            emit valueChanged(uasId, "vis. vy", pos.vy, time);
            emit valueChanged(uasId, "vis. vz", pos.vz, time);
            emit valueChanged(uasId, "vis. vyaw", pos.vyaw, time);
            // Set internal state
            if (!positionLock)
            {
                // If position was not locked before, notify positive
               // GAudioOutput::instance()->notifyPositive();
            }
            positionLock = true;
        }
        break;
    default:
        // Do nothing
        break;
    }
}

void PxMAV::sendProcessCommand(int watchdogId, int processId, unsigned int command)
{
    mavlink_watchdog_command_t payload;
    payload.target_system_id = uasId;
    payload.watchdog_id = watchdogId;
    payload.process_id = processId;
    payload.command_id = (uint8_t)command;

    mavlink_message_t msg;
    mavlink_msg_watchdog_command_encode(mavlink->getSystemId(), mavlink->getComponentId(), &msg, &payload);
    sendMessage(msg);
}

 Q_EXPORT_PLUGIN2(pixhawk_plugins, PxMAV)
