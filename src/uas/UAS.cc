/*=====================================================================

PIXHAWK Micro Air Vehicle Flying Robotics Toolkit

(c) 2009, 2010 PIXHAWK PROJECT  <http://pixhawk.ethz.ch>

This file is part of the PIXHAWK project

    PIXHAWK is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    PIXHAWK is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with PIXHAWK. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief Represents one unmanned aerial vehicle
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#include <QList>
#include <QMessageBox>
#include <QTimer>
#include <iostream>
#include <QDebug>
#include <cmath>
#include "UAS.h"
#include "LinkInterface.h"
#include "UASManager.h"
#include "MG.h"
#include "GAudioOutput.h"
#include "MAVLinkProtocol.h"
#include <mavlink.h>

UAS::UAS(MAVLinkProtocol* protocol, int id) : UASInterface(),
        uasId(id),
        startTime(MG::TIME::getGroundTimeNow()),
        commStatus(COMM_DISCONNECTED),
        name(""),
        links(new QList<LinkInterface*>()),
        unknownPackets(),
        mavlink(protocol),
        waypointManager(*this),
        thrustSum(0),
        thrustMax(10),
        startVoltage(0),
        currentVoltage(12.0f),
        lpVoltage(12.0f),
        mode(MAV_MODE_UNINIT),
        status(MAV_STATE_UNINIT),
        onboardTimeOffset(0),
        controlRollManual(true),
        controlPitchManual(true),
        controlYawManual(true),
        controlThrustManual(true),
        manualRollAngle(0),
        manualPitchAngle(0),
        manualYawAngle(0),
        manualThrust(0),
        receiveDropRate(0),
        sendDropRate(0),
        lowBattAlarm(false),
        positionLock(false),
        statusTimeout(new QTimer(this))
{
    color = UASInterface::getNextColor();
    setBattery(LIPOLY, 3);
    statusTimeout->setInterval(500);
    connect(statusTimeout, SIGNAL(timeout()), this, SLOT(updateState()));
}

UAS::~UAS()
{
    delete links;
}

int UAS::getUASID() const
{
    return uasId;
}

void UAS::updateState()
{
    // Position lock is set by the MAVLink message handler
    // if no position lock is available, indicate an error
    if (positionLock)
    {
        positionLock = false;
    }
    else
    {
        if (mode > MAV_MODE_LOCKED && positionLock)
        {
            GAudioOutput::instance()->notifyNegative();
        }
    }
}

void UAS::setSelected()
{
    UASManager::instance()->setActiveUAS(this);
}

void UAS::receiveMessage(LinkInterface* link, mavlink_message_t message)
{
    if (!links->contains(link))
    {
        addLink(link);
    }

    //qDebug() << "UAS RECEIVED" << message.sysid << message.compid << message.msgid;

    if (message.sysid == uasId)
    {
        QString uasState;
        QString stateDescription;
        QString patternPath;
        switch (message.msgid)
        {
        case MAVLINK_MSG_ID_HEARTBEAT:
            emit heartbeat(this);
            // Set new type if it has changed
            if (this->type != mavlink_msg_heartbeat_get_type(&message))
            {
                this->type = mavlink_msg_heartbeat_get_type(&message);
                emit systemTypeSet(this, type);
            }
            break;
        case MAVLINK_MSG_ID_BOOT:
            getStatusForCode((int)MAV_STATE_BOOT, uasState, stateDescription);
            emit statusChanged(this, uasState, stateDescription);
            onboardTimeOffset = 0; // Reset offset measurement
            break;
        case MAVLINK_MSG_ID_SYS_STATUS:
            {
                mavlink_sys_status_t state;
                mavlink_msg_sys_status_decode(&message, &state);

                // FIXME
                //qDebug() << "SYSTEM NAV MODE:" << state.nav_mode;

                QString audiostring = "System " + QString::number(this->getUASID());
                QString stateAudio = "";
                QString modeAudio = "";
                bool statechanged = false;
                bool modechanged = false;

                if (state.status != this->status)
                {
                    statechanged = true;
                    this->status = (int)state.status;
                    getStatusForCode((int)state.status, uasState, stateDescription);
                    emit statusChanged(this, uasState, stateDescription);
                    stateAudio = " changed status to " + uasState;
                }

                if (this->mode != static_cast<unsigned int>(state.mode))
                {
                    modechanged = true;
                    this->mode = static_cast<unsigned int>(state.mode);
                    QString mode;

                    switch (state.mode)
                    {
                    case (uint8_t)MAV_MODE_LOCKED:
                        mode = "LOCKED MODE";
                        break;
                    case (uint8_t)MAV_MODE_MANUAL:
                        mode = "MANUAL MODE";
                        break;
                    case (uint8_t)MAV_MODE_AUTO:
                        mode = "AUTO MODE";
                        break;
                    case (uint8_t)MAV_MODE_GUIDED:
                        mode = "GUIDED MODE";
                        break;
                    case (uint8_t)MAV_MODE_READY:
                        mode = "READY";
                        break;
                    case (uint8_t)MAV_MODE_TEST1:
                        mode = "TEST1 MODE";
                        break;
                    case (uint8_t)MAV_MODE_TEST2:
                        mode = "TEST2 MODE";
                        break;
                    case (uint8_t)MAV_MODE_TEST3:
                        mode = "TEST3 MODE";
                        break;
                    default:
                        mode = "UNINIT MODE";
                        break;
                    }

                    emit modeChanged(this->getUASID(), mode, "");
                    modeAudio = " is now in " + mode;
                }
                currentVoltage = state.vbat/1000.0f;
                lpVoltage = filterVoltage(currentVoltage);
                if (startVoltage == 0) startVoltage = currentVoltage;
                timeRemaining = calculateTimeRemaining();
                //qDebug() << "Voltage: " << currentVoltage << " Chargelevel: " << getChargeLevel() << " Time remaining " << timeRemaining;
                emit batteryChanged(this, lpVoltage, getChargeLevel(), timeRemaining);
                emit voltageChanged(message.sysid, state.vbat/1000.0f);

                // LOW BATTERY ALARM
                float chargeLevel = getChargeLevel();
                if (chargeLevel <= 10.0f)
                {
                    startLowBattAlarm();
                }
                else
                {
                    stopLowBattAlarm();
                }

                // COMMUNICATIONS DROP RATE
                emit dropRateChanged(this->getUASID(), state.packet_drop);
                //qDebug() << __FILE__ << __LINE__ << "RCV LOSS: " << state.packet_drop;

                // AUDIO
                if (modechanged && statechanged)
                {
                    // Output both messages
                    audiostring += modeAudio + " and " + stateAudio;
                }
                else
                {
                    // Output the one message
                    audiostring += modeAudio + stateAudio;
                }
                if ((int)state.status == (int)MAV_STATE_CRITICAL || state.status == (int)MAV_STATE_EMERGENCY)
                {
                    GAudioOutput::instance()->startEmergency();
                }
                else if (modechanged || statechanged)
                {
                    GAudioOutput::instance()->stopEmergency();
                    GAudioOutput::instance()->say(audiostring);
                }
            }
            break;
        case MAVLINK_MSG_ID_RAW_IMU:
            {
                mavlink_raw_imu_t raw;
                mavlink_msg_raw_imu_decode(&message, &raw);
                quint64 time = getUnixTime(raw.usec);

                emit valueChanged(uasId, "Accel. X", raw.xacc, time);
                emit valueChanged(uasId, "Accel. Y", raw.yacc, time);
                emit valueChanged(uasId, "Accel. Z", raw.zacc, time);
                emit valueChanged(uasId, "Gyro Phi", raw.xgyro, time);
                emit valueChanged(uasId, "Gyro Theta", raw.ygyro, time);
                emit valueChanged(uasId, "Gyro Psi", raw.zgyro, time);
                emit valueChanged(uasId, "Mag. X", raw.xmag, time);
                emit valueChanged(uasId, "Mag. Y", raw.ymag, time);
                emit valueChanged(uasId, "Mag. Z", raw.zmag, time);
            }
            break;
        case MAVLINK_MSG_ID_ATTITUDE:
            //std::cerr << std::endl;
            //std::cerr << "Decoded attitude message:" << " roll: " << std::dec << mavlink_msg_attitude_get_roll(message.payload) << " pitch: " << mavlink_msg_attitude_get_pitch(message.payload) << " yaw: " << mavlink_msg_attitude_get_yaw(message.payload) << std::endl;
            {
                mavlink_attitude_t attitude;
                mavlink_msg_attitude_decode(&message, &attitude);
                quint64 time = getUnixTime(attitude.usec);
                emit valueChanged(uasId, "roll IMU", mavlink_msg_attitude_get_roll(&message), time);
                emit valueChanged(uasId, "pitch IMU", mavlink_msg_attitude_get_pitch(&message), time);
                emit valueChanged(uasId, "yaw IMU", mavlink_msg_attitude_get_yaw(&message), time);
                emit valueChanged(this, "roll IMU", mavlink_msg_attitude_get_roll(&message), time);
                emit valueChanged(this, "pitch IMU", mavlink_msg_attitude_get_pitch(&message), time);
                emit valueChanged(this, "yaw IMU", mavlink_msg_attitude_get_yaw(&message), time);
                emit valueChanged(uasId, "rollspeed IMU", attitude.rollspeed, time);
                emit valueChanged(uasId, "pitchspeed IMU", attitude.pitchspeed, time);
                emit valueChanged(uasId, "yawspeed IMU", attitude.yawspeed, time);
                emit attitudeChanged(this, mavlink_msg_attitude_get_roll(&message), mavlink_msg_attitude_get_pitch(&message), mavlink_msg_attitude_get_yaw(&message), time);
            }
            break;
        case MAVLINK_MSG_ID_LOCAL_POSITION:
            //std::cerr << std::endl;
            //std::cerr << "Decoded attitude message:" << " roll: " << std::dec << mavlink_msg_attitude_get_roll(message.payload) << " pitch: " << mavlink_msg_attitude_get_pitch(message.payload) << " yaw: " << mavlink_msg_attitude_get_yaw(message.payload) << std::endl;
            {
                mavlink_local_position_t pos;
                mavlink_msg_local_position_decode(&message, &pos);
                quint64 time = getUnixTime(pos.usec);
                emit valueChanged(uasId, "x", pos.x, time);
                emit valueChanged(uasId, "y", pos.y, time);
                emit valueChanged(uasId, "z", pos.z, time);
                emit valueChanged(uasId, "vx", pos.vx, time);
                emit valueChanged(uasId, "vy", pos.vy, time);
                emit valueChanged(uasId, "vz", pos.vz, time);
                emit localPositionChanged(this, pos.x, pos.y, pos.z, time);
            }
            break;
        case MAVLINK_MSG_ID_GLOBAL_POSITION:
            //std::cerr << std::endl;
            //std::cerr << "Decoded attitude message:" << " roll: " << std::dec << mavlink_msg_attitude_get_roll(message.payload) << " pitch: " << mavlink_msg_attitude_get_pitch(message.payload) << " yaw: " << mavlink_msg_attitude_get_yaw(message.payload) << std::endl;
            {
                mavlink_global_position_t pos;
                mavlink_msg_global_position_decode(&message, &pos);
                quint64 time = getUnixTime(pos.usec);
                emit valueChanged(uasId, "lat", pos.lat, time);
                emit valueChanged(uasId, "lon", pos.lon, time);
                emit valueChanged(uasId, "alt", pos.alt, time);
                emit valueChanged(uasId, "g-vx", pos.vx, time);
                emit valueChanged(uasId, "g-vy", pos.vy, time);
                emit valueChanged(uasId, "g-vz", pos.vz, time);
                emit globalPositionChanged(this, pos.lon, pos.lat, pos.alt, time);
            }
            break;
        case MAVLINK_MSG_ID_GPS_RAW:
            //std::cerr << std::endl;
            //std::cerr << "Decoded attitude message:" << " roll: " << std::dec << mavlink_msg_attitude_get_roll(message.payload) << " pitch: " << mavlink_msg_attitude_get_pitch(message.payload) << " yaw: " << mavlink_msg_attitude_get_yaw(message.payload) << std::endl;
            {
                mavlink_gps_raw_t pos;
                mavlink_msg_gps_raw_decode(&message, &pos);

                // SANITY CHECK
                // only accept values in a realistic range
               // quint64 time = getUnixTime(pos.usec);
                quint64 time = MG::TIME::getGroundTimeNow();
                emit valueChanged(uasId, "lat", pos.lat, time);
                emit valueChanged(uasId, "lon", pos.lon, time);
                // Check for NaN
                int alt = pos.alt;
                if (alt != alt)
                {
                    alt = 0;
                    emit textMessageReceived(uasId, 255, "GCS ERROR: RECEIVED NaN FOR ALTITUDE");
                }
                emit valueChanged(uasId, "alt", pos.alt, time);
                // Smaller than threshold and not NaN
                if (pos.v < 1000000 && pos.v == pos.v)
                {
                    emit valueChanged(uasId, "speed", pos.v, time);
                    //qDebug() << "GOT GPS RAW";
                    emit speedChanged(this, (double)pos.v, 0.0, 0.0, time);
                }
                else
                {
                     emit textMessageReceived(uasId, 255, QString("GCS ERROR: RECEIVED INVALID SPEED OF %1 m/s").arg(pos.v));
                }
                emit globalPositionChanged(this, pos.lon, pos.lat, alt, time);
            }
            break;
        case MAVLINK_MSG_ID_GPS_STATUS:
            {
                mavlink_gps_status_t pos;
                mavlink_msg_gps_status_decode(&message, &pos);
                for(int i = 0; i < (int)pos.satellites_visible; i++)
                {
                    emit gpsSatelliteStatusChanged(uasId, (unsigned char)pos.satellite_prn[i], (unsigned char)pos.satellite_elevation[i], (unsigned char)pos.satellite_azimuth[i], (unsigned char)pos.satellite_snr[i], static_cast<bool>(pos.satellite_used[i]));
                }
            }
            break;
        case MAVLINK_MSG_ID_PARAM_VALUE:
            {
                mavlink_param_value_t value;
                mavlink_msg_param_value_decode(&message, &value);
                emit parameterChanged(uasId, message.compid, QString((char*)value.param_id), value.param_value);
            }
            break;
        case MAVLINK_MSG_ID_DEBUG:
            emit valueChanged(uasId, QString("debug ") + QString::number(mavlink_msg_debug_get_ind(&message)), mavlink_msg_debug_get_value(&message), MG::TIME::getGroundTimeNow());
            break;

        case MAVLINK_MSG_ID_WAYPOINT_COUNT:
            {
                mavlink_waypoint_count_t wpc;
                mavlink_msg_waypoint_count_decode(&message, &wpc);
                waypointManager.handleWaypointCount(message.sysid, message.compid, wpc.count);
            }
            break;

        case MAVLINK_MSG_ID_WAYPOINT:
            {
                mavlink_waypoint_t wp;
                mavlink_msg_waypoint_decode(&message, &wp);
                waypointManager.handleWaypoint(message.sysid, message.compid, &wp);
            }
            break;

        case MAVLINK_MSG_ID_WAYPOINT_REQUEST:
            {
                mavlink_waypoint_request_t wpr;
                mavlink_msg_waypoint_request_decode(&message, &wpr);
                waypointManager.handleWaypointRequest(message.sysid, message.compid, &wpr);
            }
            break;

        case MAVLINK_MSG_ID_WAYPOINT_REACHED:
            {
//                mavlink_waypoint_reached_t wp;
//                mavlink_msg_waypoint_reached_decode(&message, &wp);
//                emit waypointReached(this, wp.id);
            }
            break;
        case MAVLINK_MSG_ID_STATUSTEXT:
            {
                QByteArray b;
                b.resize(256);
                mavlink_msg_statustext_get_text(&message, (int8_t*)b.data());
                //b.append('\0');
                QString text = QString(b);
                int severity = mavlink_msg_statustext_get_severity(&message);
                //qDebug() << "RECEIVED STATUS:" << text;false
                //emit statusTextReceived(severity, text);
                emit textMessageReceived(uasId, severity, text);
            }
            break;
        default:
            {
                if (!unknownPackets.contains(message.msgid))
                {
                    unknownPackets.append(message.msgid);
                    //GAudioOutput::instance()->say("UNABLE TO DECODE MESSAGE WITH ID " + QString::number(message.msgid) + " FROM SYSTEM " + QString::number(message.sysid));
                    std::cout << "Unable to decode message from system " << std::dec << static_cast<int>(message.sysid) << " with message id:" << static_cast<int>(message.msgid) << std::endl;
                    //qDebug() << std::cerr << "Unable to decode message from system " << std::dec << static_cast<int>(message.acid) << " with message id:" << static_cast<int>(message.msgid) << std::endl;
                }
            }
            break;
        }
    }
}

quint64 UAS::getUnixTime(quint64 time)
{
    if (time == 0)
    {
        return MG::TIME::getGroundTimeNow();
    }
    // Check if time is smaller than 40 years,
    // assuming no system without Unix timestamp
    // runs longer than 40 years continuously without
    // reboot. In worst case this will add/subtract the
    // communication delay between GCS and MAV,
    // it will never alter the timestamp in a safety
    // critical way.
    //
    // Calculation:
    // 40 years
    // 365 days
    // 24 hours
    // 60 minutes
    // 60 seconds
    // 1000 milliseconds
    // 1000 microseconds
    else if (time < 1261440000000000LLU)
    {
        if (onboardTimeOffset == 0)
        {
            onboardTimeOffset = MG::TIME::getGroundTimeNow() - time/1000;
        }
        return time/1000 + onboardTimeOffset;
    }
    else
    {
        // Time is not zero and larger than 40 years -> has to be
        // a Unix epoch timestamp. Do nothing.
        return time/1000;
    }
}

void UAS::setMode(int mode)
{
    if ((uint8_t)mode >= MAV_MODE_LOCKED && (uint8_t)mode <= MAV_MODE_TEST3)
    {
        mavlink_message_t msg;
        mavlink_msg_set_mode_pack(mavlink->getSystemId(), mavlink->getComponentId(), &msg, (uint8_t)uasId, (uint8_t)mode);
        sendMessage(msg);
        qDebug() << "SENDING REQUEST TO SET MODE TO SYSTEM" << uasId << ", REQUEST TO SET MODE " << (uint8_t)mode;
    }
}

void UAS::sendMessage(mavlink_message_t message)
{
    // Emit message on all links that are currently connected
    QList<LinkInterface*>::iterator i;
    for (i = links->begin(); i != links->end(); ++i)
    {
        sendMessage(*i, message);
    }
}

void UAS::sendMessage(LinkInterface* link, mavlink_message_t message)
{
    // Create buffer
    uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
    // Write message into buffer, prepending start sign
    int len = mavlink_msg_to_send_buffer(buffer, &message);
    // If link is connected
    if (link->isConnected())
    {
        // Send the portion of the buffer now occupied by the message
        link->writeBytes((const char*)buffer, len);
    }
}

/**
 * @param value battery voltage
 */
