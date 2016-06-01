/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief Represents one unmanned aerial vehicle
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#include <QList>
#include <QTimer>
#include <QSettings>
#include <iostream>
#include <QDebug>

#include <cmath>
#include <qmath.h>

#include <limits>
#include <cstdlib>

#include "UAS.h"
#include "LinkInterface.h"
#include "HomePositionManager.h"
#include "QGC.h"
#include "GAudioOutput.h"
#include "MAVLinkProtocol.h"
#include "QGCMAVLink.h"
#include "LinkManager.h"
#ifndef __ios__
#include "SerialLink.h"
#endif
#include <Eigen/Geometry>
#include "FirmwarePluginManager.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"
#include "Joystick.h"
#include "QGCApplication.h"
#include "MissionCommands.h"

QGC_LOGGING_CATEGORY(UASLog, "UASLog")

/**
* Gets the settings from the previous UAS (name, airframe, autopilot, battery specs)
* by calling readSettings. This means the new UAS will have the same settings
* as the previous one created unless one calls deleteSettings in the code after
* creating the UAS.
*/

UAS::UAS(MAVLinkProtocol* protocol, Vehicle* vehicle, FirmwarePluginManager * firmwarePluginManager) : UASInterface(),
    lipoFull(4.2f),
    lipoEmpty(3.5f),
    uasId(vehicle->id()),
    unknownPackets(),
    mavlink(protocol),
    receiveDropRate(0),
    sendDropRate(0),

    status(-1),

    startTime(QGC::groundTimeMilliseconds()),
    onboardTimeOffset(0),

    controlRollManual(true),
    controlPitchManual(true),
    controlYawManual(true),
    controlThrustManual(true),
    manualRollAngle(0),
    manualPitchAngle(0),
    manualYawAngle(0),
    manualThrust(0),

    isGlobalPositionKnown(false),

    latitude(0.0),
    longitude(0.0),
    altitudeAMSL(0.0),
    altitudeAMSLFT(0.0),
    altitudeRelative(0.0),

    satRawHDOP(1e10f),
    satRawVDOP(1e10f),
    satRawCOG(0.0),

    globalEstimatorActive(false),

    latitude_gps(0.0),
    longitude_gps(0.0),
    altitude_gps(0.0),

    speedX(0.0),
    speedY(0.0),
    speedZ(0.0),

    airSpeed(std::numeric_limits<double>::quiet_NaN()),
    groundSpeed(std::numeric_limits<double>::quiet_NaN()),
#ifndef __mobile__
    fileManager(this, vehicle),
#endif

    attitudeKnown(false),
    attitudeStamped(false),
    lastAttitude(0),

    roll(0.0),
    pitch(0.0),
    yaw(0.0),

    imagePackets(0),    // We must initialize to 0, otherwise extended data packets maybe incorrectly thought to be images

    blockHomePositionChanges(false),
    receivedMode(false),

    // Note variances calculated from flight case from this log: http://dash.oznet.ch/view/MRjW8NUNYQSuSZkbn8dEjY
    // TODO: calibrate stand-still pixhawk variances
    xacc_var(0.6457f),
    yacc_var(0.7048f),
    zacc_var(0.97885f),
    rollspeed_var(0.8126f),
    pitchspeed_var(0.6145f),
    yawspeed_var(0.5852f),
    xmag_var(0.2393f),
    ymag_var(0.2283f),
    zmag_var(0.1665f),
    abs_pressure_var(0.5802f),
    diff_pressure_var(0.5802f),
    pressure_alt_var(0.5802f),
    temperature_var(0.7145f),
    /*
    xacc_var(0.0f),
    yacc_var(0.0f),
    zacc_var(0.0f),
    rollspeed_var(0.0f),
    pitchspeed_var(0.0f),
    yawspeed_var(0.0f),
    xmag_var(0.0f),
    ymag_var(0.0f),
    zmag_var(0.0f),
    abs_pressure_var(0.0f),
    diff_pressure_var(0.0f),
    pressure_alt_var(0.0f),
    temperature_var(0.0f),
    */

#ifndef __mobile__
    simulation(0),
#endif

    // The protected members.
    connectionLost(false),
    lastVoltageWarning(0),
    lastNonNullTime(0),
    onboardTimeOffsetInvalidCount(0),
    hilEnabled(false),
    sensorHil(false),
    lastSendTimeGPS(0),
    lastSendTimeSensors(0),
    lastSendTimeOpticalFlow(0),
    _vehicle(vehicle),
    _firmwarePluginManager(firmwarePluginManager)
{

    for (unsigned int i = 0; i<255;++i)
    {
        componentID[i] = -1;
        componentMulti[i] = false;
    }

#ifndef __mobile__
    connect(_vehicle, &Vehicle::mavlinkMessageReceived, &fileManager, &FileManager::receiveMessage);
#endif

    color = UASInterface::getNextColor();
}

/**
* @ return the id of the uas
*/
int UAS::getUASID() const
{
    return uasId;
}

