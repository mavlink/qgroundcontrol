#include "QGCMAVLink.h"
#include "MAVLinkDecoder.h"

#include <QDebug>

MAVLinkDecoder::MAVLinkDecoder(MAVLinkProtocol* protocol, QObject *parent) :
    QThread()
{
    Q_UNUSED(parent);
    // We're doing it wrong - because the Qt folks got the API wrong:
    // http://blog.qt.digia.com/blog/2010/06/17/youre-doing-it-wrong/
    moveToThread(this);

    memset(receivedMessages, 0, sizeof(mavlink_message_t)*cMessageIds);
    for (unsigned int i = 0; i<cMessageIds;++i)
    {
        componentID[i] = -1;
        componentMulti[i] = false;
        onboardTimeOffset[i] = 0;
        onboardToGCSUnixTimeOffsetAndDelay[i] = 0;
        firstOnboardTime[i] = 0;
    }

    // Fill filter
    // Allow system status
//    messageFilter.insert(MAVLINK_MSG_ID_HEARTBEAT, false);
//    messageFilter.insert(MAVLINK_MSG_ID_SYS_STATUS, false);
    messageFilter.insert(MAVLINK_MSG_ID_STATUSTEXT, false);
    messageFilter.insert(MAVLINK_MSG_ID_COMMAND_LONG, false);
    messageFilter.insert(MAVLINK_MSG_ID_COMMAND_ACK, false);
    messageFilter.insert(MAVLINK_MSG_ID_PARAM_SET, false);
    messageFilter.insert(MAVLINK_MSG_ID_PARAM_VALUE, false);
    messageFilter.insert(MAVLINK_MSG_ID_MISSION_ITEM, false);
    messageFilter.insert(MAVLINK_MSG_ID_MISSION_COUNT, false);
    messageFilter.insert(MAVLINK_MSG_ID_MISSION_ACK, false);
    messageFilter.insert(MAVLINK_MSG_ID_DATA_STREAM, false);
    messageFilter.insert(MAVLINK_MSG_ID_GPS_STATUS, false);
    messageFilter.insert(MAVLINK_MSG_ID_RC_CHANNELS_RAW, false);
    messageFilter.insert(MAVLINK_MSG_ID_LOG_DATA, false);
    #ifdef MAVLINK_MSG_ID_ENCAPSULATED_DATA
    messageFilter.insert(MAVLINK_MSG_ID_ENCAPSULATED_DATA, false);
    #endif
    #ifdef MAVLINK_MSG_ID_DATA_TRANSMISSION_HANDSHAKE
    messageFilter.insert(MAVLINK_MSG_ID_DATA_TRANSMISSION_HANDSHAKE, false);
    #endif
    messageFilter.insert(MAVLINK_MSG_ID_FILE_TRANSFER_PROTOCOL, false);

    textMessageFilter.insert(MAVLINK_MSG_ID_DEBUG, false);
    textMessageFilter.insert(MAVLINK_MSG_ID_DEBUG_VECT, false);
    textMessageFilter.insert(MAVLINK_MSG_ID_NAMED_VALUE_FLOAT, false);
    textMessageFilter.insert(MAVLINK_MSG_ID_NAMED_VALUE_INT, false);
//    textMessageFilter.insert(MAVLINK_MSG_ID_HIGHRES_IMU, false);

    connect(protocol, &MAVLinkProtocol::messageReceived, this, &MAVLinkDecoder::receiveMessage);

    start(LowPriority);
}

/**
 * @brief Runs the thread
 *
 **/
void MAVLinkDecoder::run()
{
    exec();
}