float UAS::filterVoltage(float value) const
{
    return lpVoltage * 0.7f + value * 0.3f;
}

void UAS::getStatusForCode(int statusCode, QString& uasState, QString& stateDescription)
{
    switch (statusCode)
    {
    case MAV_STATE_UNINIT:
        uasState = tr("UNINIT");
        stateDescription = tr("Not initialized");
        break;
    case MAV_STATE_BOOT:
        uasState = tr("BOOT");
        stateDescription = tr("Booting system, please wait..");
        break;
    case MAV_STATE_CALIBRATING:
        uasState = tr("CALIBRATING");
        stateDescription = tr("Calibrating sensors..");
        break;
    case MAV_STATE_ACTIVE:
        uasState = tr("ACTIVE");
        stateDescription = tr("Normal operation mode");
        break;
    case MAV_STATE_STANDBY:
        uasState = tr("STANDBY");
        stateDescription = tr("Standby, operational");
        break;
    case MAV_STATE_CRITICAL:
        uasState = tr("CRITICAL");
        stateDescription = tr("Failure occured!");
        break;
    case MAV_STATE_EMERGENCY:
        uasState = tr("EMERGENCY");
        stateDescription = tr("EMERGENCY: Please land");
        break;
    case MAV_STATE_POWEROFF:
        uasState = tr("SHUTDOWN");
        stateDescription = tr("Powering off system");
        break;
    default:
        uasState = tr("UNKNOWN");
        stateDescription = tr("FAILURE: Unknown system state");
        break;
    }
}