void UAS::receiveMessage(mavlink_message_t message)
{
    if (!components.contains(message.compid))
    {
        QString componentName;

        switch (message.compid)
        {
        case MAV_COMP_ID_ALL:
        {
            componentName = "ANONYMOUS";
            break;
        }
        case MAV_COMP_ID_IMU:
        {
            componentName = "IMU #1";
            break;
        }
        case MAV_COMP_ID_CAMERA:
        {
            componentName = "CAMERA";
            break;
        }
        case MAV_COMP_ID_MISSIONPLANNER:
        {
            componentName = "MISSIONPLANNER";
            break;
        }
        }

        components.insert(message.compid, componentName);
    }

    //    qDebug() << "UAS RECEIVED from" << message.sysid << "component" << message.compid << "msg id" << message.msgid << "seq no" << message.seq;

    // Only accept messages from this system (condition 1)
    // and only then if a) attitudeStamped is disabled OR b) attitudeStamped is enabled
    // and we already got one attitude packet
    if (message.sysid == uasId && (!attitudeStamped || (attitudeStamped && (lastAttitude != 0)) || message.msgid == MAVLINK_MSG_ID_ATTITUDE))
    {
        QString uasState;
        QString stateDescription;

        bool multiComponentSourceDetected = false;
        bool wrongComponent = false;

        switch (message.compid)
        {
        case MAV_COMP_ID_IMU_2:
            // Prefer IMU 2 over IMU 1 (FIXME)
            componentID[message.msgid] = MAV_COMP_ID_IMU_2;
            break;
        default:
            // Do nothing
            break;
        }

        // Store component ID
        if (componentID[message.msgid] == -1)
        {
            // Prefer the first component
            componentID[message.msgid] = message.compid;
        }
        else
        {
            // Got this message already
            if (componentID[message.msgid] != message.compid)
            {
                componentMulti[message.msgid] = true;
                wrongComponent = true;
            }
        }

        if (componentMulti[message.msgid] == true) multiComponentSourceDetected = true;


        switch (message.msgid)
        {
        case MAVLINK_MSG_ID_HEARTBEAT:
        {
            if (multiComponentSourceDetected && wrongComponent)
            {
                break;
            }
            mavlink_heartbeat_t state;
            mavlink_msg_heartbeat_decode(&message, &state);

            // Send the base_mode and system_status values to the plotter. This uses the ground time
            // so the Ground Time checkbox must be ticked for these values to display
            quint64 time = getUnixTime();
            QString name = QString("M%1:HEARTBEAT.%2").arg(message.sysid);
            emit valueChanged(uasId, name.arg("base_mode"), "bits", state.base_mode, time);
            emit valueChanged(uasId, name.arg("custom_mode"), "bits", state.custom_mode, time);
            emit valueChanged(uasId, name.arg("system_status"), "-", state.system_status, time);

            if ((state.system_status != this->status) && state.system_status != MAV_STATE_UNINIT)
            {
                this->status = state.system_status;
                getStatusForCode((int)state.system_status, uasState, stateDescription);
                emit statusChanged(this, uasState, stateDescription);
                emit statusChanged(this->status);
            }

            // We got the mode
            receivedMode = true;
        }

            break;

        case MAVLINK_MSG_ID_SYS_STATUS:
        {
            if (multiComponentSourceDetected && wrongComponent)
            {
                break;
            }
            mavlink_sys_status_t state;
            mavlink_msg_sys_status_decode(&message, &state);

            // Prepare for sending data to the realtime plotter, which is every field excluding onboard_control_sensors_present.
            quint64 time = getUnixTime();
            QString name = QString("M%1:SYS_STATUS.%2").arg(message.sysid);
            emit valueChanged(uasId, name.arg("sensors_enabled"), "bits", state.onboard_control_sensors_enabled, time);
            emit valueChanged(uasId, name.arg("sensors_health"), "bits", state.onboard_control_sensors_health, time);
            emit valueChanged(uasId, name.arg("errors_comm"), "-", state.errors_comm, time);
            emit valueChanged(uasId, name.arg("errors_count1"), "-", state.errors_count1, time);
            emit valueChanged(uasId, name.arg("errors_count2"), "-", state.errors_count2, time);
            emit valueChanged(uasId, name.arg("errors_count3"), "-", state.errors_count3, time);
            emit valueChanged(uasId, name.arg("errors_count4"), "-", state.errors_count4, time);

            // Process CPU load.
            emit loadChanged(this,state.load/10.0f);
            emit valueChanged(uasId, name.arg("load"), "%", state.load/10.0f, time);

            // control_sensors_enabled:
            // relevant bits: 11: attitude stabilization, 12: yaw position, 13: z/altitude control, 14: x/y position control
            emit attitudeControlEnabled(state.onboard_control_sensors_enabled & (1 << 11));
            emit positionYawControlEnabled(state.onboard_control_sensors_enabled & (1 << 12));
            emit positionZControlEnabled(state.onboard_control_sensors_enabled & (1 << 13));
            emit positionXYControlEnabled(state.onboard_control_sensors_enabled & (1 << 14));

            // Trigger drop rate updates as needed. Here we convert the incoming
            // drop_rate_comm value from 1/100 of a percent in a uint16 to a true
            // percentage as a float. We also cap the incoming value at 100% as defined
            // by the MAVLink specifications.
            if (state.drop_rate_comm > 10000)
            {
                state.drop_rate_comm = 10000;
            }
            emit dropRateChanged(this->getUASID(), state.drop_rate_comm/100.0f);
            emit valueChanged(uasId, name.arg("drop_rate_comm"), "%", state.drop_rate_comm/100.0f, time);
        }
            break;
        case MAVLINK_MSG_ID_ATTITUDE:
        {
            mavlink_attitude_t attitude;
            mavlink_msg_attitude_decode(&message, &attitude);
            quint64 time = getUnixReferenceTime(attitude.time_boot_ms);

            emit attitudeChanged(this, message.compid, QGC::limitAngleToPMPIf(attitude.roll), QGC::limitAngleToPMPIf(attitude.pitch), QGC::limitAngleToPMPIf(attitude.yaw), time);

            if (!wrongComponent)
            {
                lastAttitude = time;
                setRoll(QGC::limitAngleToPMPIf(attitude.roll));
                setPitch(QGC::limitAngleToPMPIf(attitude.pitch));
                setYaw(QGC::limitAngleToPMPIf(attitude.yaw));

                attitudeKnown = true;
                emit attitudeChanged(this, getRoll(), getPitch(), getYaw(), time);
                emit attitudeRotationRatesChanged(uasId, attitude.rollspeed, attitude.pitchspeed, attitude.yawspeed, time);
            }
        }
            break;
        case MAVLINK_MSG_ID_ATTITUDE_QUATERNION:
        {
            mavlink_attitude_quaternion_t attitude;
            mavlink_msg_attitude_quaternion_decode(&message, &attitude);
            quint64 time = getUnixReferenceTime(attitude.time_boot_ms);

            double a = attitude.q1;
            double b = attitude.q2;
            double c = attitude.q3;
            double d = attitude.q4;

            double aSq = a * a;
            double bSq = b * b;
            double cSq = c * c;
            double dSq = d * d;
            float dcm[3][3];
            dcm[0][0] = aSq + bSq - cSq - dSq;
            dcm[0][1] = 2.0 * (b * c - a * d);
            dcm[0][2] = 2.0 * (a * c + b * d);
            dcm[1][0] = 2.0 * (b * c + a * d);
            dcm[1][1] = aSq - bSq + cSq - dSq;
            dcm[1][2] = 2.0 * (c * d - a * b);
            dcm[2][0] = 2.0 * (b * d - a * c);
            dcm[2][1] = 2.0 * (a * b + c * d);
            dcm[2][2] = aSq - bSq - cSq + dSq;

            float phi, theta, psi;
            theta = asin(-dcm[2][0]);

            if (fabs(theta - M_PI_2) < 1.0e-3f) {
                phi = 0.0f;
                psi = (atan2(dcm[1][2] - dcm[0][1],
                        dcm[0][2] + dcm[1][1]) + phi);

            } else if (fabs(theta + M_PI_2) < 1.0e-3f) {
                phi = 0.0f;
                psi = atan2f(dcm[1][2] - dcm[0][1],
                          dcm[0][2] + dcm[1][1] - phi);

            } else {
                phi = atan2f(dcm[2][1], dcm[2][2]);
                psi = atan2f(dcm[1][0], dcm[0][0]);
            }

            emit attitudeChanged(this, message.compid, QGC::limitAngleToPMPIf(phi),
                                 QGC::limitAngleToPMPIf(theta),
                                 QGC::limitAngleToPMPIf(psi), time);

            if (!wrongComponent)
            {
                lastAttitude = time;
                setRoll(QGC::limitAngleToPMPIf(phi));
                setPitch(QGC::limitAngleToPMPIf(theta));
                setYaw(QGC::limitAngleToPMPIf(psi));

                attitudeKnown = true;
                emit attitudeChanged(this, getRoll(), getPitch(), getYaw(), time);
                emit attitudeRotationRatesChanged(uasId, attitude.rollspeed, attitude.pitchspeed, attitude.yawspeed, time);
            }
        }
            break;
        case MAVLINK_MSG_ID_HIL_CONTROLS:
        {
            mavlink_hil_controls_t hil;
            mavlink_msg_hil_controls_decode(&message, &hil);
            emit hilControlsChanged(hil.time_usec, hil.roll_ailerons, hil.pitch_elevator, hil.yaw_rudder, hil.throttle, hil.mode, hil.nav_mode);
        }
            break;
        case MAVLINK_MSG_ID_VFR_HUD:
        {
            mavlink_vfr_hud_t hud;
            mavlink_msg_vfr_hud_decode(&message, &hud);
            quint64 time = getUnixTime();
            // Display updated values
            emit thrustChanged(this, hud.throttle/100.0);

            if (!attitudeKnown)
            {
                setYaw(QGC::limitAngleToPMPId((((double)hud.heading)/180.0)*M_PI));
                emit attitudeChanged(this, getRoll(), getPitch(), getYaw(), time);
            }

            setAltitudeAMSL(hud.alt);
            setGroundSpeed(hud.groundspeed);
            if (!qIsNaN(hud.airspeed))
                setAirSpeed(hud.airspeed);
            speedZ = -hud.climb;
            emit altitudeChanged(this, altitudeAMSL, altitudeRelative, -speedZ, time);
            emit speedChanged(this, groundSpeed, airSpeed, time);
        }
            break;
        case MAVLINK_MSG_ID_LOCAL_POSITION_NED:
            //std::cerr << std::endl;
            //std::cerr << "Decoded attitude message:" << " roll: " << std::dec << mavlink_msg_attitude_get_roll(message.payload) << " pitch: " << mavlink_msg_attitude_get_pitch(message.payload) << " yaw: " << mavlink_msg_attitude_get_yaw(message.payload) << std::endl;
        {
            mavlink_local_position_ned_t pos;
            mavlink_msg_local_position_ned_decode(&message, &pos);
            quint64 time = getUnixTime(pos.time_boot_ms);

            if (!wrongComponent)
            {
                speedX = pos.vx;
                speedY = pos.vy;
                speedZ = pos.vz;

                // Emit
                emit velocityChanged_NED(this, speedX, speedY, speedZ, time);
            }
        }
            break;
        case MAVLINK_MSG_ID_GLOBAL_VISION_POSITION_ESTIMATE:
        {
            mavlink_global_vision_position_estimate_t pos;
            mavlink_msg_global_vision_position_estimate_decode(&message, &pos);
            quint64 time = getUnixTime(pos.usec);
            emit attitudeChanged(this, message.compid, pos.roll, pos.pitch, pos.yaw, time);
        }
            break;
        case MAVLINK_MSG_ID_GLOBAL_POSITION_INT:
            //std::cerr << std::endl;
            //std::cerr << "Decoded attitude message:" << " roll: " << std::dec << mavlink_msg_attitude_get_roll(message.payload) << " pitch: " << mavlink_msg_attitude_get_pitch(message.payload) << " yaw: " << mavlink_msg_attitude_get_yaw(message.payload) << std::endl;
        {
            mavlink_global_position_int_t pos;
            mavlink_msg_global_position_int_decode(&message, &pos);

            quint64 time = getUnixTime();

            setLatitude(pos.lat/(double)1E7);
            setLongitude(pos.lon/(double)1E7);
            setAltitudeRelative(pos.relative_alt/1000.0);

            globalEstimatorActive = true;

            speedX = pos.vx/100.0;
            speedY = pos.vy/100.0;
            speedZ = pos.vz/100.0;

            emit globalPositionChanged(this, getLatitude(), getLongitude(), getAltitudeAMSL(), time);
            emit altitudeChanged(this, altitudeAMSL, altitudeRelative, -speedZ, time);
            // We had some frame mess here, global and local axes were mixed.
            emit velocityChanged_NED(this, speedX, speedY, speedZ, time);

            setGroundSpeed(qSqrt(speedX*speedX+speedY*speedY));
            emit speedChanged(this, groundSpeed, airSpeed, time);

            isGlobalPositionKnown = true;
        }
            break;
        case MAVLINK_MSG_ID_GPS_RAW_INT:
        {
            mavlink_gps_raw_int_t pos;
            mavlink_msg_gps_raw_int_decode(&message, &pos);

            quint64 time = getUnixTime(pos.time_usec);

            // TODO: track localization state not only for gps but also for other loc. sources
            int loc_type = pos.fix_type;
            if (loc_type == 1)
            {
                loc_type = 0;
            }
            setSatelliteCount(pos.satellites_visible);

            if (pos.fix_type > 2)
            {
                isGlobalPositionKnown = true;

                latitude_gps  = pos.lat/(double)1E7;
                longitude_gps = pos.lon/(double)1E7;
                altitude_gps  = pos.alt/1000.0;

                // If no GLOBAL_POSITION_INT messages ever received, use these raw GPS values instead.
                if (!globalEstimatorActive) {
                    setLatitude(latitude_gps);
                    setLongitude(longitude_gps);
                    emit globalPositionChanged(this, getLatitude(), getLongitude(), getAltitudeAMSL(), time);
                    emit altitudeChanged(this, altitudeAMSL, altitudeRelative, -speedZ, time);

                    float vel = pos.vel/100.0f;
                    // Smaller than threshold and not NaN
                    if ((vel < 1000000) && !qIsNaN(vel) && !qIsInf(vel)) {
                        setGroundSpeed(vel);
                        emit speedChanged(this, groundSpeed, airSpeed, time);
                    } else {
                        emit textMessageReceived(uasId, message.compid, MAV_SEVERITY_NOTICE, QString("GCS ERROR: RECEIVED INVALID SPEED OF %1 m/s").arg(vel));
                    }
                }
            }

            double dtmp;
            //-- Raw GPS data
            dtmp = pos.eph == 0xFFFF ? 1e10f : pos.eph / 100.0;
            if(dtmp != satRawHDOP)
            {
                satRawHDOP = dtmp;
                emit satRawHDOPChanged(satRawHDOP);
            }
            dtmp = pos.epv == 0xFFFF ? 1e10f : pos.epv / 100.0;
            if(dtmp != satRawVDOP)
            {
                satRawVDOP = dtmp;
                emit satRawVDOPChanged(satRawVDOP);
            }
            dtmp = pos.cog == 0xFFFF ? 0.0 : pos.cog / 100.0;
            if(dtmp != satRawCOG)
            {
                satRawCOG = dtmp;
                emit satRawCOGChanged(satRawCOG);
            }

            // Emit this signal after the above signals. This way a trigger on gps lock signal which then asks for vehicle position
            // gets a good position.
            emit localizationChanged(this, loc_type);
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
            setSatelliteCount(pos.satellites_visible);
        }
            break;
        case MAVLINK_MSG_ID_GPS_GLOBAL_ORIGIN:
        {
            mavlink_gps_global_origin_t pos;
            mavlink_msg_gps_global_origin_decode(&message, &pos);
            emit homePositionChanged(uasId, pos.latitude / 10000000.0, pos.longitude / 10000000.0, pos.altitude / 1000.0);
        }
            break;

        case MAVLINK_MSG_ID_PARAM_VALUE:
        {
            mavlink_param_value_t rawValue;
            mavlink_msg_param_value_decode(&message, &rawValue);
            QByteArray bytes(rawValue.param_id, MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN);
            // Construct a string stopping at the first NUL (0) character, else copy the whole
            // byte array (max MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN, so safe)
            QString parameterName(bytes);
            mavlink_param_union_t paramVal;
            paramVal.param_float = rawValue.param_value;
            paramVal.type = rawValue.param_type;

            processParamValueMsg(message, parameterName,rawValue,paramVal);
         }
            break;
        case MAVLINK_MSG_ID_ATTITUDE_TARGET:
        {
            mavlink_attitude_target_t out;
            mavlink_msg_attitude_target_decode(&message, &out);
            float roll, pitch, yaw;
            mavlink_quaternion_to_euler(out.q, &roll, &pitch, &yaw);
            quint64 time = getUnixTimeFromMs(out.time_boot_ms);
            emit attitudeThrustSetPointChanged(this, roll, pitch, yaw, out.thrust, time);

            // For plotting emit roll sp, pitch sp and yaw sp values
            emit valueChanged(uasId, "roll sp", "rad", roll, time);
            emit valueChanged(uasId, "pitch sp", "rad", pitch, time);
            emit valueChanged(uasId, "yaw sp", "rad", yaw, time);
        }
            break;

        case MAVLINK_MSG_ID_POSITION_TARGET_LOCAL_NED:
        {
            if (multiComponentSourceDetected && wrongComponent)
            {
                break;
            }
            mavlink_position_target_local_ned_t p;
            mavlink_msg_position_target_local_ned_decode(&message, &p);
            quint64 time = getUnixTimeFromMs(p.time_boot_ms);
            emit positionSetPointsChanged(uasId, p.x, p.y, p.z, 0/* XXX remove yaw and move it to attitude */, time);
        }
            break;
        case MAVLINK_MSG_ID_SET_POSITION_TARGET_LOCAL_NED:
        {
            mavlink_set_position_target_local_ned_t p;
            mavlink_msg_set_position_target_local_ned_decode(&message, &p);
            emit userPositionSetPointsChanged(uasId, p.x, p.y, p.z, 0/* XXX remove yaw and move it to attitude */);
        }
            break;
        case MAVLINK_MSG_ID_STATUSTEXT:
        {
            QByteArray b;
            b.resize(MAVLINK_MSG_STATUSTEXT_FIELD_TEXT_LEN+1);
            mavlink_msg_statustext_get_text(&message, b.data());

            // Ensure NUL-termination
            b[b.length()-1] = '\0';
            QString text = QString(b);
            int severity = mavlink_msg_statustext_get_severity(&message);

        // If the message is NOTIFY or higher severity, or starts with a '#',
        // then read it aloud.
            if (text.startsWith("#") || severity <= MAV_SEVERITY_NOTICE)
            {
                text.remove("#");
                emit textMessageReceived(uasId, message.compid, severity, text);
                _say(text.toLower(), severity);
            }
            else
            {
                emit textMessageReceived(uasId, message.compid, severity, text);
            }
        }
            break;

        case MAVLINK_MSG_ID_DATA_TRANSMISSION_HANDSHAKE:
        {
            mavlink_data_transmission_handshake_t p;
            mavlink_msg_data_transmission_handshake_decode(&message, &p);
            imageSize = p.size;
            imagePackets = p.packets;
            imagePayload = p.payload;
            imageQuality = p.jpg_quality;
            imageType = p.type;
            imageWidth = p.width;
            imageHeight = p.height;
            imageStart = QGC::groundTimeMilliseconds();
            imagePacketsArrived = 0;

        }
            break;

        case MAVLINK_MSG_ID_ENCAPSULATED_DATA:
        {
            mavlink_encapsulated_data_t img;
            mavlink_msg_encapsulated_data_decode(&message, &img);
            int seq = img.seqnr;
            int pos = seq * imagePayload;

            // Check if we have a valid transaction
            if (imagePackets == 0)
            {
                // NO VALID TRANSACTION - ABORT
                // Restart statemachine
                imagePacketsArrived = 0;
                break;
            }

            for (int i = 0; i < imagePayload; ++i)
            {
                if (pos <= imageSize) {
                    imageRecBuffer[pos] = img.data[i];
                }
                ++pos;
            }

            ++imagePacketsArrived;

            // emit signal if all packets arrived
            if (imagePacketsArrived >= imagePackets)
            {
                // Restart statemachine
                imagePackets = 0;
                imagePacketsArrived = 0;
                emit imageReady(this);
            }
        }
            break;

        case MAVLINK_MSG_ID_NAV_CONTROLLER_OUTPUT:
        {
            mavlink_nav_controller_output_t p;
            mavlink_msg_nav_controller_output_decode(&message,&p);
            setDistToWaypoint(p.wp_dist);
            setBearingToWaypoint(p.nav_bearing);
            emit navigationControllerErrorsChanged(this, p.alt_error, p.aspd_error, p.xtrack_error);
            emit NavigationControllerDataChanged(this, p.nav_roll, p.nav_pitch, p.nav_bearing, p.target_bearing, p.wp_dist);
        }
            break;

        case MAVLINK_MSG_ID_LOG_ENTRY:
        {
            mavlink_log_entry_t log;
            mavlink_msg_log_entry_decode(&message, &log);
            emit logEntry(this, log.time_utc, log.size, log.id, log.num_logs, log.last_log_num);
        }
            break;

        case MAVLINK_MSG_ID_LOG_DATA:
        {
            mavlink_log_data_t log;
            mavlink_msg_log_data_decode(&message, &log);
            emit logData(this, log.ofs, log.id, log.count, log.data);
        }
            break;

        default:
            break;
        }
    }
}

