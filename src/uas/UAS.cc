/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

// NO NEW CODE HERE
// UASInterface, UAS.h/cc are deprecated. All new functionality should go into Vehicle.h/cc
//

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
#include "QGC.h"
#include "MAVLinkProtocol.h"
#include "QGCMAVLink.h"
#include "LinkManager.h"
#ifndef NO_SERIAL_LINK
#include "SerialLink.h"
#endif
#include "FirmwarePluginManager.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"
#include "Joystick.h"
#include "QGCApplication.h"

QGC_LOGGING_CATEGORY(UASLog, "UASLog")

// THIS CLASS IS DEPRECATED. ALL NEW FUNCTIONALITY SHOULD GO INTO Vehicle class
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

#ifndef __mobile__
    fileManager(this, vehicle),
#endif

    attitudeKnown(false),
    attitudeStamped(false),
    lastAttitude(0),

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

#ifndef __mobile__
    connect(_vehicle, &Vehicle::mavlinkMessageReceived, &fileManager, &FileManager::receiveMessage);
#endif

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
    // Only accept messages from this system (condition 1)
    // and only then if a) attitudeStamped is disabled OR b) attitudeStamped is enabled
    // and we already got one attitude packet
    if (message.sysid == uasId && (!attitudeStamped || lastAttitude != 0 || message.msgid == MAVLINK_MSG_ID_ATTITUDE))
    {
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
        if (!componentID.contains(message.msgid))
        {
            // Prefer the first component
            componentID[message.msgid] = message.compid;
            componentMulti[message.msgid] = false;
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

        if (componentMulti[message.msgid] == true) {
            multiComponentSourceDetected = true;
        }


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
            emit valueChanged(uasId, name.arg("load"), "%", state.load/10.0f, time);
            emit valueChanged(uasId, name.arg("drop_rate_comm"), "%", state.drop_rate_comm/100.0f, time);
        }
            break;
        case MAVLINK_MSG_ID_HIL_CONTROLS:
        {
            mavlink_hil_controls_t hil;
            mavlink_msg_hil_controls_decode(&message, &hil);
            emit hilControlsChanged(hil.time_usec, hil.roll_ailerons, hil.pitch_elevator, hil.yaw_rudder, hil.throttle, hil.mode, hil.nav_mode);
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

            // For plotting emit roll sp, pitch sp and yaw sp values
            emit valueChanged(uasId, "roll sp", "rad", roll, time);
            emit valueChanged(uasId, "pitch sp", "rad", pitch, time);
            emit valueChanged(uasId, "yaw sp", "rad", yaw, time);
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
    int pressureCal = 0;
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
    case StartCalibrationPressure:
        pressureCal = 1;
        break;
    case StartCalibrationEsc:
        escCal = 1;
        break;
    case StartCalibrationUavcanEsc:
        escCal = 2;
        break;
    case StartCalibrationCompassMot:
        airspeedCal = 1; // ArduPilot, bit of a hack
        break;
    }

    // We can't use sendMavCommand here since we have no idea how long it will be before the command returns a result. This in turn
    // causes the retry logic to break down.
    mavlink_message_t msg;
    mavlink_msg_command_long_pack_chan(mavlink->getSystemId(),
                                       mavlink->getComponentId(),
                                       _vehicle->priorityLink()->mavlinkChannel(),
                                       &msg,
                                       uasId,
                                       _vehicle->defaultComponentId(),   // target component
                                       MAV_CMD_PREFLIGHT_CALIBRATION,    // command id
                                       0,                                // 0=first transmission of command
                                       gyroCal,                          // gyro cal
                                       magCal,                           // mag cal
                                       pressureCal,                      // ground pressure
                                       radioCal,                         // radio cal
                                       accelCal,                         // accel cal
                                       airspeedCal,                      // PX4: airspeed cal, ArduPilot: compass mot
                                       escCal);                          // esc cal
    _vehicle->sendMessageOnLink(_vehicle->priorityLink(), msg);
}

void UAS::stopCalibration(void)
{
    if (!_vehicle) {
        return;
    }

    _vehicle->sendMavCommand(_vehicle->defaultComponentId(),    // target component
                             MAV_CMD_PREFLIGHT_CALIBRATION,     // command id
                             true,                              // showError
                             0,                                 // gyro cal
                             0,                                 // mag cal
                             0,                                 // ground pressure
                             0,                                 // radio cal
                             0,                                 // accel cal
                             0,                                 // airspeed cal
                             0);                                // unused
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

    _vehicle->sendMavCommand(_vehicle->defaultComponentId(),    // target component
                             MAV_CMD_PREFLIGHT_UAVCAN,          // command id
                             true,                              // showError
                             actuatorCal);                      // actuators
}