/* MANAGEMENT */

/*
 *
 * @return The uptime in milliseconds
 *
 **/
quint64 UAS::getUptime() const
{
    if(startTime == 0) {
        return 0;
    } else {
        return MG::TIME::getGroundTimeNow() - startTime;
    }
}

int UAS::getCommunicationStatus() const
{
    return commStatus;
}

void UAS::requestParameters()
{
    mavlink_message_t msg;
    mavlink_msg_param_request_list_pack(mavlink->getSystemId(), mavlink->getComponentId(), &msg, this->getUASID(), 25);
    // Send message twice to increase chance of reception
    sendMessage(msg);
}

void UAS::writeParameters()
{
    //mavlink_message_t msg;
    qDebug() << __FILE__ << __LINE__ << __func__ << "IS NOT IMPLEMENTED!";
}

void UAS::enableAllDataTransmission(bool enabled)
{
    // Buffers to write data to
    mavlink_message_t msg;
    mavlink_request_data_stream_t stream;
    // Select the message to request from now on
    // 0 is a magic ID and will enable/disable the standard message set except for heartbeat
    stream.req_stream_id = 0;
    // Select the update rate in Hz the message should be send
    // All messages will be send with their default rate
    stream.req_message_rate = 0;
    // Start / stop the message
    stream.start_stop = (enabled) ? 1 : 0;
    // The system which should take this command
    stream.target_system = uasId;
    // The component / subsystem which should take this command
    stream.target_component = 0;
    // Encode and send the message
    mavlink_msg_request_data_stream_encode(mavlink->getSystemId(), mavlink->getComponentId(), &msg, &stream);
    // Send message twice to increase chance of reception
    sendMessage(msg);
    sendMessage(msg);
}