void MAVLinkDecoder::receiveMessage(LinkInterface* link,mavlink_message_t message)
{
    if (message.msgid >= cMessageIds) {
        // XXX No support for messag ids above 512
	// instead of an array, should use a std::map to
	// save memory, can't allocate for 10000 mavlink message
        // which would be required by current internals
        return;
    }

    Q_UNUSED(link);
    memcpy(receivedMessages+message.msgid, &message, sizeof(mavlink_message_t));

    uint32_t msgid = message.msgid;
    const mavlink_message_info_t* msgInfo = mavlink_get_message_info(&message);

    // Store an arrival time for this message. This value ends up being calculated later.
    quint64 time = 0;
    
    // The SYSTEM_TIME message is special, in that it's handled here for synchronizing the QGC time with the remote time.
    if (message.msgid == MAVLINK_MSG_ID_SYSTEM_TIME)
    {
        mavlink_system_time_t timebase;
        mavlink_msg_system_time_decode(&message, &timebase);
        onboardTimeOffset[message.sysid] = (timebase.time_unix_usec+500)/1000 - timebase.time_boot_ms;
        onboardToGCSUnixTimeOffsetAndDelay[message.sysid] = static_cast<qint64>(QGC::groundTimeMilliseconds() - (timebase.time_unix_usec+500)/1000);
    }
    else
    {

        // See if first value is a time value and if it is, use that as the arrival time for this data.
        uint8_t fieldid = 0;
        uint8_t* m = (uint8_t*)&((mavlink_message_t*)(receivedMessages+msgid))->payload64[0];

        if (QString(msgInfo->fields[fieldid].name) == QString("time_boot_ms") && msgInfo->fields[fieldid].type == MAVLINK_TYPE_UINT32_T)
        {
            time = *((quint32*)(m+msgInfo->fields[fieldid].wire_offset));
        }
        else if (QString(msgInfo->fields[fieldid].name).contains("usec") && msgInfo->fields[fieldid].type == MAVLINK_TYPE_UINT64_T)
        {
            time = *((quint64*)(m+msgInfo->fields[fieldid].wire_offset));
            time = (time+500)/1000; // Scale to milliseconds, round up/down correctly
        }
    }

    // Align UAS time to global time
    time = getUnixTimeFromMs(message.sysid, time);

    // Send out all field values for this message
    for (unsigned int i = 0; i < msgInfo->num_fields; ++i)
    {
        emitFieldValue(&message, i, time);
    }

    // Send out combined math expressions
    // FIXME XXX TODO
}

