#include "MAVLinkDecoder.h"
#include "UASManager.h"

MAVLinkDecoder::MAVLinkDecoder(MAVLinkProtocol* protocol, QObject *parent) :
    QObject(parent)
{
    mavlink_message_info_t msg[256] = MAVLINK_MESSAGE_INFO;
    memcpy(messageInfo, msg, sizeof(mavlink_message_info_t)*256);
    memset(receivedMessages, 0, sizeof(mavlink_message_t)*256);
    for (unsigned int i = 0; i<255;++i)
    {
        componentID[i] = -1;
        componentMulti[i] = false;
        onboardTimeOffset[i] = 0;
    }



    // Fill filter
    messageFilter.insert(MAVLINK_MSG_ID_HEARTBEAT, false);
    messageFilter.insert(MAVLINK_MSG_ID_SYS_STATUS, false);
    messageFilter.insert(MAVLINK_MSG_ID_STATUSTEXT, false);
    messageFilter.insert(MAVLINK_MSG_ID_COMMAND_LONG, false);
    messageFilter.insert(MAVLINK_MSG_ID_COMMAND_ACK, false);
    messageFilter.insert(MAVLINK_MSG_ID_PARAM_SET, false);
    messageFilter.insert(MAVLINK_MSG_ID_PARAM_VALUE, false);
    messageFilter.insert(MAVLINK_MSG_ID_MISSION_ITEM, false);
    messageFilter.insert(MAVLINK_MSG_ID_MISSION_COUNT, false);
    messageFilter.insert(MAVLINK_MSG_ID_MISSION_ACK, false);
    messageFilter.insert(MAVLINK_MSG_ID_DATA_STREAM, false);
    #ifdef MAVLINK_ENABLED_PIXHAWK
    messageFilter.insert(MAVLINK_MSG_ID_ENCAPSULATED_DATA, false);
    messageFilter.insert(MAVLINK_MSG_ID_DATA_TRANSMISSION_HANDSHAKE, false);
    #endif
    messageFilter.insert(MAVLINK_MSG_ID_EXTENDED_MESSAGE, false);

    textMessageFilter.insert(MAVLINK_MSG_ID_DEBUG, false);
    textMessageFilter.insert(MAVLINK_MSG_ID_DEBUG_VECT, false);
    textMessageFilter.insert(MAVLINK_MSG_ID_NAMED_VALUE_FLOAT, false);
    textMessageFilter.insert(MAVLINK_MSG_ID_NAMED_VALUE_INT, false);

    connect(protocol, SIGNAL(messageReceived(LinkInterface*,mavlink_message_t)), this, SLOT(receiveMessage(LinkInterface*,mavlink_message_t)));
}

void MAVLinkDecoder::receiveMessage(LinkInterface* link,mavlink_message_t message)
{
    Q_UNUSED(link);
    memcpy(receivedMessages+message.msgid, &message, sizeof(mavlink_message_t));

    uint8_t msgid = message.msgid;

    // Handle time sync message
    if (message.msgid == MAVLINK_MSG_ID_SYSTEM_TIME && message.compid == 200)
    {
        mavlink_system_time_t timebase;
        mavlink_msg_system_time_decode(&message, &timebase);
        onboardTimeOffset[message.sysid] = timebase.time_unix_usec/1000 - timebase.time_boot_ms;
        onboardToGCSUnixTimeOffsetAndDelay[message.sysid] = static_cast<qint64>(QGC::groundTimeMilliseconds() - timebase.time_unix_usec/1000);
    }
    else
    {

        QString messageName("%1 (#%2)");
        messageName = messageName.arg(messageInfo[msgid].name).arg(msgid);

        // See if first value is a time value
        quint64 time = 0;
        uint8_t fieldid = 0;
        uint8_t* m = ((uint8_t*)(receivedMessages+msgid))+8;
        if (QString(messageInfo[msgid].fields[fieldid].name) == QString("time_boot_ms") && messageInfo[msgid].fields[fieldid].type == MAVLINK_TYPE_UINT32_T)
        {
            time = *((quint32*)(m+messageInfo[msgid].fields[fieldid].wire_offset));
        }
        else if (QString(messageInfo[msgid].fields[fieldid].name).contains("usec") && messageInfo[msgid].fields[fieldid].type == MAVLINK_TYPE_UINT64_T)
        {
            time = *((quint64*)(m+messageInfo[msgid].fields[fieldid].wire_offset));
            time = time/1000; // Scale to milliseconds
        }
        else
        {
            // First value is not time, send out value 0
            emitFieldValue(&message, fieldid, getUnixTimeFromMs(message.sysid, 0));
        }

        // Align time to global time
        time = getUnixTimeFromMs(message.sysid, time);

        // Send out field values from 1..n
        for (unsigned int i = 1; i < messageInfo[msgid].num_fields; ++i)
        {
            emitFieldValue(&message, i, time);
        }
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
        if (onboardTimeOffset[systemID] == 0 || time < (firstOnboardTime[systemID]-2000))
        {
            firstOnboardTime[systemID] = time;
            onboardTimeOffset[systemID] = QGC::groundTimeMilliseconds() - time;
        }
        ret = time + onboardTimeOffset[systemID];
    }
    else
    {
        // Time is not zero and larger than 40 years -> has to be
        // a Unix epoch timestamp. Do nothing.
        ret = time;
    }

    return ret;
}

