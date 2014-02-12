#include "FactMavShim.h"
#include "UAS.h"
#include <QDebug>
#include <QStringList>

typedef struct {
    int id;
    const char *name;
} Id2Msg;

#define ID2MSGROW(id) { id, #id },

static const Id2Msg rgId2Msg[] = {
    ID2MSGROW(MAVLINK_MSG_ID_SENSOR_OFFSETS)
    ID2MSGROW(MAVLINK_MSG_ID_SET_MAG_OFFSETS)
    ID2MSGROW(MAVLINK_MSG_ID_MEMINFO)
    ID2MSGROW(MAVLINK_MSG_ID_AP_ADC)
    ID2MSGROW(MAVLINK_MSG_ID_DIGICAM_CONFIGURE)
    ID2MSGROW(MAVLINK_MSG_ID_DIGICAM_CONTROL)
    ID2MSGROW(MAVLINK_MSG_ID_MOUNT_CONFIGURE)
    ID2MSGROW(MAVLINK_MSG_ID_MOUNT_CONTROL)
    ID2MSGROW(MAVLINK_MSG_ID_MOUNT_STATUS)
    ID2MSGROW(MAVLINK_MSG_ID_FENCE_POINT)
    ID2MSGROW(MAVLINK_MSG_ID_FENCE_FETCH_POINT)
    ID2MSGROW(MAVLINK_MSG_ID_FENCE_STATUS)
    ID2MSGROW(MAVLINK_MSG_ID_AHRS)
    ID2MSGROW(MAVLINK_MSG_ID_SIMSTATE)
    ID2MSGROW(MAVLINK_MSG_ID_HWSTATUS)
    ID2MSGROW(MAVLINK_MSG_ID_RADIO)
    ID2MSGROW(MAVLINK_MSG_ID_HEARTBEAT)
    ID2MSGROW(MAVLINK_MSG_ID_SYSTEM_TIME)
    ID2MSGROW(MAVLINK_MSG_ID_PING)
    ID2MSGROW(MAVLINK_MSG_ID_CHANGE_OPERATOR_CONTROL)
    ID2MSGROW(MAVLINK_MSG_ID_CHANGE_OPERATOR_CONTROL_ACK)
    ID2MSGROW(MAVLINK_MSG_ID_AUTH_KEY)
    ID2MSGROW(MAVLINK_MSG_ID_SET_MODE)
    ID2MSGROW(MAVLINK_MSG_ID_PARAM_REQUEST_READ)
    ID2MSGROW(MAVLINK_MSG_ID_PARAM_REQUEST_LIST)
    ID2MSGROW(MAVLINK_MSG_ID_PARAM_VALUE)
    ID2MSGROW(MAVLINK_MSG_ID_PARAM_SET)
    ID2MSGROW(MAVLINK_MSG_ID_GPS_RAW_INT)
    ID2MSGROW(MAVLINK_MSG_ID_SCALED_IMU)
    ID2MSGROW(MAVLINK_MSG_ID_GPS_STATUS)
    ID2MSGROW(MAVLINK_MSG_ID_RAW_IMU)
    ID2MSGROW(MAVLINK_MSG_ID_RAW_PRESSURE)
    ID2MSGROW(MAVLINK_MSG_ID_SCALED_PRESSURE)
    ID2MSGROW(MAVLINK_MSG_ID_ATTITUDE)
    ID2MSGROW(MAVLINK_MSG_ID_SYS_STATUS)
    ID2MSGROW(MAVLINK_MSG_ID_RC_CHANNELS_RAW)
    ID2MSGROW(MAVLINK_MSG_ID_RC_CHANNELS_SCALED)
    ID2MSGROW(MAVLINK_MSG_ID_SERVO_OUTPUT_RAW)
    ID2MSGROW(MAVLINK_MSG_ID_LOCAL_POSITION_SETPOINT)
    ID2MSGROW(MAVLINK_MSG_ID_SAFETY_SET_ALLOWED_AREA)
    ID2MSGROW(MAVLINK_MSG_ID_SAFETY_ALLOWED_AREA)
    ID2MSGROW(MAVLINK_MSG_ID_SET_ROLL_PITCH_YAW_THRUST)
    ID2MSGROW(MAVLINK_MSG_ID_SET_ROLL_PITCH_YAW_SPEED_THRUST)
    ID2MSGROW(MAVLINK_MSG_ID_ROLL_PITCH_YAW_THRUST_SETPOINT)
    ID2MSGROW(MAVLINK_MSG_ID_ROLL_PITCH_YAW_SPEED_THRUST_SETPOINT)
    ID2MSGROW(MAVLINK_MSG_ID_NAV_CONTROLLER_OUTPUT)
    ID2MSGROW(MAVLINK_MSG_ID_STATE_CORRECTION)
    ID2MSGROW(MAVLINK_MSG_ID_REQUEST_DATA_STREAM)
    ID2MSGROW(MAVLINK_MSG_ID_HIL_STATE)
    ID2MSGROW(MAVLINK_MSG_ID_HIL_CONTROLS)
    ID2MSGROW(MAVLINK_MSG_ID_MANUAL_CONTROL)
    ID2MSGROW(MAVLINK_MSG_ID_RC_CHANNELS_OVERRIDE)
    ID2MSGROW(MAVLINK_MSG_ID_GLOBAL_POSITION_INT)
    ID2MSGROW(MAVLINK_MSG_ID_VFR_HUD)
    ID2MSGROW(MAVLINK_MSG_ID_COMMAND_ACK)
    ID2MSGROW(MAVLINK_MSG_ID_OPTICAL_FLOW)
    ID2MSGROW(MAVLINK_MSG_ID_DEBUG_VECT)
    ID2MSGROW(MAVLINK_MSG_ID_NAMED_VALUE_FLOAT)
    ID2MSGROW(MAVLINK_MSG_ID_NAMED_VALUE_INT)
    ID2MSGROW(MAVLINK_MSG_ID_STATUSTEXT)
    ID2MSGROW(MAVLINK_MSG_ID_WIND)
};