void UAS::enableRawSensorDataTransmission(bool enabled)
{
    // Buffers to write data to
    mavlink_message_t msg;
    mavlink_request_data_stream_t stream;
    // Select the message to request from now on
    stream.req_stream_id = 1;
    // Select the update rate in Hz the message should be send
    stream.req_message_rate = 200;
    // Start / stop the message
    stream.start_stop = (enabled) ? 1 : 0;
    // The system which should take this command
    stream.target_system = uasId;
    // The component / subsystem which should take this command
    stream.target_component = 0;
    // Encode and send the message
    mavlink_msg_request_data_stream_encode(mavlink->getSystemId(), mavlink->getComponentId(), &msg, &stream);
    // Send message twice to increase chance of reception
    sendMessage(msg);
    sendMessage(msg);
}

void UAS::enableExtendedSystemStatusTransmission(bool enabled)
{
    // Buffers to write data to
    mavlink_message_t msg;
    mavlink_request_data_stream_t stream;
    // Select the message to request from now on
    stream.req_stream_id = 2;
    // Select the update rate in Hz the message should be send
    stream.req_message_rate = 10;
    // Start / stop the message
    stream.start_stop = (enabled) ? 1 : 0;
    // The system which should take this command
    stream.target_system = uasId;
    // The component / subsystem which should take this command
    stream.target_component = 0;
    // Encode and send the message
    mavlink_msg_request_data_stream_encode(mavlink->getSystemId(), mavlink->getComponentId(), &msg, &stream);
    // Send message twice to increase chance of reception
    sendMessage(msg);
    sendMessage(msg);
}