void MAVLinkDecoder::emitFieldValue(mavlink_message_t* msg, int fieldid, quint64 time)
{
    bool multiComponentSourceDetected = false;
    bool wrongComponent = false;

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
            wrongComponent = true;
        }
    }

    if (componentMulti[msg->msgid] == true) multiComponentSourceDetected = true;

    // Add field tree widget item
    uint8_t msgid = msg->msgid;
    if (messageFilter.contains(msgid)) return;
    QString fieldName(messageInfo[msgid].fields[fieldid].name);
    QString fieldType;
    uint8_t* m = ((uint8_t*)(receivedMessages+msgid))+8;
    QString name("%1.%2");
    QString unit("");

    // Debug vector messages
    if (msgid == MAVLINK_MSG_ID_DEBUG_VECT)
    {
        mavlink_debug_vect_t debug;
        mavlink_msg_debug_vect_decode(msg, &debug);
        name = name.arg(QString(debug.name), fieldName);
        time = debug.time_usec / 1000;
    }
    else if (msgid == MAVLINK_MSG_ID_DEBUG)
    {
        mavlink_debug_t debug;
        mavlink_msg_debug_decode(msg, &debug);
        name = name.arg(QString("debug")).arg(debug.ind);
        time = debug.time_boot_ms;
    }
    else if (msgid == MAVLINK_MSG_ID_NAMED_VALUE_FLOAT)
    {
        mavlink_named_value_float_t debug;
        mavlink_msg_named_value_float_decode(msg, &debug);
        name = name.arg(debug.name).arg(fieldName);
        time = debug.time_boot_ms;
    }
    else if (msgid == MAVLINK_MSG_ID_NAMED_VALUE_INT)
    {
        mavlink_named_value_int_t debug;
        mavlink_msg_named_value_int_decode(msg, &debug);
        name = name.arg(debug.name).arg(fieldName);
        time = debug.time_boot_ms;
    }
    else
    {
        name = name.arg(messageInfo[msgid].name, fieldName);
    }

    if (multiComponentSourceDetected) name.prepend(QString("C%1:").arg(msg->compid));
    name.prepend(QString("M%1:").arg(msg->sysid));
    switch (messageInfo[msgid].fields[fieldid].type)
    {
    case MAVLINK_TYPE_CHAR:
        if (messageInfo[msgid].fields[fieldid].array_length > 0)
        {
            char* str = (char*)(m+messageInfo[msgid].fields[fieldid].wire_offset);
            // Enforce null termination
            str[messageInfo[msgid].fields[fieldid].array_length-1] = '\0';
            QString string(name + ": " + str);
            if (!textMessageFilter.contains(msgid)) emit textMessageReceived(msg->sysid, msg->compid, 0, string);
        }
        else
        {
            // Single char
            char b = *((char*)(m+messageInfo[msgid].fields[fieldid].wire_offset));
            unit = QString("char[%1]").arg(messageInfo[msgid].fields[fieldid].array_length);
            emit valueChanged(msg->sysid, name, unit, b, time);
        }
        break;
    case MAVLINK_TYPE_UINT8_T:
        if (messageInfo[msgid].fields[fieldid].array_length > 0)
        {
            uint8_t* nums = m+messageInfo[msgid].fields[fieldid].wire_offset;
            fieldType = QString("uint8_t[%1]").arg(messageInfo[msgid].fields[fieldid].array_length);
            for (unsigned int j = 0; j < messageInfo[msgid].fields[fieldid].array_length; ++j)
            {
                emit valueChanged(msg->sysid, QString("%1.%2").arg(name).arg(j), fieldType, nums[j], time);
            }
        }
        else
        {
            // Single value
            uint8_t u = *(m+messageInfo[msgid].fields[fieldid].wire_offset);
            fieldType = "uint8_t";
            emit valueChanged(msg->sysid, name, fieldType, u, time);
        }
        break;
    case MAVLINK_TYPE_INT8_T:
        if (messageInfo[msgid].fields[fieldid].array_length > 0)
        {
            int8_t* nums = (int8_t*)(m+messageInfo[msgid].fields[fieldid].wire_offset);
            fieldType = QString("int8_t[%1]").arg(messageInfo[msgid].fields[fieldid].array_length);
            for (unsigned int j = 0; j < messageInfo[msgid].fields[j].array_length; ++j)
            {
                emit valueChanged(msg->sysid, QString("%1.%2").arg(name).arg(j), fieldType, nums[j], time);
            }
        }
        else
        {
            // Single value
            int8_t n = *((int8_t*)(m+messageInfo[msgid].fields[fieldid].wire_offset));
            fieldType = "int8_t";
            emit valueChanged(msg->sysid, name, fieldType, n, time);
        }
        break;
    case MAVLINK_TYPE_UINT16_T:
        if (messageInfo[msgid].fields[fieldid].array_length > 0)
        {
            uint16_t* nums = (uint16_t*)(m+messageInfo[msgid].fields[fieldid].wire_offset);
            fieldType = QString("uint16_t[%1]").arg(messageInfo[msgid].fields[fieldid].array_length);
            for (unsigned int j = 0; j < messageInfo[msgid].fields[j].array_length; ++j)
            {
                emit valueChanged(msg->sysid, QString("%1.%2").arg(name).arg(j), fieldType, nums[j], time);
            }
        }
        else
        {
            // Single value
            uint16_t n = *((uint16_t*)(m+messageInfo[msgid].fields[fieldid].wire_offset));
            fieldType = "uint16_t";
            emit valueChanged(msg->sysid, name, fieldType, n, time);
        }
        break;
    case MAVLINK_TYPE_INT16_T:
        if (messageInfo[msgid].fields[fieldid].array_length > 0)
        {
            int16_t* nums = (int16_t*)(m+messageInfo[msgid].fields[fieldid].wire_offset);
            fieldType = QString("int16_t[%1]").arg(messageInfo[msgid].fields[fieldid].array_length);
            for (unsigned int j = 0; j < messageInfo[msgid].fields[j].array_length; ++j)
            {
                emit valueChanged(msg->sysid, QString("%1.%2").arg(name).arg(j), fieldType, nums[j], time);
            }
        }
        else
        {
            // Single value
            int16_t n = *((int16_t*)(m+messageInfo[msgid].fields[fieldid].wire_offset));
            fieldType = "int16_t";
            emit valueChanged(msg->sysid, name, fieldType, n, time);
        }
        break;
    case MAVLINK_TYPE_UINT32_T:
        if (messageInfo[msgid].fields[fieldid].array_length > 0)
        {
            uint32_t* nums = (uint32_t*)(m+messageInfo[msgid].fields[fieldid].wire_offset);
            fieldType = QString("uint32_t[%1]").arg(messageInfo[msgid].fields[fieldid].array_length);
            for (unsigned int j = 0; j < messageInfo[msgid].fields[j].array_length; ++j)
            {
                emit valueChanged(msg->sysid, QString("%1.%2").arg(name).arg(j), fieldType, nums[j], time);
            }
        }
        else
        {
            // Single value
            float n = *((uint32_t*)(m+messageInfo[msgid].fields[fieldid].wire_offset));
            fieldType = "uint32_t";
            emit valueChanged(msg->sysid, name, fieldType, n, time);
        }
        break;
    case MAVLINK_TYPE_INT32_T:
        if (messageInfo[msgid].fields[fieldid].array_length > 0)
        {
            int32_t* nums = (int32_t*)(m+messageInfo[msgid].fields[fieldid].wire_offset);
            fieldType = QString("int32_t[%1]").arg(messageInfo[msgid].fields[fieldid].array_length);
            for (unsigned int j = 0; j < messageInfo[msgid].fields[j].array_length; ++j)
            {
                emit valueChanged(msg->sysid, QString("%1.%2").arg(name).arg(j), fieldType, nums[j], time);
            }
        }
        else
        {
            // Single value
            int32_t n = *((int32_t*)(m+messageInfo[msgid].fields[fieldid].wire_offset));
            fieldType = "int32_t";
            emit valueChanged(msg->sysid, name, fieldType, n, time);
        }
        break;
    case MAVLINK_TYPE_FLOAT:
        if (messageInfo[msgid].fields[fieldid].array_length > 0)
        {
            float* nums = (float*)(m+messageInfo[msgid].fields[fieldid].wire_offset);
            fieldType = QString("float[%1]").arg(messageInfo[msgid].fields[fieldid].array_length);
            for (unsigned int j = 0; j < messageInfo[msgid].fields[j].array_length; ++j)
            {
                emit valueChanged(msg->sysid, QString("%1.%2").arg(name).arg(j), fieldType, nums[j], time);
            }
        }
        else
        {
            // Single value
            float f = *((float*)(m+messageInfo[msgid].fields[fieldid].wire_offset));
            fieldType = "float";
            emit valueChanged(msg->sysid, name, fieldType, f, time);
        }
        break;
    case MAVLINK_TYPE_DOUBLE:
        if (messageInfo[msgid].fields[fieldid].array_length > 0)
        {
            double* nums = (double*)(m+messageInfo[msgid].fields[fieldid].wire_offset);
            fieldType = QString("double[%1]").arg(messageInfo[msgid].fields[fieldid].array_length);
            for (unsigned int j = 0; j < messageInfo[msgid].fields[j].array_length; ++j)
            {
                emit valueChanged(msg->sysid, QString("%1.%2").arg(name).arg(j), fieldType, nums[j], time);
            }
        }
        else
        {
            // Single value
            double f = *((double*)(m+messageInfo[msgid].fields[fieldid].wire_offset));
            fieldType = "double";
            emit valueChanged(msg->sysid, name, fieldType, f, time);
        }
        break;
    case MAVLINK_TYPE_UINT64_T:
        if (messageInfo[msgid].fields[fieldid].array_length > 0)
        {
            uint64_t* nums = (uint64_t*)(m+messageInfo[msgid].fields[fieldid].wire_offset);
            fieldType = QString("uint64_t[%1]").arg(messageInfo[msgid].fields[fieldid].array_length);
            for (unsigned int j = 0; j < messageInfo[msgid].fields[j].array_length; ++j)
            {
                emit valueChanged(msg->sysid, QString("%1.%2").arg(name).arg(j), fieldType, (quint64) nums[j], time);
            }
        }
        else
        {
            // Single value
            uint64_t n = *((uint64_t*)(m+messageInfo[msgid].fields[fieldid].wire_offset));
            fieldType = "uint64_t";
            emit valueChanged(msg->sysid, name, fieldType, (quint64) n, time);
        }
        break;
    case MAVLINK_TYPE_INT64_T:
        if (messageInfo[msgid].fields[fieldid].array_length > 0)
        {
            int64_t* nums = (int64_t*)(m+messageInfo[msgid].fields[fieldid].wire_offset);
            fieldType = QString("int64_t[%1]").arg(messageInfo[msgid].fields[fieldid].array_length);
            for (unsigned int j = 0; j < messageInfo[msgid].fields[j].array_length; ++j)
            {
                emit valueChanged(msg->sysid, QString("%1.%2").arg(name).arg(j), fieldType, (qint64) nums[j], time);
            }
        }
        else
        {
            // Single value
            int64_t n = *((int64_t*)(m+messageInfo[msgid].fields[fieldid].wire_offset));
            fieldType = "int64_t";
            emit valueChanged(msg->sysid, name, fieldType, (qint64) n, time);
        }
        break;
    }
}