FactMavShim::FactMavShim(QObject* parent) :
    QObject(parent),
    _uas(NULL)
{

}

void FactMavShim::setup(UASInterface* uas)
{
    _uas = uas;

    MAVLinkProtocol* mavlink = _uas->getMavlink();
    bool connected = connect(mavlink,
                             SIGNAL(messageReceived(LinkInterface*, mavlink_message_t)),
                             this,
                             SLOT(_receiveMessage(LinkInterface*, mavlink_message_t)));
}

void FactMavShim::_receiveMessage(LinkInterface* link, mavlink_message_t message)
{
    Q_ASSERT(_uas);
    
    static mavlink_message_info_t msgInfo[256] = MAVLINK_MESSAGE_INFO;

    
    // Only process messages targetted to our uas
    if (message.sysid == _uas->getUASID()) {
        switch (message.msgid) {
            case MAVLINK_MSG_ID_HEARTBEAT:
                _handleHeartbeat(message);
                break;
                
            case MAVLINK_MSG_ID_PARAM_VALUE:
                _handleParamValue(message);
                break;
                
            default:
                _processTelemetryData(msgInfo, 256, message);
                break;
        }
    }
}

void FactMavShim::_handleParamValue(mavlink_message_t& msg)
{
    Q_ASSERT(_uas);
    
    mavlink_param_value_t paramMsg;
    static const size_t cParamId = sizeof(paramMsg.param_id);
    char fixedId[cParamId+1];
    
    mavlink_msg_param_value_decode(&msg, &paramMsg);
    fixedId[cParamId] = 0;
    memcpy(fixedId, paramMsg.param_id, cParamId);
    
    emit receivedParamValue(_uas->getUASID(), QString(fixedId), paramMsg.param_type, paramMsg.param_value, paramMsg.param_index, paramMsg.param_count);
}

void FactMavShim::_handleHeartbeat(mavlink_message_t& msg)
{
    Q_ASSERT(_uas);
    
    static bool firstHeartbeat = true;
    mavlink_heartbeat_t heartbeatMsg;
    
    mavlink_msg_heartbeat_decode(&msg, &heartbeatMsg);
    
    if (firstHeartbeat) {
        firstHeartbeat = false;
        emit mavtypeKnown(_uas->getUASID(), heartbeatMsg.autopilot, heartbeatMsg.type);
    }
}

void FactMavShim::_processTelemetryData(mavlink_message_info_t* rgMessageInfo, size_t cMessageInfo, mavlink_message_t& message)
{
    Q_ASSERT(_uas);
    int uasId = _uas->getUASID();
    
    mavlink_message_info_t* msgInfo = &rgMessageInfo[message.msgid];
    for (size_t iField=0; iField<msgInfo->num_fields; iField++) {
        mavlink_field_info_t* fldInfo = &msgInfo->fields[iField];
        
        // FIXME: We don't handle arrays yet
        if (fldInfo->array_length) {
            continue;
        }
        
        uint8_t* m = ((uint8_t *)(&message)) + 8 + fldInfo->wire_offset;
        QString factId(QString(msgInfo->name) + "_" + fldInfo->name);
        switch (fldInfo->type)
        {
            case MAVLINK_TYPE_CHAR:
                // Single char
                emit receivedTelemValue(uasId, factId, fldInfo->type, QVariant(QChar(*((char*)m))));
                break;
            case MAVLINK_TYPE_UINT8_T:
                emit receivedTelemValue(uasId, factId, fldInfo->type, QVariant((uint)*((uint8_t*)m)));
                break;
            case MAVLINK_TYPE_INT8_T:
                emit receivedTelemValue(uasId, factId, fldInfo->type, QVariant((int)*((int8_t*)m)));
                break;
            case MAVLINK_TYPE_UINT16_T:
                emit receivedTelemValue(uasId, factId, fldInfo->type, QVariant((uint)*((uint16_t*)m)));
                break;
            case MAVLINK_TYPE_INT16_T:
                emit receivedTelemValue(uasId, factId, fldInfo->type, QVariant((int)*((int16_t*)m)));
                break;
            case MAVLINK_TYPE_UINT32_T:
                emit receivedTelemValue(uasId, factId, fldInfo->type, QVariant((uint)*((uint32_t*)m)));
                break;
            case MAVLINK_TYPE_INT32_T:
                emit receivedTelemValue(uasId, factId, fldInfo->type, QVariant((int)*((int32_t*)m)));
                break;
            case MAVLINK_TYPE_FLOAT:
                emit receivedTelemValue(uasId, factId, fldInfo->type, QVariant(*((float*)m)));
                break;
            case MAVLINK_TYPE_DOUBLE:
                emit receivedTelemValue(uasId, factId, fldInfo->type, QVariant(*((double*)m)));
                break;
            case MAVLINK_TYPE_UINT64_T:
                emit receivedTelemValue(uasId, factId, fldInfo->type, QVariant((qulonglong)*((uint64_t*)m)));
                break;
            case MAVLINK_TYPE_INT64_T:
                emit receivedTelemValue(uasId, factId, fldInfo->type, QVariant((qlonglong)*((int64_t*)m)));
                break;
            default:
                qDebug() << "WARNING: UNKNOWN MAVLINK TYPE";
        }
    }
}

