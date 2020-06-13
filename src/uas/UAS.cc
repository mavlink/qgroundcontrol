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

    // The protected members.
    connectionLost(false),
    lastVoltageWarning(0),
    lastNonNullTime(0),
    onboardTimeOffsetInvalidCount(0),
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

// Ignore warnings from mavlink headers for both GCC/Clang and MSVC
#ifdef __GNUC__

#if __GNUC__ > 8
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waddress-of-packed-member"
#elif defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Waddress-of-packed-member"
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#endif

#else
#pragma warning(push, 0)
#endif

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

// Pop warnings ignoring for mavlink headers for both GCC/Clang and MSVC
#ifdef __GNUC__
    #if defined(__clang__)
        #pragma clang diagnostic pop
    #else
        #pragma GCC diagnostic pop
    #endif
#else
#pragma warning(pop, 0)
#endif

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
    _vehicle->sendMessageOnLinkThreadSafe(_vehicle->priorityLink(), msg);
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
        _vehicle->sendMessageOnLinkThreadSafe(_vehicle->priorityLink(), msg);
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

void UAS::shutdownVehicle(void)
{
    _vehicle = nullptr;
}