void UAS::startCalibration(UASInterface::StartCalibrationType calType)
{
    if (!_vehicle) {
        return;
    }

    int gyroCal = 0;
    int magCal = 0;
    int airspeedCal = 0;
    int radioCal = 0;
    int accelCal = 0;
    int escCal = 0;

    switch (calType) {
        case StartCalibrationGyro:
            gyroCal = 1;
            break;
        case StartCalibrationMag:
            magCal = 1;
            break;
        case StartCalibrationAirspeed:
            airspeedCal = 1;
            break;
        case StartCalibrationRadio:
            radioCal = 1;
            break;
        case StartCalibrationCopyTrims:
            radioCal = 2;
            break;
        case StartCalibrationAccel:
            accelCal = 1;
            break;
        case StartCalibrationLevel:
            accelCal = 2;
            break;
        case StartCalibrationEsc:
            escCal = 1;
            break;
        case StartCalibrationUavcanEsc:
            escCal = 2;
            break;
    }

    mavlink_message_t msg;
    mavlink_msg_command_long_pack(mavlink->getSystemId(),
                                  mavlink->getComponentId(),
                                  &msg,
                                  uasId,
                                  _vehicle->defaultComponentId(),   // target component
                                  MAV_CMD_PREFLIGHT_CALIBRATION,    // command id
                                  0,                                // 0=first transmission of command
                                  gyroCal,                          // gyro cal
                                  magCal,                           // mag cal
                                  0,                                // ground pressure
                                  radioCal,                         // radio cal
                                  accelCal,                         // accel cal
                                  airspeedCal,                      // airspeed cal
                                  escCal);                          // esc cal
    _vehicle->sendMessageOnPriorityLink(msg);
}