void UAS::enableRCChannelDataTransmission(bool enabled)
{
    // Buffers to write data to
    mavlink_message_t msg;
    mavlink_request_data_stream_t stream;
    // Select the message to request from now on
    stream.req_stream_id = 3;
    // Select the update rate in Hz the message should be send
    stream.req_message_rate = 200;
    // Start / stop the message
    stream.start_stop = (enabled) ? 1 : 0;
    // The system which should take this command
    stream.target_system = uasId;
    // The component / subsystem which should take this command
    stream.target_component = 0;
    // Encode and send the message
    mavlink_msg_request_data_stream_encode(mavlink->getSystemId(), mavlink->getComponentId(), &msg, &stream);
    // Send message twice to increase chance of reception
    sendMessage(msg);
    sendMessage(msg);
}

void UAS::enableRawControllerDataTransmission(bool enabled)
{
    // Buffers to write data to
    mavlink_message_t msg;
    mavlink_request_data_stream_t stream;
    // Select the message to request from now on
    stream.req_stream_id = 4;
    // Select the update rate in Hz the message should be send
    stream.req_message_rate = 200;
    // Start / stop the message
    stream.start_stop = (enabled) ? 1 : 0;
    // The system which should take this command
    stream.target_system = uasId;
    // The component / subsystem which should take this command
    stream.target_component = 0;
    // Encode and send the message
    mavlink_msg_request_data_stream_encode(mavlink->getSystemId(), mavlink->getComponentId(), &msg, &stream);
    // Send message twice to increase chance of reception
    sendMessage(msg);
    sendMessage(msg);
}

void UAS::enableRawSensorFusionTransmission(bool enabled)
{
    // Buffers to write data to
    mavlink_message_t msg;
    mavlink_request_data_stream_t stream;
    // Select the message to request from now on
    stream.req_stream_id = 5;
    // Select the update rate in Hz the message should be send
    stream.req_message_rate = 200;
    // Start / stop the message
    stream.start_stop = (enabled) ? 1 : 0;
    // The system which should take this command
    stream.target_system = uasId;
    // The component / subsystem which should take this command
    stream.target_component = 0;
    // Encode and send the message
    mavlink_msg_request_data_stream_encode(mavlink->getSystemId(), mavlink->getComponentId(), &msg, &stream);
    // Send message twice to increase chance of reception
    sendMessage(msg);
    sendMessage(msg);
}