void UAS::stopBusConfig(void)
{
    if (!_vehicle) {
        return;
    }

    _vehicle->sendMavCommand(_vehicle->defaultComponentId(),    // target component
                             MAV_CMD_PREFLIGHT_UAVCAN,          // command id
                             true,                              // showError
                             0);                                // cancel
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
        mavlink_msg_data_transmission_handshake_pack_chan(mavlink->getSystemId(),
                                                          mavlink->getComponentId(),
                                                          _vehicle->priorityLink()->mavlinkChannel(),
                                                          &msg,
                                                          MAVLINK_DATA_STREAM_IMG_JPEG,
                                                          0, 0, 0, 0, 0, 50);
        _vehicle->sendMessageOnLink(_vehicle->priorityLink(), msg);
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

/**
* Set the manual control commands.
* This can only be done if the system has manual inputs enabled and is armed.
*/
void UAS::setExternalControlSetpoint(float roll, float pitch, float yaw, float thrust, quint16 buttons, int joystickMode)
{
    if (!_vehicle || !_vehicle->priorityLink()) {
        return;
    }
    mavlink_message_t message;
    if (joystickMode == Vehicle::JoystickModeAttitude) {
        // send an external attitude setpoint command (rate control disabled)
        float attitudeQuaternion[4];
        mavlink_euler_to_quaternion(roll, pitch, yaw, attitudeQuaternion);
        uint8_t typeMask = 0x7; // disable rate control
        mavlink_msg_set_attitude_target_pack_chan(
            mavlink->getSystemId(),
            mavlink->getComponentId(),
            _vehicle->priorityLink()->mavlinkChannel(),
            &message,
            QGC::groundTimeUsecs(),
            this->uasId,
            0,
            typeMask,
            attitudeQuaternion,
            0,
            0,
            0,
            thrust);
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
        mavlink_msg_set_position_target_local_ned_pack_chan(
            mavlink->getSystemId(),
            mavlink->getComponentId(),
            _vehicle->priorityLink()->mavlinkChannel(),
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
            0);
    } else if (joystickMode == Vehicle::JoystickModeForce) {
        // Send the the force setpoint (local pos sp external message)
        float dcm[3][3];
        mavlink_euler_to_dcm(roll, pitch, yaw, dcm);
        const float fx = -dcm[0][2] * thrust;
        const float fy = -dcm[1][2] * thrust;
        const float fz = -dcm[2][2] * thrust;
        uint16_t typeMask = (3<<10)|(7<<3)|(7<<0)|(1<<9); // select only FORCE control (disable everything else)
        mavlink_msg_set_position_target_local_ned_pack_chan(
            mavlink->getSystemId(),
            mavlink->getComponentId(),
            _vehicle->priorityLink()->mavlinkChannel(),
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
            0);
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
        mavlink_msg_set_position_target_local_ned_pack_chan(
            mavlink->getSystemId(),
            mavlink->getComponentId(),
            _vehicle->priorityLink()->mavlinkChannel(),
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
            yawrate);
    } else if (joystickMode == Vehicle::JoystickModeRC) {
        // Store scaling values for all 3 axes
        static const float axesScaling = 1.0 * 1000.0;
        // Calculate the new commands for roll, pitch, yaw, and thrust
        const float newRollCommand = roll * axesScaling;
        // negate pitch value because pitch is negative for pitching forward but mavlink message argument is positive for forward
        const float newPitchCommand  = -pitch * axesScaling;
        const float newYawCommand    = yaw * axesScaling;
        const float newThrustCommand = thrust * axesScaling;
        // Send the MANUAL_COMMAND message
        mavlink_msg_manual_control_pack_chan(
            static_cast<uint8_t>(mavlink->getSystemId()),
            static_cast<uint8_t>(mavlink->getComponentId()),
            _vehicle->priorityLink()->mavlinkChannel(),
            &message,
            static_cast<uint8_t>(this->uasId),
            static_cast<int16_t>(newPitchCommand),
            static_cast<int16_t>(newRollCommand),
            static_cast<int16_t>(newThrustCommand),
            static_cast<int16_t>(newYawCommand),
            buttons);
    }
    _vehicle->sendMessageOnLink(_vehicle->priorityLink(), message);
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
        mavlink_msg_set_attitude_target_pack_chan(mavlink->getSystemId(),
                                                  mavlink->getComponentId(),
                                                  _vehicle->priorityLink()->mavlinkChannel(),
                                                  &message,
                                                  QGC::groundTimeMilliseconds(), this->uasId, _vehicle->defaultComponentId(),
                                                  mask, q, 0, 0, 0, 0);
        _vehicle->sendMessageOnLink(_vehicle->priorityLink(), message);
        quint16 position_mask = (1 << 3) | (1 << 4) | (1 << 5) |
            (1 << 6) | (1 << 7) | (1 << 8);
        mavlink_msg_set_position_target_local_ned_pack_chan(mavlink->getSystemId(), mavlink->getComponentId(),
                                                            _vehicle->priorityLink()->mavlinkChannel(),
                                                            &message, QGC::groundTimeMilliseconds(), this->uasId, _vehicle->defaultComponentId(),
                                                            MAV_FRAME_LOCAL_NED, position_mask, x, y, z, 0, 0, 0, 0, 0, 0, yaw, yawrate);
        _vehicle->sendMessageOnLink(_vehicle->priorityLink(), message);
        qDebug() << __FILE__ << __LINE__ << ": SENT 6DOF CONTROL MESSAGES: x" << x << " y: " << y << " z: " << z << " roll: " << roll << " pitch: " << pitch << " yaw: " << yaw;
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
    if (_vehicle) {
        _vehicle->sendMavCommand(_vehicle->defaultComponentId(),    // target component
                                 MAV_CMD_START_RX_PAIR,             // command id
                                 true,                              // showError
                                 rxType,
                                 rxSubType);
    }
}

/**
* If enabled, connect the JSBSim link.
*/
#ifndef __mobile__
void UAS::enableHilJSBSim(bool enable, QString options)
{
    auto* link = qobject_cast<QGCJSBSimLink*>(simulation);
    if (!link) {
        // Delete wrong sim
        if (simulation) {
            stopHil();
            delete simulation;
        }
        simulation = new QGCJSBSimLink(_vehicle, options);
    }
    // Connect Flight Gear Link
    link = qobject_cast<QGCJSBSimLink*>(simulation);
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
    auto* link = qobject_cast<QGCXPlaneLink*>(simulation);
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
        mavlink_msg_hil_state_quaternion_pack_chan(mavlink->getSystemId(),
                                                   mavlink->getComponentId(),
                                                   _vehicle->priorityLink()->mavlinkChannel(),
                                                   &msg,
                                                   time_us, q, rollspeed, pitchspeed, yawspeed,
                                                   lat*1e7f, lon*1e7f, alt*1000, vx*100, vy*100, vz*100, ind_airspeed*100, true_airspeed*100, xacc*1000/9.81, yacc*1000/9.81, zacc*1000/9.81);
        _vehicle->sendMessageOnLink(_vehicle->priorityLink(), msg);
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

    //Finally guard against any case where the noise is not real
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
        mavlink_msg_hil_sensor_pack_chan(mavlink->getSystemId(),
                                         mavlink->getComponentId(),
                                         _vehicle->priorityLink()->mavlinkChannel(),
                                         &msg,
                                         time_us, xacc_corrupt, yacc_corrupt, zacc_corrupt, rollspeed_corrupt, pitchspeed_corrupt,
                                         yawspeed_corrupt, xmag_corrupt, ymag_corrupt, zmag_corrupt, abs_pressure_corrupt,
                                         diff_pressure_corrupt, pressure_alt_corrupt, temperature_corrupt, fields_changed);
        _vehicle->sendMessageOnLink(_vehicle->priorityLink(), msg);
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
        mavlink_msg_hil_optical_flow_pack_chan(mavlink->getSystemId(),
                                               mavlink->getComponentId(),
                                               _vehicle->priorityLink()->mavlinkChannel(),
                                               &msg,
                                               time_us, 0, 0 /* hack */, flow_x, flow_y, 0.0f /* hack */, 0.0f /* hack */, 0.0f /* hack */, 0 /* hack */, quality, ground_distance);

        _vehicle->sendMessageOnLink(_vehicle->priorityLink(), msg);
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
        mavlink_msg_hil_gps_pack_chan(mavlink->getSystemId(),
                                      mavlink->getComponentId(),
                                      _vehicle->priorityLink()->mavlinkChannel(),
                                      &msg,
                                      time_us, fix_type, lat*1e7, lon*1e7, alt*1e3, eph*1e2, epv*1e2, vel*1e2, vn*1e2, ve*1e2, vd*1e2, course*1e2, satellites);
        lastSendTimeGPS = QGC::groundTimeMilliseconds();
        _vehicle->sendMessageOnLink(_vehicle->priorityLink(), msg);
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

    mavlink_msg_param_map_rc_pack_chan(mavlink->getSystemId(),
                                       mavlink->getComponentId(),
                                       _vehicle->priorityLink()->mavlinkChannel(),
                                       &message,
                                       this->uasId,
                                       _vehicle->defaultComponentId(),
                                       param_id_cstr,
                                       -1,
                                       param_rc_channel_index,
                                       value0,
                                       scale,
                                       valueMin,
                                       valueMax);
    _vehicle->sendMessageOnLink(_vehicle->priorityLink(), message);
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
        mavlink_msg_param_map_rc_pack_chan(mavlink->getSystemId(),
                                           mavlink->getComponentId(),
                                           _vehicle->priorityLink()->mavlinkChannel(),
                                           &message,
                                           this->uasId,
                                           _vehicle->defaultComponentId(),
                                           param_id_cstr,
                                           -2,
                                           i,
                                           0.0f,
                                           0.0f,
                                           0.0f,
                                           0.0f);
        _vehicle->sendMessageOnLink(_vehicle->priorityLink(), message);
    }
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
    _vehicle = nullptr;
}