void UAS::stopCalibration(void)
{
    if (!_vehicle) {
        return;
    }

    mavlink_message_t msg;
    mavlink_msg_command_long_pack(mavlink->getSystemId(),
                                  mavlink->getComponentId(),
                                  &msg,
                                  uasId,
                                  _vehicle->defaultComponentId(),   // target component
                                  MAV_CMD_PREFLIGHT_CALIBRATION,    // command id
                                  0,                                // 0=first transmission of command
                                  0,                                // gyro cal
                                  0,                                // mag cal
                                  0,                                // ground pressure
                                  0,                                // radio cal
                                  0,                                // accel cal
                                  0,                                // airspeed cal
                                  0);                               // unused
    _vehicle->sendMessageOnPriorityLink(msg);
}

void UAS::startBusConfig(UASInterface::StartBusConfigType calType)
{
    if (!_vehicle) {
        return;
    }

   int actuatorCal = 0;

    switch (calType) {
        case StartBusConfigActuators:
            actuatorCal = 1;
        break;
        case EndBusConfigActuators:
            actuatorCal = 0;
        break;
    }

    mavlink_message_t msg;
    mavlink_msg_command_long_pack(mavlink->getSystemId(),
                                  mavlink->getComponentId(),
                                  &msg,
                                  uasId,
                                  _vehicle->defaultComponentId(),   // target component
                                  MAV_CMD_PREFLIGHT_UAVCAN,         // command id
                                  0,                                // 0=first transmission of command
                                  actuatorCal,                      // actuators
                                  0,
                                  0,
                                  0,
                                  0,
                                  0,
                                  0);
    _vehicle->sendMessageOnPriorityLink(msg);
}

void UAS::stopBusConfig(void)
{
    if (!_vehicle) {
        return;
    }

    mavlink_message_t msg;
    mavlink_msg_command_long_pack(mavlink->getSystemId(),
                                  mavlink->getComponentId(),
                                  &msg,
                                  uasId,
                                  _vehicle->defaultComponentId(),   // target component
                                  MAV_CMD_PREFLIGHT_UAVCAN,         // command id
                                  0,                                // 0=first transmission of command
                                  0,
                                  0,
                                  0,
                                  0,
                                  0,
                                  0,
                                  0);
    _vehicle->sendMessageOnPriorityLink(msg);
}