quint64 MAVLinkDecoder::getUnixTimeFromMs(int systemID, quint64 time)
{
    quint64 ret = 0;
    if (time == 0)
    {
        ret = QGC::groundTimeMilliseconds() - onboardToGCSUnixTimeOffsetAndDelay[systemID];
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
    else if (time < 1261440000000LLU)
#else
    else if (time < 1261440000000)
#endif
    {
        if (onboardTimeOffset[systemID] == 0 || time < (firstOnboardTime[systemID]-100))
        {
            firstOnboardTime[systemID] = time;
            onboardTimeOffset[systemID] = QGC::groundTimeMilliseconds() - time;
        }

        if (time > firstOnboardTime[systemID]) firstOnboardTime[systemID] = time;

        ret = time + onboardTimeOffset[systemID];
    }
    else
    {
        // Time is not zero and larger than 40 years -> has to be
        // a Unix epoch timestamp. Do nothing.
        ret = time;
    }


//    // Check if the offset estimation likely went wrong
//    // and we're talking to a new instance / the system
//    // has rebooted. Only reset if this is consistent.
//    if (!isNull && lastNonNullTime > ret)
//    {
//        onboardTimeOffsetInvalidCount++;
//    }
//    else if (!isNull && lastNonNullTime < ret)
//    {
//        onboardTimeOffsetInvalidCount = 0;
//    }

//    // Reset onboard time offset estimation, since it seems to be really off
//    if (onboardTimeOffsetInvalidCount > 20)
//    {
//        onboardTimeOffset = 0;
//        onboardTimeOffsetInvalidCount = 0;
//        lastNonNullTime = 0;
//        qDebug() << "RESETTET ONBOARD TIME OFFSET";
//    }

//    // If we're progressing in time, set it
//    // else wait for the reboot detection to
//    // catch the timestamp wrap / reset
//    if (!isNull && (lastNonNullTime < ret)) {
//        lastNonNullTime = ret;
//    }

    return ret;
}

void MAVLinkDecoder::emitFieldValue(mavlink_message_t* msg, int fieldid, quint64 time)
{
    bool multiComponentSourceDetected = false;
    const mavlink_message_info_t* msgInfo = mavlink_get_message_info(msg);

    // Store component ID
    if (componentID[msg->msgid] == -1)
    {
        componentID[msg->msgid] = msg->compid;
    }
    else
    {
        // Got this message already
        if (componentID[msg->msgid] != msg->compid)
        {
            componentMulti[msg->msgid] = true;
        }
    }

    if (componentMulti[msg->msgid] == true) multiComponentSourceDetected = true;

    // Add field tree widget item
    uint32_t msgid = msg->msgid;
    if (messageFilter.contains(msgid)) return;
    QString fieldName(msgInfo->fields[fieldid].name);
    QString fieldType;
    uint8_t* m = (uint8_t*)&((mavlink_message_t*)(receivedMessages+msgid))->payload64[0];
    QString name("%1.%2");
    QString unit("");

    // Debug vector messages
    if (msgid == MAVLINK_MSG_ID_DEBUG_VECT)
    {
        mavlink_debug_vect_t debug;
        mavlink_msg_debug_vect_decode(msg, &debug);
        char buf[11];
        strncpy(buf, debug.name, 10);
        buf[10] = '\0';
        name = QString("%1.%2").arg(buf).arg(fieldName);
        time = getUnixTimeFromMs(msg->sysid, (debug.time_usec+500)/1000); // Scale to milliseconds, round up/down correctly
    }
    else if (msgid == MAVLINK_MSG_ID_DEBUG)
    {
        mavlink_debug_t debug;
        mavlink_msg_debug_decode(msg, &debug);
        name = name.arg(QString("debug")).arg(debug.ind);
        time = getUnixTimeFromMs(msg->sysid, debug.time_boot_ms);
    }
    else if (msgid == MAVLINK_MSG_ID_NAMED_VALUE_FLOAT)
    {
        mavlink_named_value_float_t debug;
        mavlink_msg_named_value_float_decode(msg, &debug);
        char buf[11];
        strncpy(buf, debug.name, 10);
        buf[10] = '\0';
        name = QString(buf);
        time = getUnixTimeFromMs(msg->sysid, debug.time_boot_ms);
    }
    else if (msgid == MAVLINK_MSG_ID_NAMED_VALUE_INT)
    {
        mavlink_named_value_int_t debug;
        mavlink_msg_named_value_int_decode(msg, &debug);
        char buf[11];
        strncpy(buf, debug.name, 10);
        buf[10] = '\0';
        name = QString(buf);
        time = getUnixTimeFromMs(msg->sysid, debug.time_boot_ms);
    }
    else if (msgid == MAVLINK_MSG_ID_RC_CHANNELS_RAW)
    {
        // XXX this is really ugly, but we do not know a better way to do this
        mavlink_rc_channels_raw_t raw;
        mavlink_msg_rc_channels_raw_decode(msg, &raw);
        name = name.arg(msgInfo->name).arg(fieldName);
        name.prepend(QString("port%1_").arg(raw.port));
    }
    else if (msgid == MAVLINK_MSG_ID_RC_CHANNELS_SCALED)
    {
        // XXX this is really ugly, but we do not know a better way to do this
        mavlink_rc_channels_scaled_t scaled;
        mavlink_msg_rc_channels_scaled_decode(msg, &scaled);
        name = name.arg(msgInfo->name).arg(fieldName);
        name.prepend(QString("port%1_").arg(scaled.port));
    }
    else if (msgid == MAVLINK_MSG_ID_SERVO_OUTPUT_RAW)
    {
        // XXX this is really ugly, but we do not know a better way to do this
        mavlink_servo_output_raw_t servo;
        mavlink_msg_servo_output_raw_decode(msg, &servo);
        name = name.arg(msgInfo->name).arg(fieldName);
        name.prepend(QString("port%1_").arg(servo.port));
    }
    else
    {
        name = name.arg(msgInfo->name).arg(fieldName);
    }

    if (multiComponentSourceDetected)
    {
        name = name.prepend(QString("C%1:").arg(msg->compid));
    }

    name = name.prepend(QString("M%1:").arg(msg->sysid));

    // handle dynamic field messages
    if (msgid == MAVLINK_MSG_ID_ESTIMATOR_STATE
            || msgid == MAVLINK_MSG_ID_ESTIMATOR_STATE_STD
            || msgid == MAVLINK_MSG_ID_ESTIMATOR_INNOV
            || msgid == MAVLINK_MSG_ID_ESTIMATOR_INNOV_STD
            )
    {
        // ignore id and sensor fields
        if (fieldName == "id" || fieldName == "sensor" || fieldName == "n") {
            return;
        }
        if (msgInfo->fields[fieldid].type == MAVLINK_TYPE_FLOAT && msgInfo->fields[fieldid].array_length > 0)
        {
            float* nums = (float*)(m+msgInfo->fields[fieldid].wire_offset);
            uint8_t* id_array = (uint8_t*)(m+msgInfo->fields[3].wire_offset);
            uint8_t* sensor_array = (uint8_t*)(m+msgInfo->fields[4].wire_offset);
            fieldType = QString("float[%1]").arg(msgInfo->fields[fieldid].array_length);
            for (unsigned int j = 0; j < msgInfo->fields[fieldid].array_length; ++j)
            {
                uint8_t id_val = id_array[j];
                QString id = QString("%1").arg(id_val);
                switch (id_val) {
                case MAV_FIELD_UNUSED:
					// use number
                    break;
                case MAV_FIELD_POS_N:
                    id = "pos_N";
                    break;
                case MAV_FIELD_POS_E:
                    id = "pos_E";
                    break;
                case MAV_FIELD_POS_D:
                    id = "pos_D";
                    break;
                case MAV_FIELD_ASL:
                    id = "asl";
                    break;
                case MAV_FIELD_AGL:
                    id = "agl";
                    break;
                case MAV_FIELD_VEL_N:
                    id = "vel_N";
                    break;
                case MAV_FIELD_VEL_E:
                    id = "vel_E";
                    break;
                case MAV_FIELD_VEL_D:
                    id = "vel_D";
                    break;
                case MAV_FIELD_VEL_X:
                    id = "vel_X";
                    break;
                case MAV_FIELD_VEL_Y:
                    id = "vel_Y";
                    break;
                case MAV_FIELD_VEL_Z:
                    id = "vel_Z";
                    break;
                case MAV_FIELD_ACC_N:
                    id = "acc_N";
                    break;
                case MAV_FIELD_ACC_E:
                    id = "acc_E";
                    break;
                case MAV_FIELD_ACC_D:
                    id = "acc_D";
                    break;
                case MAV_FIELD_ACC_X:
                    id = "acc_X";
                    break;
                case MAV_FIELD_ACC_Y:
                    id = "acc_Y";
                    break;
                case MAV_FIELD_ACC_Z:
                    id = "acc_Z";
                    break;
                case MAV_FIELD_Q0:
                    id = "q0";
                    break;
                case MAV_FIELD_Q1:
                    id = "q1";
                    break;
                case MAV_FIELD_Q2:
                    id = "q2";
                    break;
                case MAV_FIELD_Q3:
                    id = "q3";
                    break;
                case MAV_FIELD_ROLL:
                    id = "roll";
                    break;
                case MAV_FIELD_PITCH:
                    id = "pitch";
                    break;
                case MAV_FIELD_YAW:
                    id = "yaw";
                    break;
                case MAV_FIELD_ANGVEL_X:
                    id = "angvel_X";
                    break;
                case MAV_FIELD_ANGVEL_Y:
                    id = "angvel_Y";
                    break;
                case MAV_FIELD_ANGVEL_Z:
                    id = "angvel_Z";
                    break;
                case MAV_FIELD_ROLLRATE:
                    id = "rollrate";
                    break;
                case MAV_FIELD_PITCHRATE:
                    id = "pitchrate";
                    break;
                case MAV_FIELD_YAWRATE:
                    id = "yawrate";
                    break;
                case MAV_FIELD_LAT:
                    id = "lat";
                    break;
                case MAV_FIELD_LON:
                    id = "lon";
                    break;
                case MAV_FIELD_BIAS_N:
                    id = "bias_N";
                    break;
                case MAV_FIELD_BIAS_E:
                    id = "bias_E";
                    break;
                case MAV_FIELD_BIAS_D:
                    id = "bias_D";
                    break;
                case MAV_FIELD_BIAS_X:
                    id = "bias_X";
                    break;
                case MAV_FIELD_BIAS_Y:
                    id = "bias_Y";
                    break;
                case MAV_FIELD_BIAS_Z:
                    id = "bias_Z";
                    break;
                case MAV_FIELD_TERRAIN_ASL:
                    id = "terrain_asl";
                    break;
                case MAV_FIELD_AIRSPEED:
                    id = "airspeed";
                    break;
                case MAV_FIELD_FLOW_X:
                    id = "flow_X";
                    break;
                case MAV_FIELD_FLOW_Y:
                    id = "flow_Y";
                    break;
                case MAV_FIELD_MAG_X:
                    id = "mag_X";
                    break;
                case MAV_FIELD_MAG_Y:
                    id = "mag_Y";
                    break;
                case MAV_FIELD_MAG_Z:
                    id = "mag_Z";
                    break;
                case MAV_FIELD_MAG_HDG:
                    id = "mag_HDG";
                    break;
                case MAV_FIELD_DIST_TOP:
                    id = "dist_top";
                    break;
                case MAV_FIELD_DIST_BOTTOM:
                    id = "dist_bottom";
                    break;
                case MAV_FIELD_DIST_FRONT:
                    id = "dist_front";
                    break;
                case MAV_FIELD_DIST_BACK:
                    id = "dist_back";
                    break;
                case MAV_FIELD_DIST_LEFT:
                    id = "dist_left";
                    break;
                case MAV_FIELD_DIST_RIGHT:
                    id = "dist_right";
                    break;
                }

                uint8_t sensor_val = sensor_array[j];
                QString sensor = QString("%1").arg(sensor_val);
                switch (sensor_val) {
                case MAV_SENSOR_TYPE_NONE:
                    sensor = "";
                    break;
                case MAV_SENSOR_TYPE_GPS:
                    sensor = "gps";
                    break;
                case MAV_SENSOR_TYPE_SONAR:
                    sensor = "sonar";
                    break;
                case MAV_SENSOR_TYPE_LIDAR:
                    sensor = "lidar";
                    break;
                case MAV_SENSOR_TYPE_GYRO:
                    sensor = "gyro";
                    break;
                case MAV_SENSOR_TYPE_ACCEL:
                    sensor = "accel";
                    break;
                case MAV_SENSOR_TYPE_MAG:
                    sensor = "mag";
                    break;
                case MAV_SENSOR_TYPE_BARO:
                    sensor = "baro";
                    break;
                case MAV_SENSOR_TYPE_PITOT:
                    sensor = "pitot";
                    break;
                case MAV_SENSOR_TYPE_MOCAP:
                    sensor = "mocap";
                    break;
                case MAV_SENSOR_TYPE_FLOW:
                    sensor = "flow";
                    break;
                case MAV_SENSOR_TYPE_LAND:
                    sensor = "land";
                    break;
                case MAV_SENSOR_TYPE_VISION:
                    sensor = "vision";
                    break;
                }
                emit valueChanged(msg->sysid, QString("%1-%2-%3").arg(name).arg(sensor).arg(id), fieldType, nums[j], time);
            }
        }
        // we already emit the fields here, return early
        return;
    }

    switch (msgInfo->fields[fieldid].type)
    {
    case MAVLINK_TYPE_CHAR:
        if (msgInfo->fields[fieldid].array_length > 0)
        {
            char* str = (char*)(m+msgInfo->fields[fieldid].wire_offset);
            // Enforce null termination
            str[msgInfo->fields[fieldid].array_length-1] = '\0';
            QString string(name + ": " + str);
            if (!textMessageFilter.contains(msgid)) emit textMessageReceived(msg->sysid, msg->compid, MAV_SEVERITY_INFO, string);
        }
        else
        {
            // Single char
            char b = *((char*)(m+msgInfo->fields[fieldid].wire_offset));
            unit = QString("char[%1]").arg(msgInfo->fields[fieldid].array_length);
            emit valueChanged(msg->sysid, name, unit, b, time);
        }
        break;
    case MAVLINK_TYPE_UINT8_T:
        if (msgInfo->fields[fieldid].array_length > 0)
        {
            uint8_t* nums = m+msgInfo->fields[fieldid].wire_offset;
            fieldType = QString("uint8_t[%1]").arg(msgInfo->fields[fieldid].array_length);
            for (unsigned int j = 0; j < msgInfo->fields[fieldid].array_length; ++j)
            {
                emit valueChanged(msg->sysid, QString("%1.%2").arg(name).arg(j), fieldType, nums[j], time);
            }
        }
        else
        {
            // Single value
            uint8_t u = *(m+msgInfo->fields[fieldid].wire_offset);
            fieldType = "uint8_t";
            emit valueChanged(msg->sysid, name, fieldType, u, time);
        }
        break;
    case MAVLINK_TYPE_INT8_T:
        if (msgInfo->fields[fieldid].array_length > 0)
        {
            int8_t* nums = (int8_t*)(m+msgInfo->fields[fieldid].wire_offset);
            fieldType = QString("int8_t[%1]").arg(msgInfo->fields[fieldid].array_length);
            for (unsigned int j = 0; j < msgInfo->fields[fieldid].array_length; ++j)
            {
                emit valueChanged(msg->sysid, QString("%1.%2").arg(name).arg(j), fieldType, nums[j], time);
            }
        }
        else
        {
            // Single value
            int8_t n = *((int8_t*)(m+msgInfo->fields[fieldid].wire_offset));
            fieldType = "int8_t";
            emit valueChanged(msg->sysid, name, fieldType, n, time);
        }
        break;
    case MAVLINK_TYPE_UINT16_T:
        if (msgInfo->fields[fieldid].array_length > 0)
        {
            uint16_t* nums = (uint16_t*)(m+msgInfo->fields[fieldid].wire_offset);
            fieldType = QString("uint16_t[%1]").arg(msgInfo->fields[fieldid].array_length);
            for (unsigned int j = 0; j < msgInfo->fields[fieldid].array_length; ++j)
            {
                emit valueChanged(msg->sysid, QString("%1.%2").arg(name).arg(j), fieldType, nums[j], time);
            }
        }
        else
        {
            // Single value
            uint16_t n = *((uint16_t*)(m+msgInfo->fields[fieldid].wire_offset));
            fieldType = "uint16_t";
            emit valueChanged(msg->sysid, name, fieldType, n, time);
        }
        break;
    case MAVLINK_TYPE_INT16_T:
        if (msgInfo->fields[fieldid].array_length > 0)
        {
            int16_t* nums = (int16_t*)(m+msgInfo->fields[fieldid].wire_offset);
            fieldType = QString("int16_t[%1]").arg(msgInfo->fields[fieldid].array_length);
            for (unsigned int j = 0; j < msgInfo->fields[fieldid].array_length; ++j)
            {
                emit valueChanged(msg->sysid, QString("%1.%2").arg(name).arg(j), fieldType, nums[j], time);
            }
        }
        else
        {
            // Single value
            int16_t n = *((int16_t*)(m+msgInfo->fields[fieldid].wire_offset));
            fieldType = "int16_t";
            emit valueChanged(msg->sysid, name, fieldType, n, time);
        }
        break;
    case MAVLINK_TYPE_UINT32_T:
        if (msgInfo->fields[fieldid].array_length > 0)
        {
            uint32_t* nums = (uint32_t*)(m+msgInfo->fields[fieldid].wire_offset);
            fieldType = QString("uint32_t[%1]").arg(msgInfo->fields[fieldid].array_length);
            for (unsigned int j = 0; j < msgInfo->fields[fieldid].array_length; ++j)
            {
                emit valueChanged(msg->sysid, QString("%1.%2").arg(name).arg(j), fieldType, nums[j], time);
            }
        }
        else
        {
            // Single value
            uint32_t n = *((uint32_t*)(m+msgInfo->fields[fieldid].wire_offset));
            fieldType = "uint32_t";
            emit valueChanged(msg->sysid, name, fieldType, n, time);
        }
        break;
    case MAVLINK_TYPE_INT32_T:
        if (msgInfo->fields[fieldid].array_length > 0)
        {
            int32_t* nums = (int32_t*)(m+msgInfo->fields[fieldid].wire_offset);
            fieldType = QString("int32_t[%1]").arg(msgInfo->fields[fieldid].array_length);
            for (unsigned int j = 0; j < msgInfo->fields[fieldid].array_length; ++j)
            {
                emit valueChanged(msg->sysid, QString("%1.%2").arg(name).arg(j), fieldType, nums[j], time);
            }
        }
        else
        {
            // Single value
            int32_t n = *((int32_t*)(m+msgInfo->fields[fieldid].wire_offset));
            fieldType = "int32_t";
            emit valueChanged(msg->sysid, name, fieldType, n, time);
        }
        break;
    case MAVLINK_TYPE_FLOAT:
        if (msgInfo->fields[fieldid].array_length > 0)
        {
            float* nums = (float*)(m+msgInfo->fields[fieldid].wire_offset);
            fieldType = QString("float[%1]").arg(msgInfo->fields[fieldid].array_length);
            for (unsigned int j = 0; j < msgInfo->fields[fieldid].array_length; ++j)
            {
                emit valueChanged(msg->sysid, QString("%1.%2").arg(name).arg(j), fieldType, (float)(nums[j]), time);
            }
        }
        else
        {
            // Single value
            float f = *((float*)(m+msgInfo->fields[fieldid].wire_offset));
            fieldType = "float";
            emit valueChanged(msg->sysid, name, fieldType, f, time);
        }
        break;
    case MAVLINK_TYPE_DOUBLE:
        if (msgInfo->fields[fieldid].array_length > 0)
        {
            double* nums = (double*)(m+msgInfo->fields[fieldid].wire_offset);
            fieldType = QString("double[%1]").arg(msgInfo->fields[fieldid].array_length);
            for (unsigned int j = 0; j < msgInfo->fields[fieldid].array_length; ++j)
            {
                emit valueChanged(msg->sysid, QString("%1.%2").arg(name).arg(j), fieldType, nums[j], time);
            }
        }
        else
        {
            // Single value
            double f = *((double*)(m+msgInfo->fields[fieldid].wire_offset));
            fieldType = "double";
            emit valueChanged(msg->sysid, name, fieldType, f, time);
        }
        break;
    case MAVLINK_TYPE_UINT64_T:
        if (msgInfo->fields[fieldid].array_length > 0)
        {
            uint64_t* nums = (uint64_t*)(m+msgInfo->fields[fieldid].wire_offset);
            fieldType = QString("uint64_t[%1]").arg(msgInfo->fields[fieldid].array_length);
            for (unsigned int j = 0; j < msgInfo->fields[fieldid].array_length; ++j)
            {
                emit valueChanged(msg->sysid, QString("%1.%2").arg(name).arg(j), fieldType, (quint64) nums[j], time);
            }
        }
        else
        {
            // Single value
            uint64_t n = *((uint64_t*)(m+msgInfo->fields[fieldid].wire_offset));
            fieldType = "uint64_t";
            emit valueChanged(msg->sysid, name, fieldType, (quint64) n, time);
        }
        break;
    case MAVLINK_TYPE_INT64_T:
        if (msgInfo->fields[fieldid].array_length > 0)
        {
            int64_t* nums = (int64_t*)(m+msgInfo->fields[fieldid].wire_offset);
            fieldType = QString("int64_t[%1]").arg(msgInfo->fields[fieldid].array_length);
            for (unsigned int j = 0; j < msgInfo->fields[fieldid].array_length; ++j)
            {
                emit valueChanged(msg->sysid, QString("%1.%2").arg(name).arg(j), fieldType, (qint64) nums[j], time);
            }
        }
        else
        {
            // Single value
            int64_t n = *((int64_t*)(m+msgInfo->fields[fieldid].wire_offset));
            fieldType = "int64_t";
            emit valueChanged(msg->sysid, name, fieldType, (qint64) n, time);
        }
        break;
    default:
        qDebug() << "WARNING: UNKNOWN MAVLINK TYPE";
    }
}
