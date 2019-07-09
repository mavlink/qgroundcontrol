#include "QGCMAVLink.h"
#include "MAVLinkDecoder.h"

#include <QDebug>

MAVLinkDecoder::MAVLinkDecoder(MAVLinkProtocol* protocol) :
    QThread(), creationThread(QThread::currentThread())
{
    // We're doing it wrong - because the Qt folks got the API wrong:
    // http://blog.qt.digia.com/blog/2010/06/17/youre-doing-it-wrong/
    moveToThread(this);

    // Fill filter
    // Allow system status
//    messageFilter.insert(MAVLINK_MSG_ID_HEARTBEAT, false);
//    messageFilter.insert(MAVLINK_MSG_ID_SYS_STATUS, false);
    messageFilter.insert(MAVLINK_MSG_ID_STATUSTEXT, false);
    messageFilter.insert(MAVLINK_MSG_ID_STATUSTEXT_LONG, false);
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
    textMessageFilter.insert(MAVLINK_MSG_ID_DEBUG_FLOAT_ARRAY, false);
    textMessageFilter.insert(MAVLINK_MSG_ID_NAMED_VALUE_FLOAT, false);
    textMessageFilter.insert(MAVLINK_MSG_ID_NAMED_VALUE_INT, false);
//    textMessageFilter.insert(MAVLINK_MSG_ID_HIGHRES_IMU, false);

    connect(protocol, &MAVLinkProtocol::messageReceived, this, &MAVLinkDecoder::receiveMessage);
    connect(this, &MAVLinkDecoder::finish, this, &QThread::quit);

    start(LowestPriority);
}

/**
 * @brief Runs the thread
 *
 **/
void MAVLinkDecoder::run()
{
    exec();
    moveToThread(creationThread);
}

void MAVLinkDecoder::receiveMessage(LinkInterface* link,mavlink_message_t message)
{
    Q_UNUSED(link);

    uint32_t msgid = message.msgid;
    const mavlink_message_info_t* msgInfo = mavlink_get_message_info(&message);
    if(!msgInfo) {
        qWarning() << "Invalid MAVLink message received. ID:" << msgid;
        return;
    }

    msgDict[message.msgid] = message;

    // Store an arrival time for this message. This value ends up being calculated later.
    quint64 time = 0;

    // The SYSTEM_TIME message is special, in that it's handled here for synchronizing the QGC time with the remote time.
    if (message.msgid == MAVLINK_MSG_ID_SYSTEM_TIME)
    {
        mavlink_system_time_t timebase;
        mavlink_msg_system_time_decode(&message, &timebase);
        sysDict[msgid].onboardTimeOffset = (timebase.time_unix_usec+500)/1000 - timebase.time_boot_ms;
        sysDict[msgid].onboardToGCSUnixTimeOffsetAndDelay  = static_cast<qint64>(QGC::groundTimeMilliseconds() - (timebase.time_unix_usec+500)/1000);
    }
    else
    {

        // See if first value is a time value and if it is, use that as the arrival time for this data.
        uint8_t fieldid = 0;
        uint8_t* m = (uint8_t*)(msgDict[msgid].payload64);

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
        ret = QGC::groundTimeMilliseconds() - sysDict[systemID].onboardToGCSUnixTimeOffsetAndDelay;
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
        if (sysDict[systemID].onboardTimeOffset == 0 || time < (sysDict[systemID].firstOnboardTime -100))
        {
            sysDict[systemID].firstOnboardTime = time;
            sysDict[systemID].onboardTimeOffset = QGC::groundTimeMilliseconds() - time;
        }

        if (time > sysDict[systemID].firstOnboardTime ) sysDict[systemID].firstOnboardTime = time;

        ret = time + sysDict[systemID].onboardTimeOffset;
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

    uint32_t msgid = msg->msgid;

    // create new system data if it wasn't dectected yet
    if (!(sysDict.keys().contains(msgid))) {
        sysDict[msgid] = SystemData();
    }

    // Store component ID
    if (sysDict[msgid].componentID == -1)
    {
        sysDict[msgid].componentID = msg->compid;
    }
    else
    {
        // Got this message already
        if (sysDict[msgid].componentID != msg->compid)
        {
            sysDict[msgid].componentMulti = true;
        }
    }

    if (sysDict[msgid].componentMulti == true) {
        multiComponentSourceDetected = true;
    }

    // Add field tree widget item
    if (messageFilter.contains(msgid)) return;
    QString fieldName(msgInfo->fields[fieldid].name);
    QString fieldType;
    uint8_t* m = (uint8_t*)(msgDict[msgid].payload64);
    QString name("%1.%2");
    QString unit;

    // Debug vector messages
    if (msgid == MAVLINK_MSG_ID_DEBUG_VECT)
    {
        mavlink_debug_vect_t debug;
        mavlink_msg_debug_vect_decode(msg, &debug);
        char buf[11];
        strncpy(buf, debug.name, 10);
        buf[10] = '\0';
        name = QString("%1.%2").arg(buf, fieldName);
        time = getUnixTimeFromMs(msg->sysid, (debug.time_usec+500)/1000); // Scale to milliseconds, round up/down correctly
    }
    else if (msgid == MAVLINK_MSG_ID_DEBUG_FLOAT_ARRAY)
    {
        mavlink_debug_float_array_t debug;
        mavlink_msg_debug_float_array_decode(msg, &debug);
        char buf[11];
        strncpy(buf, debug.name, 10);
        buf[10] = '\0';
        name = QString("%1.%2").arg(buf, fieldName);
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
        name = name.arg(msgInfo->name, fieldName);
        name.prepend(QString("port%1_").arg(raw.port));
    }
    else if (msgid == MAVLINK_MSG_ID_RC_CHANNELS_SCALED)
    {
        // XXX this is really ugly, but we do not know a better way to do this
        mavlink_rc_channels_scaled_t scaled;
        mavlink_msg_rc_channels_scaled_decode(msg, &scaled);
        name = name.arg(msgInfo->name, fieldName);
        name.prepend(QString("port%1_").arg(scaled.port));
    }
    else if (msgid == MAVLINK_MSG_ID_SERVO_OUTPUT_RAW)
    {
        // XXX this is really ugly, but we do not know a better way to do this
        mavlink_servo_output_raw_t servo;
        mavlink_msg_servo_output_raw_decode(msg, &servo);
        name = name.arg(msgInfo->name, fieldName);
        name.prepend(QString("port%1_").arg(servo.port));
    }
    else
    {
        name = name.arg(msgInfo->name, fieldName);
    }

    if (multiComponentSourceDetected)
    {
        name = name.prepend(QString("C%1:").arg(msg->compid));
    }

    name = name.prepend(QString("M%1:").arg(msg->sysid));

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