/**
* Check if time is smaller than 40 years, assuming no system without Unix
* timestamp runs longer than 40 years continuously without reboot. In worst case
* this will add/subtract the communication delay between GCS and MAV, it will
* never alter the timestamp in a safety critical way.
*/
quint64 UAS::getUnixReferenceTime(quint64 time)
{
    // Same as getUnixTime, but does not react to attitudeStamped mode
    if (time == 0)
    {
        //        qDebug() << "XNEW time:" <<QGC::groundTimeMilliseconds();
        return QGC::groundTimeMilliseconds();
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
        //        qDebug() << "GEN time:" << time/1000 + onboardTimeOffset;
        if (onboardTimeOffset == 0)
        {
            onboardTimeOffset = QGC::groundTimeMilliseconds() - time/1000;
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

/**
* @warning If attitudeStamped is enabled, this function will not actually return
* the precise time stamp of this measurement augmented to UNIX time, but will
* MOVE the timestamp IN TIME to match the last measured attitude. There is no
* reason why one would want this, except for system setups where the onboard
* clock is not present or broken and datasets should be collected that are still
* roughly synchronized. PLEASE NOTE THAT ENABLING ATTITUDE STAMPED RUINS THE
* SCIENTIFIC NATURE OF THE CORRECT LOGGING FUNCTIONS OF QGROUNDCONTROL!
*/
quint64 UAS::getUnixTimeFromMs(quint64 time)
{
    return getUnixTime(time*1000);
}

/**
* @warning If attitudeStamped is enabled, this function will not actually return
* the precise time stam of this measurement augmented to UNIX time, but will
* MOVE the timestamp IN TIME to match the last measured attitude. There is no
* reason why one would want this, except for system setups where the onboard
* clock is not present or broken and datasets should be collected that are
* still roughly synchronized. PLEASE NOTE THAT ENABLING ATTITUDE STAMPED
* RUINS THE SCIENTIFIC NATURE OF THE CORRECT LOGGING FUNCTIONS OF QGROUNDCONTROL!
*/
quint64 UAS::getUnixTime(quint64 time)
{
    quint64 ret = 0;
    if (attitudeStamped)
    {
        ret = lastAttitude;
    }

    if (time == 0)
    {
        ret = QGC::groundTimeMilliseconds();
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
        //        qDebug() << "GEN time:" << time/1000 + onboardTimeOffset;
        if (onboardTimeOffset == 0 || time < (lastNonNullTime - 100))
        {
            lastNonNullTime = time;
            onboardTimeOffset = QGC::groundTimeMilliseconds() - time/1000;
        }
        if (time > lastNonNullTime) lastNonNullTime = time;

        ret = time/1000 + onboardTimeOffset;
    }
    else
    {
        // Time is not zero and larger than 40 years -> has to be
        // a Unix epoch timestamp. Do nothing.
        ret = time/1000;
    }

    return ret;
}

/**
* Get the status of the code and a description of the status.
* Status can be unitialized, booting up, calibrating sensors, active
* standby, cirtical, emergency, shutdown or unknown.
*/
void UAS::getStatusForCode(int statusCode, QString& uasState, QString& stateDescription)
{
    switch (statusCode)
    {
    case MAV_STATE_UNINIT:
        uasState = tr("UNINIT");
        stateDescription = tr("Unitialized, booting up.");
        break;
    case MAV_STATE_BOOT:
        uasState = tr("BOOT");
        stateDescription = tr("Booting system, please wait.");
        break;
    case MAV_STATE_CALIBRATING:
        uasState = tr("CALIBRATING");
        stateDescription = tr("Calibrating sensors, please wait.");
        break;
    case MAV_STATE_ACTIVE:
        uasState = tr("ACTIVE");
        stateDescription = tr("Active, normal operation.");
        break;
    case MAV_STATE_STANDBY:
        uasState = tr("STANDBY");
        stateDescription = tr("Standby mode, ready for launch.");
        break;
    case MAV_STATE_CRITICAL:
        uasState = tr("CRITICAL");
        stateDescription = tr("FAILURE: Continuing operation.");
        break;
    case MAV_STATE_EMERGENCY:
        uasState = tr("EMERGENCY");
        stateDescription = tr("EMERGENCY: Land Immediately!");
        break;
        //case MAV_STATE_HILSIM:
        //uasState = tr("HIL SIM");
        //stateDescription = tr("HIL Simulation, Sensors read from SIM");
        //break;

    case MAV_STATE_POWEROFF:
        uasState = tr("SHUTDOWN");
        stateDescription = tr("Powering off system.");
        break;

    default:
        uasState = tr("UNKNOWN");
        stateDescription = tr("Unknown system state");
        break;
    }
}

QImage UAS::getImage()
{

//    qDebug() << "IMAGE TYPE:" << imageType;

    // RAW greyscale
    if (imageType == MAVLINK_DATA_STREAM_IMG_RAW8U)
    {
        int imgColors = 255;

        // Construct PGM header
        QString header("P5\n%1 %2\n%3\n");
        header = header.arg(imageWidth).arg(imageHeight).arg(imgColors);

        QByteArray tmpImage(header.toStdString().c_str(), header.length());
        tmpImage.append(imageRecBuffer);

        //qDebug() << "IMAGE SIZE:" << tmpImage.size() << "HEADER SIZE: (15):" << header.size() << "HEADER: " << header;

        if (imageRecBuffer.isNull())
        {
            qDebug()<< "could not convertToPGM()";
            return QImage();
        }

        if (!image.loadFromData(tmpImage, "PGM"))
        {
            qDebug()<< __FILE__ << __LINE__ << "could not create extracted image";
            return QImage();
        }

    }
    // BMP with header
    else if (imageType == MAVLINK_DATA_STREAM_IMG_BMP ||
             imageType == MAVLINK_DATA_STREAM_IMG_JPEG ||
             imageType == MAVLINK_DATA_STREAM_IMG_PGM ||
             imageType == MAVLINK_DATA_STREAM_IMG_PNG)
    {
        if (!image.loadFromData(imageRecBuffer))
        {
            qDebug() << __FILE__ << __LINE__ << "Loading data from image buffer failed!";
            return QImage();
        }
    }

    // Restart statemachine
    imagePacketsArrived = 0;
    imagePackets = 0;
    imageRecBuffer.clear();
    return image;
}

void UAS::requestImage()
{
    if (!_vehicle) {
        return;
    }

   qDebug() << "trying to get an image from the uas...";

    // check if there is already an image transmission going on
    if (imagePacketsArrived == 0)
    {
        mavlink_message_t msg;
        mavlink_msg_data_transmission_handshake_pack(mavlink->getSystemId(), mavlink->getComponentId(), &msg, MAVLINK_DATA_STREAM_IMG_JPEG, 0, 0, 0, 0, 0, 50);
        _vehicle->sendMessage(msg);
    }
}


/* MANAGEMENT */

/**
 *
 * @return The uptime in milliseconds
 *
 */
quint64 UAS::getUptime() const
{
    if(startTime == 0)
    {
        return 0;
    }
    else
    {
        return QGC::groundTimeMilliseconds() - startTime;
    }
}

//TODO update this to use the parameter manager / param data model instead
void UAS::processParamValueMsg(mavlink_message_t& msg, const QString& paramName, const mavlink_param_value_t& rawValue,  mavlink_param_union_t& paramUnion)
{
    int compId = msg.compid;

    QVariant paramValue;

    // Insert with correct type

    switch (rawValue.param_type) {
        case MAV_PARAM_TYPE_REAL32:
            paramValue = QVariant(paramUnion.param_float);
            break;

        case MAV_PARAM_TYPE_UINT8:
            paramValue = QVariant(paramUnion.param_uint8);
            break;

        case MAV_PARAM_TYPE_INT8:
            paramValue = QVariant(paramUnion.param_int8);
            break;

        case MAV_PARAM_TYPE_UINT16:
            paramValue = QVariant(paramUnion.param_uint16);
            break;

        case MAV_PARAM_TYPE_INT16:
            paramValue = QVariant(paramUnion.param_int16);
            break;

        case MAV_PARAM_TYPE_UINT32:
            paramValue = QVariant(paramUnion.param_uint32);
            break;

        case MAV_PARAM_TYPE_INT32:
            paramValue = QVariant(paramUnion.param_int32);
            break;

        //-- Note: These are not handled above:
        //
        //   MAV_PARAM_TYPE_UINT64
        //   MAV_PARAM_TYPE_INT64
        //   MAV_PARAM_TYPE_REAL64
        //
        //   No space in message (the only storage allocation is a "float") and not present in mavlink_param_union_t

        default:
            qCritical() << "INVALID DATA TYPE USED AS PARAMETER VALUE: " << rawValue.param_type;
    }

    qCDebug(UASLog) << "Received PARAM_VALUE" << paramName << paramValue << rawValue.param_type;

    emit parameterUpdate(uasId, compId, paramName, rawValue.param_count, rawValue.param_index, rawValue.param_type, paramValue);
}

void UAS::executeCommand(MAV_CMD command, int confirmation, float param1, float param2, float param3, float param4, float param5, float param6, float param7, int component)
{
    if (!_vehicle) {
        return;
    }

    mavlink_message_t msg;
    mavlink_command_long_t cmd;
    cmd.command = (uint16_t)command;
    cmd.confirmation = confirmation;
    cmd.param1 = param1;
    cmd.param2 = param2;
    cmd.param3 = param3;
    cmd.param4 = param4;
    cmd.param5 = param5;
    cmd.param6 = param6;
    cmd.param7 = param7;
    cmd.target_system = uasId;
    cmd.target_component = component;
    mavlink_msg_command_long_encode(mavlink->getSystemId(), mavlink->getComponentId(), &msg, &cmd);
    _vehicle->sendMessage(msg);
}

/**
* Set the manual control commands.
* This can only be done if the system has manual inputs enabled and is armed.
*/
void UAS::setExternalControlSetpoint(float roll, float pitch, float yaw, float thrust, quint16 buttons, int joystickMode)
{
    if (!_vehicle) {
        return;
    }

    // Store the previous manual commands
    static float manualRollAngle = 0.0;
    static float manualPitchAngle = 0.0;
    static float manualYawAngle = 0.0;
    static float manualThrust = 0.0;
    static quint16 manualButtons = 0;
    static quint8 countSinceLastTransmission = 0; // Track how many calls to this function have occurred since the last MAVLink transmission

    // Transmit the external setpoints only if they've changed OR if it's been a little bit since they were last transmit. To make sure there aren't issues with
    // response rate, we make sure that a message is transmit when the commands have changed, then one more time, and then switch to the lower transmission rate
    // if no command inputs have changed.

    // The default transmission rate is 25Hz, but when no inputs have changed it drops down to 5Hz.
    bool sendCommand = false;
    if (countSinceLastTransmission++ >= 5) {
        sendCommand = true;
        countSinceLastTransmission = 0;
    } else if ((!qIsNaN(roll) && roll != manualRollAngle) || (!qIsNaN(pitch) && pitch != manualPitchAngle) ||
             (!qIsNaN(yaw) && yaw != manualYawAngle) || (!qIsNaN(thrust) && thrust != manualThrust) ||
             buttons != manualButtons) {
        sendCommand = true;

        // Ensure that another message will be sent the next time this function is called
        countSinceLastTransmission = 10;
    }

    // Now if we should trigger an update, let's do that
    if (sendCommand) {
        // Save the new manual control inputs
        manualRollAngle = roll;
        manualPitchAngle = pitch;
        manualYawAngle = yaw;
        manualThrust = thrust;
        manualButtons = buttons;

        mavlink_message_t message;

        if (joystickMode == Vehicle::JoystickModeAttitude) {
            // send an external attitude setpoint command (rate control disabled)
            float attitudeQuaternion[4];
            mavlink_euler_to_quaternion(roll, pitch, yaw, attitudeQuaternion);
            uint8_t typeMask = 0x7; // disable rate control
            mavlink_msg_set_attitude_target_pack(mavlink->getSystemId(),
                mavlink->getComponentId(),
                &message,
                QGC::groundTimeUsecs(),
                this->uasId,
                0,
                typeMask,
                attitudeQuaternion,
                0,
                0,
                0,
                thrust
                );
        } else if (joystickMode == Vehicle::JoystickModePosition) {
            // Send the the local position setpoint (local pos sp external message)
            static float px = 0;
            static float py = 0;
            static float pz = 0;
            //XXX: find decent scaling
            px -= pitch;
            py += roll;
            pz -= 2.0f*(thrust-0.5);
            uint16_t typeMask = (1<<11)|(7<<6)|(7<<3); // select only POSITION control
            mavlink_msg_set_position_target_local_ned_pack(mavlink->getSystemId(),
                    mavlink->getComponentId(),
                    &message,
                    QGC::groundTimeUsecs(),
                    this->uasId,
                    0,
                    MAV_FRAME_LOCAL_NED,
                    typeMask,
                    px,
                    py,
                    pz,
                    0,
                    0,
                    0,
                    0,
                    0,
                    0,
                    yaw,
                    0
                    );
        } else if (joystickMode == Vehicle::JoystickModeForce) {
            // Send the the force setpoint (local pos sp external message)
            float dcm[3][3];
            mavlink_euler_to_dcm(roll, pitch, yaw, dcm);
            const float fx = -dcm[0][2] * thrust;
            const float fy = -dcm[1][2] * thrust;
            const float fz = -dcm[2][2] * thrust;
            uint16_t typeMask = (3<<10)|(7<<3)|(7<<0)|(1<<9); // select only FORCE control (disable everything else)
            mavlink_msg_set_position_target_local_ned_pack(mavlink->getSystemId(),
                    mavlink->getComponentId(),
                    &message,
                    QGC::groundTimeUsecs(),
                    this->uasId,
                    0,
                    MAV_FRAME_LOCAL_NED,
                    typeMask,
                    0,
                    0,
                    0,
                    0,
                    0,
                    0,
                    fx,
                    fy,
                    fz,
                    0,
                    0
                    );
        } else if (joystickMode == Vehicle::JoystickModeVelocity) {
            // Send the the local velocity setpoint (local pos sp external message)
            static float vx = 0;
            static float vy = 0;
            static float vz = 0;
            static float yawrate = 0;
            //XXX: find decent scaling
            vx -= pitch;
            vy += roll;
            vz -= 2.0f*(thrust-0.5);
            yawrate += yaw; //XXX: not sure what scale to apply here
            uint16_t typeMask = (1<<10)|(7<<6)|(7<<0); // select only VELOCITY control
            mavlink_msg_set_position_target_local_ned_pack(mavlink->getSystemId(),
                    mavlink->getComponentId(),
                    &message,
                    QGC::groundTimeUsecs(),
                    this->uasId,
                    0,
                    MAV_FRAME_LOCAL_NED,
                    typeMask,
                    0,
                    0,
                    0,
                    vx,
                    vy,
                    vz,
                    0,
                    0,
                    0,
                    0,
                    yawrate
                    );
        } else if (joystickMode == Vehicle::JoystickModeRC) {

            // Save the new manual control inputs
            manualRollAngle = roll;
            manualPitchAngle = pitch;
            manualYawAngle = yaw;
            manualThrust = thrust;
            manualButtons = buttons;

            // Store scaling values for all 3 axes
            const float axesScaling = 1.0 * 1000.0;

            // Calculate the new commands for roll, pitch, yaw, and thrust
            const float newRollCommand = roll * axesScaling;
            // negate pitch value because pitch is negative for pitching forward but mavlink message argument is positive for forward
            const float newPitchCommand = -pitch * axesScaling;
            const float newYawCommand = yaw * axesScaling;
            const float newThrustCommand = thrust * axesScaling;

            //qDebug() << newRollCommand << newPitchCommand << newYawCommand << newThrustCommand;

            // Send the MANUAL_COMMAND message
            mavlink_msg_manual_control_pack(mavlink->getSystemId(), mavlink->getComponentId(), &message, this->uasId, newPitchCommand, newRollCommand, newThrustCommand, newYawCommand, buttons);
        }

        _vehicle->sendMessage(message);
        // Emit an update in control values to other UI elements, like the HSI display
        emit attitudeThrustSetPointChanged(this, roll, pitch, yaw, thrust, QGC::groundTimeMilliseconds());
    }
}

#ifndef __mobile__
void UAS::setManual6DOFControlCommands(double x, double y, double z, double roll, double pitch, double yaw)
{
    if (!_vehicle) {
        return;
    }
    const uint8_t base_mode = _vehicle->baseMode();

   // If system has manual inputs enabled and is armed
    if(((base_mode & MAV_MODE_FLAG_DECODE_POSITION_MANUAL) && (base_mode & MAV_MODE_FLAG_DECODE_POSITION_SAFETY)) || (base_mode & MAV_MODE_FLAG_HIL_ENABLED))
    {
        mavlink_message_t message;
        float q[4];
        mavlink_euler_to_quaternion(roll, pitch, yaw, q);

        float yawrate = 0.0f;

        // Do not control rates and throttle
        quint8 mask = (1 << 0) | (1 << 1) | (1 << 2); // ignore rates
        mask |= (1 << 6); // ignore throttle
        mavlink_msg_set_attitude_target_pack(mavlink->getSystemId(), mavlink->getComponentId(),
                                             &message, QGC::groundTimeMilliseconds(), this->uasId, 0,
                                             mask, q, 0, 0, 0, 0);
        _vehicle->sendMessage(message);
        quint16 position_mask = (1 << 3) | (1 << 4) | (1 << 5) |
            (1 << 6) | (1 << 7) | (1 << 8);
        mavlink_msg_set_position_target_local_ned_pack(mavlink->getSystemId(), mavlink->getComponentId(),
                                                       &message, QGC::groundTimeMilliseconds(), this->uasId, 0,
                                                       MAV_FRAME_LOCAL_NED, position_mask, x, y, z, 0, 0, 0, 0, 0, 0, yaw, yawrate);
        _vehicle->sendMessage(message);
        qDebug() << __FILE__ << __LINE__ << ": SENT 6DOF CONTROL MESSAGES: x" << x << " y: " << y << " z: " << z << " roll: " << roll << " pitch: " << pitch << " yaw: " << yaw;

        //emit attitudeThrustSetPointChanged(this, roll, pitch, yaw, thrust, QGC::groundTimeMilliseconds());
    }
    else
    {
        qDebug() << "3DMOUSE/MANUAL CONTROL: IGNORING COMMANDS: Set mode to MANUAL to send 3DMouse commands first";
    }
}
#endif

/**
* Order the robot to start receiver pairing
*/
void UAS::pairRX(int rxType, int rxSubType)
{
    if (!_vehicle) {
        return;
    }

    mavlink_message_t msg;

    mavlink_msg_command_long_pack(mavlink->getSystemId(), mavlink->getComponentId(), &msg, uasId, MAV_COMP_ID_ALL, MAV_CMD_START_RX_PAIR, 0, rxType, rxSubType, 0, 0, 0, 0, 0);
    _vehicle->sendMessage(msg);
}

/**
* If enabled, connect the flight gear link.
*/
#ifndef __mobile__
void UAS::enableHilFlightGear(bool enable, QString options, bool sensorHil, QObject * configuration)
{
    Q_UNUSED(configuration);

    QGCFlightGearLink* link = dynamic_cast<QGCFlightGearLink*>(simulation);
    if (!link) {
        // Delete wrong sim
        if (simulation) {
            stopHil();
            delete simulation;
        }
        simulation = new QGCFlightGearLink(_vehicle, options);
    }

    float noise_scaler = 0.0001f;
    xacc_var = noise_scaler * 0.2914f;
    yacc_var = noise_scaler * 0.2914f;
    zacc_var = noise_scaler * 0.9577f;
    rollspeed_var = noise_scaler * 0.8126f;
    pitchspeed_var = noise_scaler * 0.6145f;
    yawspeed_var = noise_scaler * 0.5852f;
    xmag_var = noise_scaler * 0.0786f;
    ymag_var = noise_scaler * 0.0566f;
    zmag_var = noise_scaler * 0.0333f;
    abs_pressure_var = noise_scaler * 0.5604f;
    diff_pressure_var = noise_scaler * 0.2604f;
    pressure_alt_var = noise_scaler * 0.5604f;
    temperature_var = noise_scaler * 0.7290f;

    // Connect Flight Gear Link
    link = dynamic_cast<QGCFlightGearLink*>(simulation);
    link->setStartupArguments(options);
    link->sensorHilEnabled(sensorHil);
    // FIXME: this signal is not on the base hil configuration widget, only on the FG widget
    //QObject::connect(configuration, SIGNAL(barometerOffsetChanged(float)), link, SLOT(setBarometerOffset(float)));
    if (enable)
    {
        startHil();
    }
    else
    {
        stopHil();
    }
}
#endif

/**
* If enabled, connect the JSBSim link.
*/
#ifndef __mobile__
void UAS::enableHilJSBSim(bool enable, QString options)
{
    QGCJSBSimLink* link = dynamic_cast<QGCJSBSimLink*>(simulation);
    if (!link) {
        // Delete wrong sim
        if (simulation) {
            stopHil();
            delete simulation;
        }
        simulation = new QGCJSBSimLink(_vehicle, options);
    }
    // Connect Flight Gear Link
    link = dynamic_cast<QGCJSBSimLink*>(simulation);
    link->setStartupArguments(options);
    if (enable)
    {
        startHil();
    }
    else
    {
        stopHil();
    }
}
#endif

/**
* If enabled, connect the X-plane gear link.
*/
#ifndef __mobile__
void UAS::enableHilXPlane(bool enable)
{
    QGCXPlaneLink* link = dynamic_cast<QGCXPlaneLink*>(simulation);
    if (!link) {
        if (simulation) {
            stopHil();
            delete simulation;
        }
        simulation = new QGCXPlaneLink(_vehicle);

        float noise_scaler = 0.0001f;
        xacc_var = noise_scaler * 0.2914f;
        yacc_var = noise_scaler * 0.2914f;
        zacc_var = noise_scaler * 0.9577f;
        rollspeed_var = noise_scaler * 0.8126f;
        pitchspeed_var = noise_scaler * 0.6145f;
        yawspeed_var = noise_scaler * 0.5852f;
        xmag_var = noise_scaler * 0.0786f;
        ymag_var = noise_scaler * 0.0566f;
        zmag_var = noise_scaler * 0.0333f;
        abs_pressure_var = noise_scaler * 0.5604f;
        diff_pressure_var = noise_scaler * 0.2604f;
        pressure_alt_var = noise_scaler * 0.5604f;
        temperature_var = noise_scaler * 0.7290f;
    }
    // Connect X-Plane Link
    if (enable)
    {
        startHil();
    }
    else
    {
        stopHil();
    }
}
#endif

/**
* @param time_us Timestamp (microseconds since UNIX epoch or microseconds since system boot)
* @param roll Roll angle (rad)
* @param pitch Pitch angle (rad)
* @param yaw Yaw angle (rad)
* @param rollspeed Roll angular speed (rad/s)
* @param pitchspeed Pitch angular speed (rad/s)
* @param yawspeed Yaw angular speed (rad/s)
* @param lat Latitude, expressed as * 1E7
* @param lon Longitude, expressed as * 1E7
* @param alt Altitude in meters, expressed as * 1000 (millimeters)
* @param vx Ground X Speed (Latitude), expressed as m/s * 100
* @param vy Ground Y Speed (Longitude), expressed as m/s * 100
* @param vz Ground Z Speed (Altitude), expressed as m/s * 100
* @param xacc X acceleration (mg)
* @param yacc Y acceleration (mg)
* @param zacc Z acceleration (mg)
*/
#ifndef __mobile__
void UAS::sendHilGroundTruth(quint64 time_us, float roll, float pitch, float yaw, float rollspeed,
                       float pitchspeed, float yawspeed, double lat, double lon, double alt,
                       float vx, float vy, float vz, float ind_airspeed, float true_airspeed, float xacc, float yacc, float zacc)
{
    Q_UNUSED(time_us);
    Q_UNUSED(xacc);
    Q_UNUSED(yacc);
    Q_UNUSED(zacc);

        // Emit attitude for cross-check
        emit valueChanged(uasId, "roll sim", "rad", roll, getUnixTime());
        emit valueChanged(uasId, "pitch sim", "rad", pitch, getUnixTime());
        emit valueChanged(uasId, "yaw sim", "rad", yaw, getUnixTime());

        emit valueChanged(uasId, "roll rate sim", "rad/s", rollspeed, getUnixTime());
        emit valueChanged(uasId, "pitch rate sim", "rad/s", pitchspeed, getUnixTime());
        emit valueChanged(uasId, "yaw rate sim", "rad/s", yawspeed, getUnixTime());

        emit valueChanged(uasId, "lat sim", "deg", lat*1e7, getUnixTime());
        emit valueChanged(uasId, "lon sim", "deg", lon*1e7, getUnixTime());
        emit valueChanged(uasId, "alt sim", "deg", alt*1e3, getUnixTime());

        emit valueChanged(uasId, "vx sim", "m/s", vx*1e2, getUnixTime());
        emit valueChanged(uasId, "vy sim", "m/s", vy*1e2, getUnixTime());
        emit valueChanged(uasId, "vz sim", "m/s", vz*1e2, getUnixTime());

        emit valueChanged(uasId, "IAS sim", "m/s", ind_airspeed, getUnixTime());
        emit valueChanged(uasId, "TAS sim", "m/s", true_airspeed, getUnixTime());
}
#endif

/**
* @param time_us Timestamp (microseconds since UNIX epoch or microseconds since system boot)
* @param roll Roll angle (rad)
* @param pitch Pitch angle (rad)
* @param yaw Yaw angle (rad)
* @param rollspeed Roll angular speed (rad/s)
* @param pitchspeed Pitch angular speed (rad/s)
* @param yawspeed Yaw angular speed (rad/s)
* @param lat Latitude, expressed as * 1E7
* @param lon Longitude, expressed as * 1E7
* @param alt Altitude in meters, expressed as * 1000 (millimeters)
* @param vx Ground X Speed (Latitude), expressed as m/s * 100
* @param vy Ground Y Speed (Longitude), expressed as m/s * 100
* @param vz Ground Z Speed (Altitude), expressed as m/s * 100
* @param xacc X acceleration (mg)
* @param yacc Y acceleration (mg)
* @param zacc Z acceleration (mg)
*/
#ifndef __mobile__
void UAS::sendHilState(quint64 time_us, float roll, float pitch, float yaw, float rollspeed,
                       float pitchspeed, float yawspeed, double lat, double lon, double alt,
                       float vx, float vy, float vz, float ind_airspeed, float true_airspeed, float xacc, float yacc, float zacc)
{
    if (!_vehicle) {
        return;
    }

    if (_vehicle->hilMode())
    {
        float q[4];

        double cosPhi_2 = cos(double(roll) / 2.0);
        double sinPhi_2 = sin(double(roll) / 2.0);
        double cosTheta_2 = cos(double(pitch) / 2.0);
        double sinTheta_2 = sin(double(pitch) / 2.0);
        double cosPsi_2 = cos(double(yaw) / 2.0);
        double sinPsi_2 = sin(double(yaw) / 2.0);
        q[0] = (cosPhi_2 * cosTheta_2 * cosPsi_2 +
                sinPhi_2 * sinTheta_2 * sinPsi_2);
        q[1] = (sinPhi_2 * cosTheta_2 * cosPsi_2 -
                cosPhi_2 * sinTheta_2 * sinPsi_2);
        q[2] = (cosPhi_2 * sinTheta_2 * cosPsi_2 +
                sinPhi_2 * cosTheta_2 * sinPsi_2);
        q[3] = (cosPhi_2 * cosTheta_2 * sinPsi_2 -
                sinPhi_2 * sinTheta_2 * cosPsi_2);

        mavlink_message_t msg;
        mavlink_msg_hil_state_quaternion_pack(mavlink->getSystemId(), mavlink->getComponentId(), &msg,
                                   time_us, q, rollspeed, pitchspeed, yawspeed,
                                   lat*1e7f, lon*1e7f, alt*1000, vx*100, vy*100, vz*100, ind_airspeed*100, true_airspeed*100, xacc*1000/9.81, yacc*1000/9.81, zacc*1000/9.81);
        _vehicle->sendMessage(msg);
    }
    else
    {
        // Attempt to set HIL mode
        _vehicle->setHilMode(true);
        qDebug() << __FILE__ << __LINE__ << "HIL is onboard not enabled, trying to enable.";
    }
}
#endif

#ifndef __mobile__
float UAS::addZeroMeanNoise(float truth_meas, float noise_var)
{
    /* Calculate normally distributed variable noise with mean = 0 and variance = noise_var.  Calculated according to
    Box-Muller transform */
    static const float epsilon = std::numeric_limits<float>::min(); //used to ensure non-zero uniform numbers
    static float z0; //calculated normal distribution random variables with mu = 0, var = 1;
    float u1, u2;        //random variables generated from c++ rand();

    /*Generate random variables in range (0 1] */
    do
    {
        //TODO seed rand() with srand(time) but srand(time should be called once on startup)
        //currently this will generate repeatable random noise
        u1 = rand() * (1.0 / RAND_MAX);
        u2 = rand() * (1.0 / RAND_MAX);
    }
    while ( u1 <= epsilon );  //Have a catch to ensure non-zero for log()

    z0 = sqrt(-2.0 * log(u1)) * cos(2.0f * M_PI * u2); //calculate normally distributed variable with mu = 0, var = 1

    //TODO add bias term that changes randomly to simulate accelerometer and gyro bias the exf should handle these
    //as well
    float noise = z0 * sqrt(noise_var); //calculate normally distributed variable with mu = 0, std = var^2

    //Finally gaurd against any case where the noise is not real
    if(std::isfinite(noise)) {
            return truth_meas + noise;
    } else {
        return truth_meas;
    }
}
#endif

/*
* @param abs_pressure Absolute Pressure (hPa)
* @param diff_pressure Differential Pressure  (hPa)
*/
#ifndef __mobile__
void UAS::sendHilSensors(quint64 time_us, float xacc, float yacc, float zacc, float rollspeed, float pitchspeed, float yawspeed,
                                    float xmag, float ymag, float zmag, float abs_pressure, float diff_pressure, float pressure_alt, float temperature, quint32 fields_changed)
{
    if (!_vehicle) {
        return;
    }

    if (_vehicle->hilMode())
    {
        float xacc_corrupt = addZeroMeanNoise(xacc, xacc_var);
        float yacc_corrupt = addZeroMeanNoise(yacc, yacc_var);
        float zacc_corrupt = addZeroMeanNoise(zacc, zacc_var);
        float rollspeed_corrupt = addZeroMeanNoise(rollspeed,rollspeed_var);
        float pitchspeed_corrupt = addZeroMeanNoise(pitchspeed,pitchspeed_var);
        float yawspeed_corrupt = addZeroMeanNoise(yawspeed,yawspeed_var);
        float xmag_corrupt = addZeroMeanNoise(xmag, xmag_var);
        float ymag_corrupt = addZeroMeanNoise(ymag, ymag_var);
        float zmag_corrupt = addZeroMeanNoise(zmag, zmag_var);
        float abs_pressure_corrupt = addZeroMeanNoise(abs_pressure,abs_pressure_var);
        float diff_pressure_corrupt = addZeroMeanNoise(diff_pressure, diff_pressure_var);
        float pressure_alt_corrupt = addZeroMeanNoise(pressure_alt, pressure_alt_var);
        float temperature_corrupt = addZeroMeanNoise(temperature,temperature_var);

        mavlink_message_t msg;
        mavlink_msg_hil_sensor_pack(mavlink->getSystemId(), mavlink->getComponentId(), &msg,
                                   time_us, xacc_corrupt, yacc_corrupt, zacc_corrupt, rollspeed_corrupt, pitchspeed_corrupt,
                                    yawspeed_corrupt, xmag_corrupt, ymag_corrupt, zmag_corrupt, abs_pressure_corrupt,
                                    diff_pressure_corrupt, pressure_alt_corrupt, temperature_corrupt, fields_changed);
        _vehicle->sendMessage(msg);
        lastSendTimeSensors = QGC::groundTimeMilliseconds();
    }
    else
    {
        // Attempt to set HIL mode
        _vehicle->setHilMode(true);
        qDebug() << __FILE__ << __LINE__ << "HIL is onboard not enabled, trying to enable.";
    }
}
#endif

#ifndef __mobile__
void UAS::sendHilOpticalFlow(quint64 time_us, qint16 flow_x, qint16 flow_y, float flow_comp_m_x,
                    float flow_comp_m_y, quint8 quality, float ground_distance)
{
    if (!_vehicle) {
        return;
    }

    // FIXME: This needs to be updated for new mavlink_msg_hil_optical_flow_pack api

    Q_UNUSED(time_us);
    Q_UNUSED(flow_x);
    Q_UNUSED(flow_y);
    Q_UNUSED(flow_comp_m_x);
    Q_UNUSED(flow_comp_m_y);
    Q_UNUSED(quality);
    Q_UNUSED(ground_distance);

    if (_vehicle->hilMode())
    {
#if 0
        mavlink_message_t msg;
        mavlink_msg_hil_optical_flow_pack(mavlink->getSystemId(), mavlink->getComponentId(), &msg,
                                   time_us, 0, 0 /* hack */, flow_x, flow_y, 0.0f /* hack */, 0.0f /* hack */, 0.0f /* hack */, 0 /* hack */, quality, ground_distance);

        _vehicle->sendMessage(msg);
        lastSendTimeOpticalFlow = QGC::groundTimeMilliseconds();
#endif
    }
    else
    {
        // Attempt to set HIL mode
        _vehicle->setHilMode(true);
        qDebug() << __FILE__ << __LINE__ << "HIL is onboard not enabled, trying to enable.";
    }

}
#endif

#ifndef __mobile__
void UAS::sendHilGps(quint64 time_us, double lat, double lon, double alt, int fix_type, float eph, float epv, float vel, float vn, float ve, float vd, float cog, int satellites)
{
    if (!_vehicle) {
        return;
    }

    // Only send at 10 Hz max rate
    if (QGC::groundTimeMilliseconds() - lastSendTimeGPS < 100)
        return;

    if (_vehicle->hilMode())
    {
        float course = cog;
        // map to 0..2pi
        if (course < 0)
            course += 2.0f * static_cast<float>(M_PI);
        // scale from radians to degrees
        course = (course / M_PI) * 180.0f;

        mavlink_message_t msg;
        mavlink_msg_hil_gps_pack(mavlink->getSystemId(), mavlink->getComponentId(), &msg,
                                   time_us, fix_type, lat*1e7, lon*1e7, alt*1e3, eph*1e2, epv*1e2, vel*1e2, vn*1e2, ve*1e2, vd*1e2, course*1e2, satellites);
        lastSendTimeGPS = QGC::groundTimeMilliseconds();
        _vehicle->sendMessage(msg);
    }
    else
    {
        // Attempt to set HIL mode
        _vehicle->setHilMode(true);
        qDebug() << __FILE__ << __LINE__ << "HIL is onboard not enabled, trying to enable.";
    }
}
#endif

/**
* Connect flight gear link.
**/
#ifndef __mobile__
void UAS::startHil()
{
    if (hilEnabled) return;
    hilEnabled = true;
    sensorHil = false;
    _vehicle->setHilMode(true);
    qDebug() << __FILE__ << __LINE__ << "HIL is onboard not enabled, trying to enable.";
    // Connect HIL simulation link
    simulation->connectSimulation();
}
#endif

/**
* disable flight gear link.
*/
#ifndef __mobile__
void UAS::stopHil()
{
   if (simulation && simulation->isConnected()) {
       simulation->disconnectSimulation();
       _vehicle->setHilMode(false);
       qDebug() << __FILE__ << __LINE__ << "HIL is onboard not enabled, trying to disable.";
   }
    hilEnabled = false;
    sensorHil = false;
}
#endif

/**
* @rerturn the map of the components
*/
QMap<int, QString> UAS::getComponents()
{
    return components;
}

void UAS::sendMapRCToParam(QString param_id, float scale, float value0, quint8 param_rc_channel_index, float valueMin, float valueMax)
{
    if (!_vehicle) {
        return;
    }

    mavlink_message_t message;

    char param_id_cstr[MAVLINK_MSG_PARAM_MAP_RC_FIELD_PARAM_ID_LEN] = {};
    // Copy string into buffer, ensuring not to exceed the buffer size
    for (unsigned int i = 0; i < sizeof(param_id_cstr); i++)
    {
        if ((int)i < param_id.length())
        {
            param_id_cstr[i] = param_id.toLatin1()[i];
        }
    }

    mavlink_msg_param_map_rc_pack(mavlink->getSystemId(),
                                  mavlink->getComponentId(),
                                  &message,
                                  this->uasId,
                                  0,
                                  param_id_cstr,
                                  -1,
                                  param_rc_channel_index,
                                  value0,
                                  scale,
                                  valueMin,
                                  valueMax);
    _vehicle->sendMessage(message);
    //qDebug() << "Mavlink message sent";
}

void UAS::unsetRCToParameterMap()
{
    if (!_vehicle) {
        return;
    }

    char param_id_cstr[MAVLINK_MSG_PARAM_MAP_RC_FIELD_PARAM_ID_LEN] = {};

    for (int i = 0; i < 3; i++) {
        mavlink_message_t message;
        mavlink_msg_param_map_rc_pack(mavlink->getSystemId(),
                                      mavlink->getComponentId(),
                                      &message,
                                      this->uasId,
                                      0,
                                      param_id_cstr,
                                      -2,
                                      i,
                                      0.0f,
                                      0.0f,
                                      0.0f,
                                      0.0f);
        _vehicle->sendMessage(message);
    }
}

void UAS::_say(const QString& text, int severity)
{
    Q_UNUSED(severity);
    qgcApp()->toolbox()->audioOutput()->say(text);
}

void UAS::shutdownVehicle(void)
{
#ifndef __mobile__
    stopHil();
    if (simulation) {
        // wait for the simulator to exit
        simulation->wait();
        simulation->disconnectSimulation();
        simulation->deleteLater();
    }
#endif
    _vehicle = NULL;
}
