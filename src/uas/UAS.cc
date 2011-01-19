/*===================================================================
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
#include <QSettings>
#include <iostream>
#include <QDebug>
#include <cmath>
#include "UAS.h"
#include "LinkInterface.h"
#include "UASManager.h"
//#include "MG.h"
#include "QGC.h"
#include "GAudioOutput.h"
#include "MAVLinkProtocol.h"
#include "QGCMAVLink.h"
#include "LinkManager.h"
#include "SerialLink.h"

#ifndef M_PI
#define M_PI        3.14159265358979323846  /* pi */
#endif

#ifndef M_PI_2
#define M_PI_2      1.57079632679489661923  /* pi/2 */
#endif

#ifndef M_PI_4
#define M_PI_4      0.78539816339744830962  /* pi/4 */
#endif



UAS::UAS(MAVLinkProtocol* protocol, int id) : UASInterface(),
uasId(id),
startTime(MG::TIME::getGroundTimeNow()),
commStatus(COMM_DISCONNECTED),
name(""),
autopilot(-1),
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
localX(0.0),
localY(0.0),
localZ(0.0),
latitude(0.0),
longitude(0.0),
altitude(0.0),
roll(0.0),
pitch(0.0),
yaw(0.0),
statusTimeout(new QTimer(this)),
paramsOnceRequested(false)
{
    color = UASInterface::getNextColor();
    setBattery(LIPOLY, 3);
    connect(statusTimeout, SIGNAL(timeout()), this, SLOT(updateState()));
    statusTimeout->start(500);
    readSettings();
}

UAS::~UAS()
{
    writeSettings();
    delete links;
    links=NULL;
}

void UAS::writeSettings()
{
    QSettings settings;
    settings.beginGroup(QString("MAV%1").arg(uasId));
    settings.setValue("NAME", this->name);
    settings.endGroup();
    settings.sync();
}

void UAS::readSettings()
{
    QSettings settings;
    settings.beginGroup(QString("MAV%1").arg(uasId));
    this->name = settings.value("NAME", this->name).toString();
    settings.endGroup();
}

int UAS::getUASID() const
{
    return uasId;
}

void UAS::updateState()
{
    // Check if heartbeat timed out
    quint64 heartbeatInterval = QGC::groundTimeUsecs() - lastHeartbeat;
    if (heartbeatInterval > timeoutIntervalHeartbeat)
    {
        emit heartbeatTimeout(heartbeatInterval);
        emit heartbeatTimeout();
    }

    // Position lock is set by the MAVLink message handler
    // if no position lock is available, indicate an error
    if (positionLock)
    {
        positionLock = false;
    }
    else
    {
        if (mode > (uint8_t)MAV_MODE_LOCKED && positionLock)
        {
            GAudioOutput::instance()->notifyNegative();
        }
    }
}

void UAS::setSelected()
{
    UASManager::instance()->setActiveUAS(this);
}

void UAS::receiveMessageNamedValue(const mavlink_message_t& message)
{
    if (message.msgid == MAVLINK_MSG_ID_NAMED_VALUE_FLOAT)
    {

    }
    else if (message.msgid == MAVLINK_MSG_ID_NAMED_VALUE_INT)
    {

    }
}

