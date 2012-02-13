/*=====================================================================
QGroundControl Open Source Ground Control Station

(c) 2009, 2010 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

    QGROUNDCONTROL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QGROUNDCONTROL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

#include "PxQuadMAV.h"
#include "GAudioOutput.h"

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
    // Only compile this portion if matching MAVLink packets have been compiled
#ifdef MAVLINK_ENABLED_PIXHAWK
    mavlink_message_t* msg = &message;

    if (message.sysid == uasId)
    {
        switch (message.msgid)
        {
        case MAVLINK_MSG_ID_RAW_AUX:
            {
            mavlink_raw_aux_t raw;
            mavlink_msg_raw_aux_decode(&message, &raw);
            quint64 time = getUnixTime(0);
            emit valueChanged(uasId, "Pressure", "raw", raw.baro, time);
            emit valueChanged(uasId, "Temperature", "raw", raw.temp, time);
        }
        break;
        case MAVLINK_MSG_ID_IMAGE_TRIGGERED:
        {
            // FIXME Kind of a hack to load data from disk
            mavlink_image_triggered_t img;
            mavlink_msg_image_triggered_decode(&message, &img);
            emit imageStarted(img.timestamp);
        }
        break;
        case MAVLINK_MSG_ID_PATTERN_DETECTED:
        {
            mavlink_pattern_detected_t detected;
            mavlink_msg_pattern_detected_decode(&message, &detected);
            QByteArray b;
            b.resize(256);
            mavlink_msg_pattern_detected_get_file(&message, b.data());
            b.append('\0');
            QString name = QString(b);
            if (detected.type == 0)
                emit patternDetected(uasId, name, detected.confidence, detected.detected);
            else if (detected.type == 1)
                emit letterDetected(uasId, name, detected.confidence, detected.detected);
        }
        break;
        case MAVLINK_MSG_ID_WATCHDOG_HEARTBEAT: {
            mavlink_watchdog_heartbeat_t payload;
            mavlink_msg_watchdog_heartbeat_decode(msg, &payload);

            emit watchdogReceived(this->uasId, payload.watchdog_id, payload.process_count);
        }
        break;

        case MAVLINK_MSG_ID_WATCHDOG_PROCESS_INFO: {
            mavlink_watchdog_process_info_t payload;
            mavlink_msg_watchdog_process_info_decode(msg, &payload);

            emit processReceived(this->uasId, payload.watchdog_id, payload.process_id, QString((const char*)payload.name), QString((const char*)payload.arguments), payload.timeout);
        }
        break;

        case MAVLINK_MSG_ID_WATCHDOG_PROCESS_STATUS: {
            mavlink_watchdog_process_status_t payload;
            mavlink_msg_watchdog_process_status_decode(msg, &payload);
            emit processChanged(this->uasId, payload.watchdog_id, payload.process_id, payload.state, (payload.muted == 1) ? true : false, payload.crashes, payload.pid);
        }
        break;
//        case MAVLINK_MSG_ID_VISION_POSITION_ESTIMATE: {
//            mavlink_vision_position_estimate_t pos;
//            mavlink_msg_vision_position_estimate_decode(&message, &pos);
//            quint64 time = getUnixTime(pos.usec);
//            //emit valueChanged(uasId, "vis. time", pos.usec, time);
//            emit valueChanged(uasId, "vis. roll", "rad", pos.roll, time);
//            emit valueChanged(uasId, "vis. pitch", "rad", pos.pitch, time);
//            emit valueChanged(uasId, "vis. yaw", "rad", pos.yaw, time);
//            emit valueChanged(uasId, "vis. x", "m", pos.x, time);
//            emit valueChanged(uasId, "vis. y", "m", pos.y, time);
//            emit valueChanged(uasId, "vis. z", "m", pos.z, time);
//        }
//        break;
//        case MAVLINK_MSG_ID_GLOBAL_VISION_POSITION_ESTIMATE: {
//            mavlink_global_vision_position_estimate_t pos;
//            mavlink_msg_global_vision_position_estimate_decode(&message, &pos);
//            quint64 time = getUnixTime(pos.usec);
//            //emit valueChanged(uasId, "vis. time", pos.usec, time);
//            emit valueChanged(uasId, "glob. vis. roll", "rad", pos.roll, time);
//            emit valueChanged(uasId, "glob. vis. pitch", "rad", pos.pitch, time);
//            emit valueChanged(uasId, "glob. vis. yaw", "rad", pos.yaw, time);
//            emit valueChanged(uasId, "glob. vis. x", "m", pos.x, time);
//            emit valueChanged(uasId, "glob. vis. y", "m", pos.y, time);
//            emit valueChanged(uasId, "glob. vis. z", "m", pos.z, time);
//        }
//        break;
//        case MAVLINK_MSG_ID_VICON_POSITION_ESTIMATE: {
//            mavlink_vicon_position_estimate_t pos;
//            mavlink_msg_vicon_position_estimate_decode(&message, &pos);
//            quint64 time = getUnixTime(pos.usec);
//            //emit valueChanged(uasId, "vis. time", pos.usec, time);
//            emit valueChanged(uasId, "vicon roll", "rad", pos.roll, time);
//            emit valueChanged(uasId, "vicon pitch", "rad", pos.pitch, time);
//            emit valueChanged(uasId, "vicon yaw", "rad", pos.yaw, time);
//            emit valueChanged(uasId, "vicon x", "m", pos.x, time);
//            emit valueChanged(uasId, "vicon y", "m", pos.y, time);
//            emit valueChanged(uasId, "vicon z", "m", pos.z, time);
//            //emit localPositionChanged(this, pos.x, pos.y, pos.z, time);
//        }
//        break;
//        case MAVLINK_MSG_ID_VISION_SPEED_ESTIMATE: {
//            mavlink_vision_speed_estimate_t speed;
//            mavlink_msg_vision_speed_estimate_decode(&message, &speed);
//            quint64 time = getUnixTime(speed.usec);
//            emit valueChanged(uasId, "vis. speed x", "m/s", speed.x, time);
//            emit valueChanged(uasId, "vis. speed y", "m/s", speed.y, time);
//            emit valueChanged(uasId, "vis. speed z", "m/s", speed.z, time);
//        }
//        break;
//        case MAVLINK_MSG_ID_CONTROL_STATUS:
//            {
//                mavlink_control_status_t status;
//                mavlink_msg_control_status_decode(&message, &status);
//                // Emit control status vector
//                emit attitudeControlEnabled(static_cast<bool>(status.control_att));
//                emit positionXYControlEnabled(static_cast<bool>(status.control_pos_xy));
//                emit positionZControlEnabled(static_cast<bool>(status.control_pos_z));
//                emit positionYawControlEnabled(static_cast<bool>(status.control_pos_yaw));

//                // Emit localization status vector
//                emit localizationChanged(this, status.position_fix);
//                emit visionLocalizationChanged(this, status.vision_fix);
//                emit gpsLocalizationChanged(this, status.gps_fix);
//            }
//            break;
//        case MAVLINK_MSG_ID_VISUAL_ODOMETRY:
//            {
//                mavlink_visual_odometry_t pos;
//                mavlink_msg_visual_odometry_decode(&message, &pos);
//                quint64 time = getUnixTime(pos.frame1_time_us);
//                //emit valueChanged(uasId, "vis. time", pos.usec, time);
//                emit valueChanged(uasId, "vis-o. roll", "rad", pos.roll, time);
//                emit valueChanged(uasId, "vis-o. pitch", "rad", pos.pitch, time);
//                emit valueChanged(uasId, "vis-o. yaw", "rad", pos.yaw, time);
//                emit valueChanged(uasId, "vis-o. x", "m", pos.x, time);
//                emit valueChanged(uasId, "vis-o. y", "m", pos.y, time);
//                emit valueChanged(uasId, "vis-o. z", "m", pos.z, time);
//            }
//            break;
//        case MAVLINK_MSG_ID_AUX_STATUS: {
//            mavlink_aux_status_t status;
//            mavlink_msg_aux_status_decode(&message, &status);
//            emit loadChanged(this, status.load/10.0f);
//            emit errCountChanged(uasId, "IMU", "I2C0", status.i2c0_err_count);
//            emit errCountChanged(uasId, "IMU", "I2C1", status.i2c1_err_count);
//            emit errCountChanged(uasId, "IMU", "SPI0", status.spi0_err_count);
//            emit errCountChanged(uasId, "IMU", "SPI1", status.spi1_err_count);
//            emit errCountChanged(uasId, "IMU", "UART", status.uart_total_err_count);
//            emit valueChanged(uasId, "Load", "%", ((float)status.load)/10.0f, getUnixTime());
//        }
//        break;
        default:
            // Let UAS handle the default message set
            UAS::receiveMessage(link, message);
            break;
        }
    }

#else
    // Let UAS handle the default message set
    UAS::receiveMessage(link, message);
    Q_UNUSED(link);
    Q_UNUSED(message);
#endif
}

#if defined(QGC_PROTOBUF_ENABLED)
void PxQuadMAV::receiveExtendedMessage(LinkInterface* link, std::tr1::shared_ptr<google::protobuf::Message> message)
{
    UAS::receiveExtendedMessage(link, message);
}

#endif

void PxQuadMAV::sendProcessCommand(int watchdogId, int processId, unsigned int command)
{
#ifdef MAVLINK_ENABLED_PIXHAWK
    mavlink_watchdog_command_t payload;
    payload.target_system_id = uasId;
    payload.watchdog_id = watchdogId;
    payload.process_id = processId;
    payload.command_id = (uint8_t)command;

    mavlink_message_t msg;
    mavlink_msg_watchdog_command_encode(mavlink->getSystemId(), mavlink->getComponentId(), &msg, &payload);
    sendMessage(msg);
#else
    Q_UNUSED(watchdogId);
    Q_UNUSED(processId);
    Q_UNUSED(command);
#endif
}