void UAS::enablePositionTransmission(bool enabled)
{
    // Buffers to write data to
    mavlink_message_t msg;
    mavlink_request_data_stream_t stream;
    // Select the message to request from now on
    stream.req_stream_id = 6;
    // Select the update rate in Hz the message should be send
    stream.req_message_rate = 200;
    // Start / stop the message
    stream.start_stop = (enabled) ? 1 : 0;
    // The system which should take this command
    stream.target_system = uasId;
    // The component / subsystem which should take this command
    stream.target_component = 0;
    // Encode and send the message
    mavlink_msg_request_data_stream_encode(mavlink->getSystemId(), mavlink->getComponentId(), &msg, &stream);
    // Send message twice to increase chance of reception
    sendMessage(msg);
    sendMessage(msg);
}

void UAS::enableExtra1Transmission(bool enabled)
{
    // Buffers to write data to
    mavlink_message_t msg;
    mavlink_request_data_stream_t stream;
    // Select the message to request from now on
    stream.req_stream_id = 7;
    // Select the update rate in Hz the message should be send
    stream.req_message_rate = 200;
    // Start / stop the message
    stream.start_stop = (enabled) ? 1 : 0;
    // The system which should take this command
    stream.target_system = uasId;
    // The component / subsystem which should take this command
    stream.target_component = 0;
    // Encode and send the message
    mavlink_msg_request_data_stream_encode(mavlink->getSystemId(), mavlink->getComponentId(), &msg, &stream);
    // Send message twice to increase chance of reception
    sendMessage(msg);
    sendMessage(msg);
}

void UAS::enableExtra2Transmission(bool enabled)
{
    // Buffers to write data to
    mavlink_message_t msg;
    mavlink_request_data_stream_t stream;
    // Select the message to request from now on
    stream.req_stream_id = 8;
    // Select the update rate in Hz the message should be send
    stream.req_message_rate = 200;
    // Start / stop the message
    stream.start_stop = (enabled) ? 1 : 0;
    // The system which should take this command
    stream.target_system = uasId;
    // The component / subsystem which should take this command
    stream.target_component = 0;
    // Encode and send the message
    mavlink_msg_request_data_stream_encode(mavlink->getSystemId(), mavlink->getComponentId(), &msg, &stream);
    // Send message twice to increase chance of reception
    sendMessage(msg);
    sendMessage(msg);
}

void UAS::enableExtra3Transmission(bool enabled)
{
    // Buffers to write data to
    mavlink_message_t msg;
    mavlink_request_data_stream_t stream;
    // Select the message to request from now on
    stream.req_stream_id = 9;
    // Select the update rate in Hz the message should be send
    stream.req_message_rate = 200;
    // Start / stop the message
    stream.start_stop = (enabled) ? 1 : 0;
    // The system which should take this command
    stream.target_system = uasId;
    // The component / subsystem which should take this command
    stream.target_component = 0;
    // Encode and send the message
    mavlink_msg_request_data_stream_encode(mavlink->getSystemId(), mavlink->getComponentId(), &msg, &stream);
    // Send message twice to increase chance of reception
    sendMessage(msg);
    sendMessage(msg);
}

void UAS::setParameter(int component, QString id, float value)
{
    mavlink_message_t msg;
    mavlink_param_set_t p;
    p.param_value = value;
    p.target_system = (uint8_t)uasId;
    p.target_component = (uint8_t)component;

    // Copy string into buffer, ensuring not to exceed the buffer size
    char* s = (char*)id.toStdString().c_str();
    for (unsigned int i = 0; i < sizeof(p.param_id); i++)
    {
        // String characters
        if ((int)i < id.length() && i < (sizeof(p.param_id) - 1))
        {
            p.param_id[i] = s[i];
        }
        // Null termination at end of string or end of buffer
        else if ((int)i == id.length() || i == (sizeof(p.param_id) - 1))
        {
            p.param_id[i] = '\0';
        }
        // Zero fill
        else
        {
            p.param_id[i] = 0;
        }
    }
    mavlink_msg_param_set_encode(mavlink->getSystemId(), mavlink->getComponentId(), &msg, &p);
    sendMessage(msg);
}

/**
 * @brief Launches the system
 *
 **/
void UAS::launch()
{
    mavlink_message_t msg;
    // TODO Replace MG System ID with static function call and allow to change ID in GUI
    mavlink_msg_action_pack(MG::SYSTEM::ID, MG::SYSTEM::COMPID, &msg, this->getUASID(),(uint8_t)MAV_ACTION_LAUNCH);
    // Send message twice to increase chance of reception
    sendMessage(msg);
    sendMessage(msg);
}

/**
 * Depending on the UAS, this might make the rotors of a helicopter spinning
 *
 **/
void UAS::enable_motors()
{
    mavlink_message_t msg;
    // TODO Replace MG System ID with static function call and allow to change ID in GUI
    mavlink_msg_action_pack(MG::SYSTEM::ID, MG::SYSTEM::COMPID, &msg, this->getUASID(),(uint8_t)MAV_ACTION_MOTORS_START);
    // Send message twice to increase chance of reception
    sendMessage(msg);
    sendMessage(msg);
}

/**
 * @warning Depending on the UAS, this might completely stop all motors.
 *
 **/