void UAS::receiveMessage(LinkInterface* link, mavlink_message_t message)
{
    if (!link) return;
    if (!links->contains(link))
    {
        addLink(link);
        //        qDebug() << __FILE__ << __LINE__ << "ADDED LINK!" << link->getName();
    }
    //    else
    //    {
    //        qDebug() << __FILE__ << __LINE__ << "DID NOT ADD LINK" << link->getName() << "ALREADY IN LIST";
    //    }

    //    qDebug() << "UAS RECEIVED from" << message.sysid << "component" << message.compid << "msg id" << message.msgid << "seq no" << message.seq;

    if (message.sysid == uasId)
    {
        QString uasState;
        QString stateDescription;
        QString patternPath;
        switch (message.msgid)
        {
        case MAVLINK_MSG_ID_HEARTBEAT:
            lastHeartbeat = QGC::groundTimeUsecs();
            emit heartbeat(this);
            // Set new type if it has changed
            if (this->type != mavlink_msg_heartbeat_get_type(&message))
            {
                this->type = mavlink_msg_heartbeat_get_type(&message);
                this->autopilot = mavlink_msg_heartbeat_get_autopilot(&message);
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
                    emit statusChanged(this->status);
                    emit loadChanged(this,state.load/10.0f);
                    emit valueChanged(uasId, "Load", "%", ((float)state.load)/1000.0f, MG::TIME::getGroundTimeNow());
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
                    case (uint8_t)MAV_MODE_RC_TRAINING:
                        mode = "RC TRAINING MODE";
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
                if (chargeLevel <= 20.0f)
                {
                    startLowBattAlarm();
                }
                else
                {
                    stopLowBattAlarm();
                }

                // COMMUNICATIONS DROP RATE
                emit dropRateChanged(this->getUASID(), state.packet_drop/1000.0f);
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

                if (state.status == MAV_STATE_POWEROFF)
                {
                    emit systemRemoved(this);
                    emit systemRemoved();
                }
            }
            break;
        case MAVLINK_MSG_ID_RAW_IMU:
            {
                mavlink_raw_imu_t raw;
                mavlink_msg_raw_imu_decode(&message, &raw);
                quint64 time = getUnixTime(raw.usec);

                emit valueChanged(uasId, "accel x", "raw", raw.xacc, time);
                emit valueChanged(uasId, "accel y", "raw", raw.yacc, time);
                emit valueChanged(uasId, "accel z", "raw", raw.zacc, time);
                emit valueChanged(uasId, "gyro roll", "raw", static_cast<double>(raw.xgyro), time);
                emit valueChanged(uasId, "gyro pitch", "raw", static_cast<double>(raw.ygyro), time);
                emit valueChanged(uasId, "gyro yaw", "raw", static_cast<double>(raw.zgyro), time);
                emit valueChanged(uasId, "mag x", "raw", raw.xmag, time);
                emit valueChanged(uasId, "mag y", "raw", raw.ymag, time);
                emit valueChanged(uasId, "mag z", "raw", raw.zmag, time);
            }
            break;
        case MAVLINK_MSG_ID_ATTITUDE:
            //std::cerr << std::endl;
            //std::cerr << "Decoded attitude message:" << " roll: " << std::dec << mavlink_msg_attitude_get_roll(message.payload) << " pitch: " << mavlink_msg_attitude_get_pitch(message.payload) << " yaw: " << mavlink_msg_attitude_get_yaw(message.payload) << std::endl;
            {
                mavlink_attitude_t attitude;
                mavlink_msg_attitude_decode(&message, &attitude);
                quint64 time = getUnixTime(attitude.usec);
                roll = attitude.roll;
                pitch = attitude.pitch;
                yaw = attitude.yaw;
//                emit valueChanged(uasId, "roll IMU", mavlink_msg_attitude_get_roll(&message), time);
//                emit valueChanged(uasId, "pitch IMU", mavlink_msg_attitude_get_pitch(&message), time);
//                emit valueChanged(uasId, "yaw IMU", mavlink_msg_attitude_get_yaw(&message), time);
                emit valueChanged(uasId, "roll", "rad", mavlink_msg_attitude_get_roll(&message), time);
                emit valueChanged(uasId, "pitch", "rad", mavlink_msg_attitude_get_pitch(&message), time);
                emit valueChanged(uasId, "yaw", "rad", mavlink_msg_attitude_get_yaw(&message), time);
                emit valueChanged(uasId, "rollspeed", "rad/s", attitude.rollspeed, time);
                emit valueChanged(uasId, "pitchspeed", "rad/s", attitude.pitchspeed, time);
                emit valueChanged(uasId, "yawspeed", "rad/s", attitude.yawspeed, time);

                // Emit in angles
                emit valueChanged(uasId, "roll", "deg", (attitude.roll/M_PI)*180.0, time);
                emit valueChanged(uasId, "pitch", "deg", (attitude.pitch/M_PI)*180.0, time);

                emit valueChanged(uasId, "rollspeed", "deg/s", (attitude.rollspeed/M_PI)*180.0, time);
                emit valueChanged(uasId, "pitchspeed", "deg/s", (attitude.pitchspeed/M_PI)*180.0, time);

                // Force yaw to 180 deg range
                double yaw = ((attitude.yaw/M_PI)*180.0);
                double sign = 1.0;
                if (yaw < 0)
                {
                    sign = -1.0;
                    yaw = -yaw;
                }
                while (yaw > 180.0)
                {
                    yaw -= 180.0;
                }

                yaw *= sign;

                emit valueChanged(uasId, "yaw", "deg", yaw, time);
                emit valueChanged(uasId, "yawspeed", "deg/s", (attitude.yawspeed/M_PI)*180.0, time);

                emit attitudeChanged(this, attitude.roll, attitude.pitch, attitude.yaw, time);
            }
            break;
        case MAVLINK_MSG_ID_LOCAL_POSITION:
            //std::cerr << std::endl;
            //std::cerr << "Decoded attitude message:" << " roll: " << std::dec << mavlink_msg_attitude_get_roll(message.payload) << " pitch: " << mavlink_msg_attitude_get_pitch(message.payload) << " yaw: " << mavlink_msg_attitude_get_yaw(message.payload) << std::endl;
            {
                mavlink_local_position_t pos;
                mavlink_msg_local_position_decode(&message, &pos);
                quint64 time = getUnixTime(pos.usec);
                localX = pos.x;
                localY = pos.y;
                localZ = pos.z;
                emit valueChanged(uasId, "x", "m", pos.x, time);
                emit valueChanged(uasId, "y", "m", pos.y, time);
                emit valueChanged(uasId, "z", "m", pos.z, time);
                emit valueChanged(uasId, "x speed", "m/s", pos.vx, time);
                emit valueChanged(uasId, "y speed", "m/s", pos.vy, time);
                emit valueChanged(uasId, "z speed", "m/s", pos.vz, time);
                emit localPositionChanged(this, pos.x, pos.y, pos.z, time);
                emit speedChanged(this, pos.vx, pos.vy, pos.vz, time);

                //                qDebug()<<"Local Position = "<<pos.x<<" - "<<pos.y<<" - "<<pos.z;
                //                qDebug()<<"Speed Local Position = "<<pos.vx<<" - "<<pos.vy<<" - "<<pos.vz;

                //emit attitudeChanged(this, pos.roll, pos.pitch, pos.yaw, time);
                // Set internal state
                if (!positionLock)
                {
                    // If position was not locked before, notify positive
                    GAudioOutput::instance()->notifyPositive();
                }
                positionLock = true;
            }
            break;
        case MAVLINK_MSG_ID_GLOBAL_POSITION_INT:
            //std::cerr << std::endl;
            //std::cerr << "Decoded attitude message:" << " roll: " << std::dec << mavlink_msg_attitude_get_roll(message.payload) << " pitch: " << mavlink_msg_attitude_get_pitch(message.payload) << " yaw: " << mavlink_msg_attitude_get_yaw(message.payload) << std::endl;
            {
                mavlink_global_position_int_t pos;
                mavlink_msg_global_position_int_decode(&message, &pos);
                quint64 time = QGC::groundTimeUsecs()/1000;
                latitude = pos.lat/(double)1E7;
                longitude = pos.lon/(double)1E7;
                altitude = pos.alt/1000.0;
                speedX = pos.vx/100.0;
                speedY = pos.vy/100.0;
                speedZ = pos.vz/100.0;
                emit valueChanged(uasId, "latitude", "deg", latitude, time);
                emit valueChanged(uasId, "longitude", "deg", longitude, time);
                emit valueChanged(uasId, "altitude", "m", altitude, time);
                emit valueChanged(uasId, "gps x speed", "m/s", speedX, time);
                emit valueChanged(uasId, "gps y speed", "m/s", speedY, time);
                emit valueChanged(uasId, "gps z speed", "m/s", speedZ, time);
                emit globalPositionChanged(this, longitude, latitude, altitude, time);
                emit speedChanged(this, speedX, speedY, speedZ, time);
                // Set internal state
                if (!positionLock)
                {
                    // If position was not locked before, notify positive
                    GAudioOutput::instance()->notifyPositive();
                }
                positionLock = true;
                //TODO fix this hack for forwarding of global position for patch antenna tracking
                forwardMessage(message);
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

                emit valueChanged(uasId, "latitude", "deg", pos.lat, time);
                emit valueChanged(uasId, "longitude", "deg", pos.lon, time);

                if (pos.fix_type > 0)
                {
                    emit globalPositionChanged(this, pos.lon, pos.lat, pos.alt, time);

                    // Check for NaN
                    int alt = pos.alt;
                    if (alt != alt)
                    {
                        alt = 0;
                        emit textMessageReceived(uasId, message.compid, 255, "GCS ERROR: RECEIVED NaN FOR ALTITUDE");
                    }
                    emit valueChanged(uasId, "altitude", "m", pos.alt, time);
                    // Smaller than threshold and not NaN
                    if (pos.v < 1000000 && pos.v == pos.v)
                    {
                        emit valueChanged(uasId, "speed", "m/s", pos.v, time);
                        //qDebug() << "GOT GPS RAW";
                       // emit speedChanged(this, (double)pos.v, 0.0, 0.0, time);
                    }
                    else
                    {
                        emit textMessageReceived(uasId, message.compid, 255, QString("GCS ERROR: RECEIVED INVALID SPEED OF %1 m/s").arg(pos.v));
                    }
                }
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
        case MAVLINK_MSG_ID_GPS_LOCAL_ORIGIN_SET:
            {
                mavlink_gps_local_origin_set_t pos;
                mavlink_msg_gps_local_origin_set_decode(&message, &pos);
                // FIXME Emit to other components
            }
            break;
            case MAVLINK_MSG_ID_RAW_PRESSURE:
            {
                mavlink_raw_pressure_t pressure;
                mavlink_msg_raw_pressure_decode(&message, &pressure);
                quint64 time = this->getUnixTime(0);
                emit valueChanged(uasId, "abs pressure", "hP", pressure.press_abs, time);
                emit valueChanged(uasId, "diff pressure 1", "hP", pressure.press_diff1, time);
                emit valueChanged(uasId, "diff pressure 2", "hP", pressure.press_diff2, time);
            }
            break;
        case MAVLINK_MSG_ID_RC_CHANNELS_RAW:
            {
                mavlink_rc_channels_raw_t channels;
                mavlink_msg_rc_channels_raw_decode(&message, &channels);
                emit remoteControlRSSIChanged(channels.rssi/255.0f);
                emit remoteControlChannelRawChanged(0, channels.chan1_raw);
                emit remoteControlChannelRawChanged(1, channels.chan2_raw);
                emit remoteControlChannelRawChanged(2, channels.chan3_raw);
                emit remoteControlChannelRawChanged(3, channels.chan4_raw);
                emit remoteControlChannelRawChanged(4, channels.chan5_raw);
                emit remoteControlChannelRawChanged(5, channels.chan6_raw);
                emit remoteControlChannelRawChanged(6, channels.chan7_raw);
                emit remoteControlChannelRawChanged(7, channels.chan8_raw);
            }
            break;
        case MAVLINK_MSG_ID_RC_CHANNELS_SCALED:
            {
                mavlink_rc_channels_scaled_t channels;
                mavlink_msg_rc_channels_scaled_decode(&message, &channels);
                emit remoteControlRSSIChanged(channels.rssi/255.0f);
                emit remoteControlChannelScaledChanged(0, channels.chan1_scaled/10000.0f);
                emit remoteControlChannelScaledChanged(1, channels.chan2_scaled/10000.0f);
                emit remoteControlChannelScaledChanged(2, channels.chan3_scaled/10000.0f);
                emit remoteControlChannelScaledChanged(3, channels.chan4_scaled/10000.0f);
                emit remoteControlChannelScaledChanged(4, channels.chan5_scaled/10000.0f);
                emit remoteControlChannelScaledChanged(5, channels.chan6_scaled/10000.0f);
                emit remoteControlChannelScaledChanged(6, channels.chan7_scaled/10000.0f);
                emit remoteControlChannelScaledChanged(7, channels.chan8_scaled/10000.0f);
            }
            break;
        case MAVLINK_MSG_ID_PARAM_VALUE:
            {
                mavlink_param_value_t value;
                mavlink_msg_param_value_decode(&message, &value);

                QString parameterName = QString((char*)value.param_id);
                int component = message.compid;
                float val = value.param_value;

                // Insert component if necessary
                if (!parameters.contains(component))
                {
                    parameters.insert(component, new QMap<QString, float>());
                }

                // Insert parameter into registry
                if (parameters.value(component)->contains(parameterName)) parameters.value(component)->remove(parameterName);
                parameters.value(component)->insert(parameterName, val);

                // Emit change
                emit parameterChanged(uasId, message.compid, parameterName, val);
            }
            break;
        case MAVLINK_MSG_ID_DEBUG:
            emit valueChanged(uasId, QString("debug ") + QString::number(mavlink_msg_debug_get_ind(&message)), "raw", mavlink_msg_debug_get_value(&message), MG::TIME::getGroundTimeNow());
            break;
        case MAVLINK_MSG_ID_ATTITUDE_CONTROLLER_OUTPUT:
            {
                mavlink_attitude_controller_output_t out;
                mavlink_msg_attitude_controller_output_decode(&message, &out);
                quint64 time = MG::TIME::getGroundTimeNowUsecs();
                emit attitudeThrustSetPointChanged(this, out.roll/127.0f, out.pitch/127.0f, out.yaw/127.0f, (uint8_t)out.thrust, time);
                emit valueChanged(uasId, "att control roll", "raw", out.roll, time/1000.0f);
                emit valueChanged(uasId, "att control pitch", "raw", out.pitch, time/1000.0f);
                emit valueChanged(uasId, "att control yaw", "raw", out.yaw, time/1000.0f);
            }
            break;
        case MAVLINK_MSG_ID_POSITION_CONTROLLER_OUTPUT:
            {
                mavlink_position_controller_output_t out;
                mavlink_msg_position_controller_output_decode(&message, &out);
                quint64 time = MG::TIME::getGroundTimeNow();
                //emit positionSetPointsChanged(uasId, out.x/127.0f, out.y/127.0f, out.z/127.0f, out.yaw, time);
                emit valueChanged(uasId, "pos control x", "raw", out.x, time);
                emit valueChanged(uasId, "pos control y", "raw", out.y, time);
                emit valueChanged(uasId, "pos control z", "raw", out.z, time);
            }
            break;
        case MAVLINK_MSG_ID_WAYPOINT_COUNT:
            {
                mavlink_waypoint_count_t wpc;
                mavlink_msg_waypoint_count_decode(&message, &wpc);
                if (wpc.target_system == mavlink->getSystemId() && wpc.target_component == mavlink->getComponentId())
                {
                    waypointManager.handleWaypointCount(message.sysid, message.compid, wpc.count);
                }
            }
            break;

        case MAVLINK_MSG_ID_WAYPOINT:
            {
                mavlink_waypoint_t wp;
                mavlink_msg_waypoint_decode(&message, &wp);
                //qDebug() << "got waypoint (" << wp.seq << ") from ID " << message.sysid << " x=" << wp.x << " y=" << wp.y << " z=" << wp.z;
                if(wp.target_system == mavlink->getSystemId() && wp.target_component == mavlink->getComponentId())
                {
                    waypointManager.handleWaypoint(message.sysid, message.compid, &wp);
                }
            }
            break;

        case MAVLINK_MSG_ID_WAYPOINT_ACK:
            {
                mavlink_waypoint_ack_t wpa;
                mavlink_msg_waypoint_ack_decode(&message, &wpa);
                if(wpa.target_system == mavlink->getSystemId() && wpa.target_component == mavlink->getComponentId())
                {
                    waypointManager.handleWaypointAck(message.sysid, message.compid, &wpa);
                }
            }
            break;

        case MAVLINK_MSG_ID_WAYPOINT_REQUEST:
            {
                mavlink_waypoint_request_t wpr;
                mavlink_msg_waypoint_request_decode(&message, &wpr);
                if(wpr.target_system == mavlink->getSystemId() && wpr.target_component == mavlink->getComponentId())
                {
                    waypointManager.handleWaypointRequest(message.sysid, message.compid, &wpr);
                }
            }
            break;

        case MAVLINK_MSG_ID_WAYPOINT_REACHED:
            {
                mavlink_waypoint_reached_t wpr;
                mavlink_msg_waypoint_reached_decode(&message, &wpr);
                waypointManager.handleWaypointReached(message.sysid, message.compid, &wpr);
            }
            break;

        case MAVLINK_MSG_ID_WAYPOINT_CURRENT:
            {
                mavlink_waypoint_current_t wpc;
                mavlink_msg_waypoint_current_decode(&message, &wpc);
                waypointManager.handleWaypointCurrent(message.sysid, message.compid, &wpc);
            }
            break;

        case MAVLINK_MSG_ID_LOCAL_POSITION_SETPOINT:
            {
                mavlink_local_position_setpoint_t p;
                mavlink_msg_local_position_setpoint_decode(&message, &p);
                emit positionSetPointsChanged(uasId, p.x, p.y, p.z, p.yaw, QGC::groundTimeUsecs());
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
                emit textMessageReceived(uasId, message.compid, severity, text);
            }
            break;
    case MAVLINK_MSG_ID_DEBUG_VECT:
            {
                mavlink_debug_vect_t vect;
                mavlink_msg_debug_vect_decode(&message, &vect);
                QString str((const char*)vect.name);
                quint64 time = getUnixTime(vect.usec);
                emit valueChanged(uasId, str+".x", "raw", vect.x, time);
                emit valueChanged(uasId, str+".y", "raw", vect.y, time);
                emit valueChanged(uasId, str+".z", "raw", vect.z, time);
            }
            break;
            //#ifdef MAVLINK_ENABLED_PIXHAWK
            //            case MAVLINK_MSG_ID_POINT_OF_INTEREST:
            //            {
            //                mavlink_point_of_interest_t poi;
            //                mavlink_msg_point_of_interest_decode(&message, &poi);
            //                emit poiFound(this, poi.type, poi.color, QString((QChar*)poi.name, MAVLINK_MSG_POINT_OF_INTEREST_FIELD_NAME_LEN), poi.x, poi.y, poi.z);
            //            }
            //            break;
            //            case MAVLINK_MSG_ID_POINT_OF_INTEREST_CONNECTION:
            //            {
            //                mavlink_point_of_interest_connection_t poi;
            //                mavlink_msg_point_of_interest_connection_decode(&message, &poi);
            //                emit poiConnectionFound(this, poi.type, poi.color, QString((QChar*)poi.name, MAVLINK_MSG_POINT_OF_INTEREST_CONNECTION_FIELD_NAME_LEN), poi.x1, poi.y1, poi.z1, poi.x2, poi.y2, poi.z2);
            //            }
            //            break;
            //#endif
#ifdef MAVLINK_ENABLED_UALBERTA
        case MAVLINK_MSG_ID_NAV_FILTER_BIAS:
            {
                mavlink_nav_filter_bias_t bias;
                mavlink_msg_nav_filter_bias_decode(&message, &bias);
                quint64 time = MG::TIME::getGroundTimeNow();
                emit valueChanged(uasId, "b_f[0]", "raw", bias.accel_0, time);
                emit valueChanged(uasId, "b_f[1]", "raw", bias.accel_1, time);
                emit valueChanged(uasId, "b_f[2]", "raw", bias.accel_2, time);
                emit valueChanged(uasId, "b_w[0]", "raw", bias.gyro_0, time);
                emit valueChanged(uasId, "b_w[1]", "raw", bias.gyro_1, time);
                emit valueChanged(uasId, "b_w[2]", "raw", bias.gyro_2, time);
            }
            break;
       case MAVLINK_MSG_ID_RADIO_CALIBRATION:
            {
                mavlink_radio_calibration_t radioMsg;
                mavlink_msg_radio_calibration_decode(&message, &radioMsg);
                QVector<float> aileron;
                QVector<float> elevator;
                QVector<float> rudder;
                QVector<float> gyro;
                QVector<float> pitch;
                QVector<float> throttle;

                for (int i=0; i<MAVLINK_MSG_RADIO_CALIBRATION_FIELD_AILERON_LEN; ++i)
                    aileron << radioMsg.aileron[i];
                for (int i=0; i<MAVLINK_MSG_RADIO_CALIBRATION_FIELD_ELEVATOR_LEN; ++i)
                    elevator << radioMsg.elevator[i];
                for (int i=0; i<MAVLINK_MSG_RADIO_CALIBRATION_FIELD_RUDDER_LEN; ++i)
                    rudder << radioMsg.rudder[i];
                for (int i=0; i<MAVLINK_MSG_RADIO_CALIBRATION_FIELD_GYRO_LEN; ++i)
                    gyro << radioMsg.gyro[i];
                for (int i=0; i<MAVLINK_MSG_RADIO_CALIBRATION_FIELD_PITCH_LEN; ++i)
                    pitch << radioMsg.pitch[i];
                for (int i=0; i<MAVLINK_MSG_RADIO_CALIBRATION_FIELD_THROTTLE_LEN; ++i)
                    throttle << radioMsg.throttle[i];

                QPointer<RadioCalibrationData> radioData = new RadioCalibrationData(aileron, elevator, rudder, gyro, pitch, throttle);
                emit radioCalibrationReceived(radioData);
                delete radioData;
            }
            break;

#endif
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

void UAS::setLocalOriginAtCurrentGPSPosition()
{

    bool result = false;
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setText("Setting new World Coordinate Frame Origin");
    msgBox.setInformativeText("Do you want to set a new origin? Waypoints defined in the local frame will be shifted in their physical location");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    int ret = msgBox.exec();

    // Close the message box shortly after the click to prevent accidental clicks
    QTimer::singleShot(5000, &msgBox, SLOT(reject()));


    if (ret == QMessageBox::Yes)
    {
        mavlink_message_t msg;
        mavlink_msg_action_pack(mavlink->getSystemId(), mavlink->getSystemId(), &msg, this->getUASID(), 0, MAV_ACTION_SET_ORIGIN);
        // Send message twice to increase chance that it reaches its goal
        sendMessage(msg);
        // Wait 5 ms
        MG::SLEEP::usleep(5000);
        // Send again
        sendMessage(msg);
        result = true;
    }
}

void UAS::setLocalPositionSetpoint(float x, float y, float z, float yaw)
{
#ifdef MAVLINK_ENABLED_PIXHAWK
    mavlink_message_t msg;
    mavlink_msg_position_control_setpoint_set_pack(mavlink->getSystemId(), mavlink->getComponentId(), &msg, uasId, 0, 0, x, y, z, yaw);
    sendMessage(msg);
#else
    Q_UNUSED(x);
    Q_UNUSED(y);
    Q_UNUSED(z);
    Q_UNUSED(yaw);
#endif
}

void UAS::setLocalPositionOffset(float x, float y, float z, float yaw)
{
#ifdef MAVLINK_ENABLED_PIXHAWK
    mavlink_message_t msg;
    mavlink_msg_position_control_offset_set_pack(mavlink->getSystemId(), mavlink->getComponentId(), &msg, uasId, 0, x, y, z, yaw);
    sendMessage(msg);
#else
    Q_UNUSED(x);
    Q_UNUSED(y);
    Q_UNUSED(z);
    Q_UNUSED(yaw);
#endif
}

void UAS::startRadioControlCalibration()
{
    mavlink_message_t msg;
    mavlink_msg_action_pack(mavlink->getSystemId(), mavlink->getComponentId(), &msg, uasId, MAV_COMP_ID_IMU, MAV_ACTION_CALIBRATE_RC);
    sendMessage(msg);
}

void UAS::startDataRecording()
{
    mavlink_message_t msg;
    mavlink_msg_action_pack(mavlink->getSystemId(), mavlink->getComponentId(), &msg, uasId, MAV_COMP_ID_IMU, MAV_ACTION_REC_START);
    sendMessage(msg);
}

void UAS::pauseDataRecording()
{
    mavlink_message_t msg;
    mavlink_msg_action_pack(mavlink->getSystemId(), mavlink->getComponentId(), &msg, uasId, MAV_COMP_ID_IMU, MAV_ACTION_REC_PAUSE);
    sendMessage(msg);
}

void UAS::stopDataRecording()
{
    mavlink_message_t msg;
    mavlink_msg_action_pack(mavlink->getSystemId(), mavlink->getComponentId(), &msg, uasId, MAV_COMP_ID_IMU, MAV_ACTION_REC_STOP);
    sendMessage(msg);
}

void UAS::startMagnetometerCalibration()
{
    mavlink_message_t msg;
    mavlink_msg_action_pack(mavlink->getSystemId(), mavlink->getComponentId(), &msg, uasId, MAV_COMP_ID_IMU, MAV_ACTION_CALIBRATE_MAG);
    sendMessage(msg);
}

void UAS::startGyroscopeCalibration()
{
    mavlink_message_t msg;
    mavlink_msg_action_pack(mavlink->getSystemId(), mavlink->getComponentId(), &msg, uasId, MAV_COMP_ID_IMU, MAV_ACTION_CALIBRATE_GYRO);
    sendMessage(msg);
}

void UAS::startPressureCalibration()
{
    mavlink_message_t msg;
    mavlink_msg_action_pack(mavlink->getSystemId(), mavlink->getComponentId(), &msg, uasId, MAV_COMP_ID_IMU, MAV_ACTION_CALIBRATE_PRESSURE);
    sendMessage(msg);
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
#ifndef _MSC_VER
    else if (time < 1261440000000000LLU)
#else
        else if (time < 1261440000000000)
#endif
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

QList<QString> UAS::getParameterNames(int component)
{
    if (parameters.contains(component))
    {
        return parameters.value(component)->keys();
    }
    else
    {
        return QList<QString>();
    }
}

QList<int> UAS::getComponentIds()
{
    return parameters.keys();
}

void UAS::setMode(int mode)
{
    if ((uint8_t)mode >= MAV_MODE_LOCKED && (uint8_t)mode <= MAV_MODE_RC_TRAINING)
    {
        this->mode = mode;
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

void UAS::forwardMessage(mavlink_message_t message)
{
    // Emit message on all links that are currently connected
    QList<LinkInterface*>link_list = LinkManager::instance()->getLinksForProtocol(mavlink);

    foreach(LinkInterface* link, link_list)
    {
        if (link)
        {
            SerialLink* serial = dynamic_cast<SerialLink*>(link);
            if(serial != 0)
            {

                for(int i=0;i<links->size();i++)
                {
                    if(serial != links->at(i))
                    {
                        qDebug()<<"Forwarding Over link: "<<serial->getName()<<" "<<serial;
                        sendMessage(serial, message);
                    }
                }
            }
        }
    }
}

void UAS::sendMessage(LinkInterface* link, mavlink_message_t message)
{
    if(!link) return;
    // Create buffer
    uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
    // Write message into buffer, prepending start sign
    int len = mavlink_msg_to_send_buffer(buffer, &message);
    mavlink_finalize_message_chan(&message, mavlink->getSystemId(), mavlink->getComponentId(), link->getId(), message.len);
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
        stateDescription = tr("Waiting..");
        break;
    case MAV_STATE_BOOT:
        uasState = tr("BOOT");
        stateDescription = tr("Booting..");
        break;
    case MAV_STATE_CALIBRATING:
        uasState = tr("CALIBRATING");
        stateDescription = tr("Calibrating..");
        break;
    case MAV_STATE_ACTIVE:
        uasState = tr("ACTIVE");
        stateDescription = tr("Normal");
        break;
    case MAV_STATE_STANDBY:
        uasState = tr("STANDBY");
        stateDescription = tr("Standby, OK");
        break;
    case MAV_STATE_CRITICAL:
        uasState = tr("CRITICAL");
        stateDescription = tr("FAILURE: Continue");
        break;
    case MAV_STATE_EMERGENCY:
        uasState = tr("EMERGENCY");
        stateDescription = tr("EMERGENCY: Please land");
        break;
    case MAV_STATE_POWEROFF:
        uasState = tr("SHUTDOWN");
        stateDescription = tr("Powering off");
        break;
    default:
        uasState = tr("UNKNOWN");
        stateDescription = tr("Unknown state");
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

void UAS::writeParametersToStorage()
{
    mavlink_message_t msg;
    // TODO Replace MG System ID with static function call and allow to change ID in GUI
    mavlink_msg_action_pack(MG::SYSTEM::ID, MG::SYSTEM::COMPID, &msg, this->getUASID(),MAV_COMP_ID_IMU, (uint8_t)MAV_ACTION_STORAGE_WRITE);
    //mavlink_msg_action_pack(MG::SYSTEM::ID, MG::SYSTEM::COMPID, &msg, this->getUASID(),(uint8_t)MAV_ACTION_STORAGE_WRITE);
    sendMessage(msg);
}

void UAS::readParametersFromStorage()
{
    mavlink_message_t msg;
    // TODO Replace MG System ID with static function call and allow to change ID in GUI
    mavlink_msg_action_pack(MG::SYSTEM::ID, MG::SYSTEM::COMPID, &msg, this->getUASID(), MAV_COMP_ID_IMU,(uint8_t)MAV_ACTION_STORAGE_READ);
    sendMessage(msg);
}

void UAS::enableAllDataTransmission(int rate)
{
    // Buffers to write data to
    mavlink_message_t msg;
    mavlink_request_data_stream_t stream;
    // Select the message to request from now on
    // 0 is a magic ID and will enable/disable the standard message set except for heartbeat
    stream.req_stream_id = MAV_DATA_STREAM_ALL;
    // Select the update rate in Hz the message should be send
    // All messages will be send with their default rate
    // TODO: use 0 to turn off and get rid of enable/disable? will require
    //  a different magic flag for turning on defaults, possibly something really high like 1111 ?
    stream.req_message_rate = 0;
    // Start / stop the message
    stream.start_stop = (rate) ? 1 : 0;
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

void UAS::enableRawSensorDataTransmission(int rate)
{
    // Buffers to write data to
    mavlink_message_t msg;
    mavlink_request_data_stream_t stream;
    // Select the message to request from now on
    stream.req_stream_id = MAV_DATA_STREAM_RAW_SENSORS;
    // Select the update rate in Hz the message should be send
    stream.req_message_rate = rate;
    // Start / stop the message
    stream.start_stop = (rate) ? 1 : 0;
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

void UAS::enableExtendedSystemStatusTransmission(int rate)
{
    // Buffers to write data to
    mavlink_message_t msg;
    mavlink_request_data_stream_t stream;
    // Select the message to request from now on
    stream.req_stream_id = MAV_DATA_STREAM_EXTENDED_STATUS;
    // Select the update rate in Hz the message should be send
    stream.req_message_rate = rate;
    // Start / stop the message
    stream.start_stop = (rate) ? 1 : 0;
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

void UAS::enableRCChannelDataTransmission(int rate)
{
#if defined(MAVLINK_ENABLED_UALBERTA_MESSAGES)
    mavlink_message_t msg;
    mavlink_msg_request_rc_channels_pack(mavlink->getSystemId(), mavlink->getComponentId(), &msg, enabled);
    sendMessage(msg);
#else
    mavlink_message_t msg;
    mavlink_request_data_stream_t stream;
    // Select the message to request from now on
    stream.req_stream_id = MAV_DATA_STREAM_RC_CHANNELS;
    // Select the update rate in Hz the message should be send
    stream.req_message_rate = rate;
    // Start / stop the message
    stream.start_stop = (rate) ? 1 : 0;
    // The system which should take this command
    stream.target_system = uasId;
    // The component / subsystem which should take this command
    stream.target_component = 0;
    // Encode and send the message
    mavlink_msg_request_data_stream_encode(mavlink->getSystemId(), mavlink->getComponentId(), &msg, &stream);
    // Send message twice to increase chance of reception
    sendMessage(msg);
    sendMessage(msg);
#endif
}

void UAS::enableRawControllerDataTransmission(int rate)
{
    // Buffers to write data to
    mavlink_message_t msg;
    mavlink_request_data_stream_t stream;
    // Select the message to request from now on
    stream.req_stream_id = MAV_DATA_STREAM_RAW_CONTROLLER;
    // Select the update rate in Hz the message should be send
    stream.req_message_rate = rate;
    // Start / stop the message
    stream.start_stop = (rate) ? 1 : 0;
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

void UAS::enableRawSensorFusionTransmission(int rate)
{
    // Buffers to write data to
    mavlink_message_t msg;
    mavlink_request_data_stream_t stream;
    // Select the message to request from now on
    stream.req_stream_id = MAV_DATA_STREAM_RAW_SENSOR_FUSION;
    // Select the update rate in Hz the message should be send
    stream.req_message_rate = rate;
    // Start / stop the message
    stream.start_stop = (rate) ? 1 : 0;
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

void UAS::enablePositionTransmission(int rate)
{
    // Buffers to write data to
    mavlink_message_t msg;
    mavlink_request_data_stream_t stream;
    // Select the message to request from now on
    stream.req_stream_id = MAV_DATA_STREAM_POSITION;
    // Select the update rate in Hz the message should be send
    stream.req_message_rate = rate;
    // Start / stop the message
    stream.start_stop = (rate) ? 1 : 0;
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

void UAS::enableExtra1Transmission(int rate)
{
    // Buffers to write data to
    mavlink_message_t msg;
    mavlink_request_data_stream_t stream;
    // Select the message to request from now on
    stream.req_stream_id = MAV_DATA_STREAM_EXTRA1;
    // Select the update rate in Hz the message should be send
    stream.req_message_rate = rate;
    // Start / stop the message
    stream.start_stop = (rate) ? 1 : 0;
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

void UAS::enableExtra2Transmission(int rate)
{
    // Buffers to write data to
    mavlink_message_t msg;
    mavlink_request_data_stream_t stream;
    // Select the message to request from now on
    stream.req_stream_id = MAV_DATA_STREAM_EXTRA2;
    // Select the update rate in Hz the message should be send
    stream.req_message_rate = rate;
    // Start / stop the message
    stream.start_stop = (rate) ? 1 : 0;
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

void UAS::enableExtra3Transmission(int rate)
{
    // Buffers to write data to
    mavlink_message_t msg;
    mavlink_request_data_stream_t stream;
    // Select the message to request from now on
    stream.req_stream_id = MAV_DATA_STREAM_EXTRA3;
    // Select the update rate in Hz the message should be send
    stream.req_message_rate = rate;
    // Start / stop the message
    stream.start_stop = (rate) ? 1 : 0;
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

/**
 * Set a parameter value onboard
 *
 * @param component The component to set the parameter
 * @param id Name of the parameter
 * @param value Parameter value
 */
void UAS::setParameter(const int component, const QString& id, const float value)
{
    if (!id.isNull())
    {
    mavlink_message_t msg;
    mavlink_param_set_t p;
    p.param_value = value;
    p.target_system = (uint8_t)uasId;
    p.target_component = (uint8_t)component;

    // Copy string into buffer, ensuring not to exceed the buffer size    
    for (unsigned int i = 0; i < sizeof(p.param_id); i++)
    {
        // String characters
        if ((int)i < id.length() && i < (sizeof(p.param_id) - 1))
        {
            p.param_id[i] = id.toAscii()[i];
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
}

void UAS::setUASName(const QString& name)
{
    this->name = name;
    writeSettings();
    emit nameChanged(name);
}

/**
 * Sets an action
 *
 **/
void UAS::setAction(MAV_ACTION action)
{
    mavlink_message_t msg;
    mavlink_msg_action_pack(mavlink->getSystemId(), mavlink->getComponentId(), &msg, this->getUASID(), 0, action);
    // Send message twice to increase chance that it reaches its goal
    sendMessage(msg);
    sendMessage(msg);
}

/**
 * Launches the system
 *
 **/
void UAS::launch()
{
    mavlink_message_t msg;
    // TODO Replace MG System ID with static function call and allow to change ID in GUI
    mavlink_msg_action_pack(MG::SYSTEM::ID, MG::SYSTEM::COMPID, &msg, this->getUASID(), MAV_COMP_ID_IMU, (uint8_t)MAV_ACTION_LAUNCH);
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
    mavlink_msg_action_pack(MG::SYSTEM::ID, MG::SYSTEM::COMPID, &msg, this->getUASID(), MAV_COMP_ID_IMU, (uint8_t)MAV_ACTION_MOTORS_START);
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
    mavlink_msg_action_pack(MG::SYSTEM::ID, MG::SYSTEM::COMPID, &msg, this->getUASID(), MAV_COMP_ID_IMU, (uint8_t)MAV_ACTION_MOTORS_STOP);
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
        mavlink_msg_manual_control_pack(mavlink->getSystemId(), mavlink->getComponentId(), &message, this->uasId, (float)manualRollAngle, (float)manualPitchAngle, (float)manualYawAngle, (float)manualThrust, controlRollManual, controlPitchManual, controlYawManual, controlThrustManual);
        sendMessage(message);
        qDebug() << __FILE__ << __LINE__ << ": SENT MANUAL CONTROL MESSAGE: roll" << manualRollAngle << " pitch: " << manualPitchAngle << " yaw: " << manualYawAngle << " thrust: " << manualThrust;

        emit attitudeThrustSetPointChanged(this, roll, pitch, yaw, thrust, MG::TIME::getGroundTimeNow());
    }
    else
    {
        qDebug() << "JOYSTICK/MANUAL CONTROL: IGNORING COMMANDS: Set mode to MANUAL to send joystick commands first";
    }
}

int UAS::getSystemType()
{
    return this->type;
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
    //    qDebug() << __FILE__ << __LINE__ << ": Received button clicked signal (button # is: " << buttonIndex << "), UNIMPLEMENTED IN MAVLINK!";

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
    mavlink_msg_action_pack(MG::SYSTEM::ID, MG::SYSTEM::COMPID, &msg, this->getUASID(), MAV_COMP_ID_IMU, (int)MAV_ACTION_HALT);
    // Send message twice to increase chance of reception
    sendMessage(msg);
    sendMessage(msg);
}

void UAS::go()
{
    mavlink_message_t msg;
    // TODO Replace MG System ID with static function call and allow to change ID in GUI
    mavlink_msg_action_pack(MG::SYSTEM::ID, MG::SYSTEM::COMPID, &msg, this->getUASID(), MAV_COMP_ID_IMU,  (int)MAV_ACTION_CONTINUE);
    // Send message twice to increase chance of reception
    sendMessage(msg);
    sendMessage(msg);
}

/** Order the robot to return home / to land on the runway **/
void UAS::home()
{
    mavlink_message_t msg;
    // TODO Replace MG System ID with static function call and allow to change ID in GUI
    mavlink_msg_action_pack(MG::SYSTEM::ID, MG::SYSTEM::COMPID, &msg, this->getUASID(), MAV_COMP_ID_IMU,  (int)MAV_ACTION_RETURN);
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
    mavlink_msg_action_pack(MG::SYSTEM::ID, MG::SYSTEM::COMPID, &msg, this->getUASID(), MAV_COMP_ID_IMU, (int)MAV_ACTION_EMCY_LAND);
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
        mavlink_msg_action_pack(MG::SYSTEM::ID, MG::SYSTEM::COMPID, &msg, this->getUASID(), MAV_COMP_ID_IMU, (int)MAV_ACTION_EMCY_KILL);
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
        mavlink_msg_action_pack(MG::SYSTEM::ID, MG::SYSTEM::COMPID, &msg, this->getUASID(), MAV_COMP_ID_IMU,(int)MAV_ACTION_SHUTDOWN);
        // Send message twice to increase chance of reception
        sendMessage(msg);
        sendMessage(msg);
        result = true;
    }
}

void UAS::setTargetPosition(float x, float y, float z, float yaw)
{
    mavlink_message_t msg;
    mavlink_msg_position_target_pack(MG::SYSTEM::ID, MG::SYSTEM::COMPID, &msg, x, y, z, yaw);

    // Send message twice to increase chance of reception
    sendMessage(msg);
    sendMessage(msg);
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
        connect(link, SIGNAL(destroyed(QObject*)), this, SLOT(removeLink(QObject*)));
    }
    //links->append(link);
    //qDebug() << link<<" ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK ADDED LINK";
}

void UAS::removeLink(QObject* object)
{
    LinkInterface* link = dynamic_cast<LinkInterface*>(object);
    if (link)
    {
        links->removeAt(links->indexOf(link));
    }
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