void UAS::disable_motors()
{
    mavlink_message_t msg;
    // TODO Replace MG System ID with static function call and allow to change ID in GUI
    mavlink_msg_action_pack(MG::SYSTEM::ID, MG::SYSTEM::COMPID, &msg, this->getUASID(),(uint8_t)MAV_ACTION_MOTORS_STOP);
    // Send message twice to increase chance of reception
    sendMessage(msg);
    sendMessage(msg);
}

void UAS::setManualControlCommands(double roll, double pitch, double yaw, double thrust)
{
    // Scale values
    double rollPitchScaling = 0.2f;
    double yawScaling = 0.5f;
    double thrustScaling = 1.0f;

    manualRollAngle = roll * rollPitchScaling;
    manualPitchAngle = pitch * rollPitchScaling;
    manualYawAngle = yaw * yawScaling;
    manualThrust = thrust * thrustScaling;

    if(mode == (int)MAV_MODE_MANUAL)
    {
        mavlink_message_t message;
        mavlink_msg_manual_control_pack(MG::SYSTEM::ID, MG::SYSTEM::COMPID, &message, this->uasId, (float)manualRollAngle, (float)manualPitchAngle, (float)manualYawAngle, (float)manualThrust, controlRollManual, controlPitchManual, controlYawManual, controlThrustManual);
        sendMessage(message);
        qDebug() << __FILE__ << __LINE__ << ": SENT MANUAL CONTROL MESSAGE: roll" << manualRollAngle << " pitch: " << manualPitchAngle << " yaw: " << manualYawAngle << " thrust: " << manualThrust;

        emit attitudeThrustSetPointChanged(this, roll, pitch, yaw, thrust, MG::TIME::getGroundTimeNow());
    }
}

void UAS::receiveButton(int buttonIndex)
{
    switch (buttonIndex)
    {
    case 0:

        break;
    case 1:

        break;
    default:

        break;
    }
    qDebug() << __FILE__ << __LINE__ << ": Received button clicked signal (button # is: " << buttonIndex << "), UNIMPLEMENTED IN MAVLINK!";

}


/*void UAS::requestWaypoints()
{
//    mavlink_message_t msg;
//    mavlink_msg_waypoint_request_list_pack(mavlink->getSystemId(), mavlink->getComponentId(), &msg, uasId, 25);
//    // Send message twice to increase chance of reception
//    sendMessage(msg);
    waypointManager.requestWaypoints();
    qDebug() << "UAS Request WPs";
}

void UAS::setWaypoint(Waypoint* wp)
{
//    mavlink_message_t msg;
//    mavlink_waypoint_set_t set;
//    set.id = wp->id;
//    //QString name = wp->name;
//    // FIXME Check if this works properly
//    //name.truncate(MAVLINK_MSG_WAYPOINT_SET_FIELD_NAME_LEN);
//    //strcpy((char*)set.name, name.toStdString().c_str());
//    set.autocontinue = wp->autocontinue;
//    set.target_component = 25; // FIXME
//    set.target_system = uasId;
//    set.active = wp->current;
//    set.x = wp->x;
//    set.y = wp->y;
//    set.z = wp->z;
//    set.yaw = wp->yaw;
//    mavlink_msg_waypoint_set_encode(mavlink->getSystemId(), mavlink->getComponentId(), &msg, &set);
//    // Send message twice to increase chance of reception
//    sendMessage(msg);
}

void UAS::setWaypointActive(int id)
{
//    mavlink_message_t msg;
//    mavlink_waypoint_set_active_t active;
//    active.id = id;
//    active.target_system = uasId;
//    active.target_component = 25; // FIXME
//    mavlink_msg_waypoint_set_active_encode(mavlink->getSystemId(), mavlink->getComponentId(), &msg, &active);
//    // Send message twice to increase chance of reception
//    sendMessage(msg);
//    sendMessage(msg);
//    // TODO This should be not directly emitted, but rather being fed back from the UAS
//    emit waypointSelected(getUASID(), id);
}

void UAS::clearWaypointList()
{
//    mavlink_message_t msg;
//    // FIXME
//    mavlink_waypoint_clear_list_t clist;
//    clist.target_system = uasId;
//    clist.target_component = 25;  // FIXME
//    mavlink_msg_waypoint_clear_list_encode(MG::SYSTEM::ID, MG::SYSTEM::COMPID, &msg, &clist);
//    sendMessage(msg);
//    qDebug() << "UAS clears Waypoints!";
}*/


void UAS::halt()
{
    mavlink_message_t msg;
    // TODO Replace MG System ID with static function call and allow to change ID in GUI
    mavlink_msg_action_pack(MG::SYSTEM::ID, MG::SYSTEM::COMPID, &msg, this->getUASID(), (int)MAV_ACTION_HALT);
    // Send message twice to increase chance of reception
    sendMessage(msg);
    sendMessage(msg);
}

void UAS::go()
{
    mavlink_message_t msg;
    // TODO Replace MG System ID with static function call and allow to change ID in GUI
    mavlink_msg_action_pack(MG::SYSTEM::ID, MG::SYSTEM::COMPID, &msg, this->getUASID(), (int)MAV_ACTION_CONTINUE);
    // Send message twice to increase chance of reception
    sendMessage(msg);
    sendMessage(msg);
}

/** Order the robot to return home / to land on the runway **/
void UAS::home()
{
    mavlink_message_t msg;
    // TODO Replace MG System ID with static function call and allow to change ID in GUI
    mavlink_msg_action_pack(MG::SYSTEM::ID, MG::SYSTEM::COMPID, &msg, this->getUASID(), (int)MAV_ACTION_RETURN);
    // Send message twice to increase chance of reception
    sendMessage(msg);
    sendMessage(msg);
}

/**
 * The MAV starts the emergency landing procedure. The behaviour depends on the onboard implementation
 * and might differ between systems.
 */
void UAS::emergencySTOP()
{
    mavlink_message_t msg;
    // TODO Replace MG System ID with static function call and allow to change ID in GUI
    mavlink_msg_action_pack(MG::SYSTEM::ID, MG::SYSTEM::COMPID, &msg, this->getUASID(), (int)MAV_ACTION_EMCY_LAND);
    // Send message twice to increase chance of reception
    sendMessage(msg);
    sendMessage(msg);
}

/**
 * All systems are immediately shut down (e.g. the main power line is cut).
 * @warning This might lead to a crash
 *
 * The command will not be executed until emergencyKILLConfirm is issues immediately afterwards
 */
bool UAS::emergencyKILL()
{
    bool result = false;
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setText("EMERGENCY: KILL ALL MOTORS ON UAS");
    msgBox.setInformativeText("Do you want to cut power on all systems?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    int ret = msgBox.exec();

    // Close the message box shortly after the click to prevent accidental clicks
    QTimer::singleShot(5000, &msgBox, SLOT(reject()));


    if (ret == QMessageBox::Yes)
    {
        mavlink_message_t msg;
        // TODO Replace MG System ID with static function call and allow to change ID in GUI
        mavlink_msg_action_pack(MG::SYSTEM::ID, MG::SYSTEM::COMPID, &msg, this->getUASID(), (int)MAV_ACTION_EMCY_KILL);
        // Send message twice to increase chance of reception
        sendMessage(msg);
        sendMessage(msg);
        result = true;
    }
    return result;
}

void UAS::shutdown()
{
    bool result = false;
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setText("Shutting down the UAS");
    msgBox.setInformativeText("Do you want to shut down the onboard computer?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    int ret = msgBox.exec();

    // Close the message box shortly after the click to prevent accidental clicks
    QTimer::singleShot(5000, &msgBox, SLOT(reject()));


    if (ret == QMessageBox::Yes)
    {
        // If the active UAS is set, execute command
        mavlink_message_t msg;
        // TODO Replace MG System ID with static function call and allow to change ID in GUI
        mavlink_msg_action_pack(MG::SYSTEM::ID, MG::SYSTEM::COMPID, &msg, this->getUASID(), (int)MAV_ACTION_SHUTDOWN);
        // Send message twice to increase chance of reception
        sendMessage(msg);
        sendMessage(msg);
        result = true;
    }
}

/**
 * @return The name of this system as string in human-readable form
 */
QString UAS::getUASName(void) const
{
    QString result;
    if (name == "")
    {
        result = tr("MAV ") + result.sprintf("%03d", getUASID());
    }
    else
    {
        result = name;
    }
    return result;
}

void UAS::addLink(LinkInterface* link)
{
    if (!links->contains(link))
    {
        links->append(link);
    }
    //links->append(link);
    //qDebug() << " ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK";
}

/**
 * @brief Get the links associated with this robot
 *
 **/
QList<LinkInterface*>* UAS::getLinks()
{
    return links;
}



void UAS::setBattery(BatteryType type, int cells)
{
    this->batteryType = type;
    this->cells = cells;
    switch (batteryType)
    {
    case NICD:
        break;
    case NIMH:
        break;
    case LIION:
        break;
    case LIPOLY:
        fullVoltage = this->cells * UAS::lipoFull;
        emptyVoltage = this->cells * UAS::lipoEmpty;
        break;
    case LIFE:
        break;
    case AGZN:
        break;
    }
}

int UAS::calculateTimeRemaining()
{
    quint64 dt = MG::TIME::getGroundTimeNow() - startTime;
    double seconds = dt / 1000.0f;
    double voltDifference = startVoltage - currentVoltage;
    if (voltDifference <= 0) voltDifference = 0.00000000001f;
    double dischargePerSecond = voltDifference / seconds;
    int remaining = static_cast<int>((currentVoltage - emptyVoltage) / dischargePerSecond);
    // Can never be below 0
    if (remaining < 0) remaining = 0;
    return remaining;
}

/**
 * @return charge level in percent - 0 - 100
 */
double UAS::getChargeLevel()
{
    float chargeLevel;
    if (lpVoltage < emptyVoltage)
    {
        chargeLevel = 0.0f;
    }
    else if (lpVoltage > fullVoltage)
    {
        chargeLevel = 100.0f;
    }
    else
    {
        chargeLevel = 100.0f * ((lpVoltage - emptyVoltage)/(fullVoltage - emptyVoltage));
    }
    return chargeLevel;
}

void UAS::startLowBattAlarm()
{
    if (!lowBattAlarm)
    {
        GAudioOutput::instance()->alert("LOW BATTERY");
        QTimer::singleShot(2000, GAudioOutput::instance(), SLOT(startEmergency()));
        lowBattAlarm = true;
    }
}

void UAS::stopLowBattAlarm()
{
    if (lowBattAlarm)
    {
        GAudioOutput::instance()->stopEmergency();
        lowBattAlarm = false;
    }
}
