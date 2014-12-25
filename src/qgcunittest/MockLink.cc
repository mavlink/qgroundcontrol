/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
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

#include "MockLink.h"

#include <QTimer>
#include <QDebug>
#include <QFile>

#include <string.h>

Q_LOGGING_CATEGORY(MockLinkLog, "MockLinkLog")

/// @file
///     @brief Mock implementation of a Link.
///
///     @author Don Gagne <don@thegagnes.com>

enum PX4_CUSTOM_MAIN_MODE {
    PX4_CUSTOM_MAIN_MODE_MANUAL = 1,
    PX4_CUSTOM_MAIN_MODE_ALTCTL,
    PX4_CUSTOM_MAIN_MODE_POSCTL,
    PX4_CUSTOM_MAIN_MODE_AUTO,
    PX4_CUSTOM_MAIN_MODE_ACRO,
    PX4_CUSTOM_MAIN_MODE_OFFBOARD,
};

enum PX4_CUSTOM_SUB_MODE_AUTO {
    PX4_CUSTOM_SUB_MODE_AUTO_READY = 1,
    PX4_CUSTOM_SUB_MODE_AUTO_TAKEOFF,
    PX4_CUSTOM_SUB_MODE_AUTO_LOITER,
    PX4_CUSTOM_SUB_MODE_AUTO_MISSION,
    PX4_CUSTOM_SUB_MODE_AUTO_RTL,
    PX4_CUSTOM_SUB_MODE_AUTO_LAND,
    PX4_CUSTOM_SUB_MODE_AUTO_RTGS
};

union px4_custom_mode {
    struct {
        uint16_t reserved;
        uint8_t main_mode;
        uint8_t sub_mode;
    };
    uint32_t data;
    float data_float;
};

MockLink::MockLink(void) :
    _linkId(getNextLinkId()),
    _name("MockLink"),
    _connected(false),
    _vehicleSystemId(128),     // FIXME: Pull from eventual parameter manager
    _vehicleComponentId(200),  // FIXME: magic number?
    _inNSH(false),
    _mavlinkStarted(false),
    _mavBaseMode(MAV_MODE_FLAG_MANUAL_INPUT_ENABLED | MAV_MODE_FLAG_CUSTOM_MODE_ENABLED),
    _mavState(MAV_STATE_STANDBY),
    _autopilotType(MAV_AUTOPILOT_PX4)
{
    union px4_custom_mode   px4_cm;

    px4_cm.data = 0;
    px4_cm.main_mode = PX4_CUSTOM_MAIN_MODE_MANUAL;
    _mavCustomMode = px4_cm.data;

    _missionItemHandler = new MockLinkMissionItemHandler(_vehicleSystemId, this);
    Q_CHECK_PTR(_missionItemHandler);
    
    moveToThread(this);
    _loadParams();
    QObject::connect(this, &MockLink::_incomingBytes, this, &MockLink::_handleIncomingBytes);
}

MockLink::~MockLink(void)
{
    _disconnect();
}

void MockLink::readBytes(void)
{
    // FIXME: This is a bad virtual from LinkInterface?
}

bool MockLink::_connect(void)
{
    if (!_connected) {
        _connected = true;
        start();
        emit connected();
    }
    
    return true;
}

bool MockLink::_disconnect(void)
{
    if (_connected) {
        _connected = false;
        exit();        
        emit disconnected();
    }
    
    return true;
}

void MockLink::run(void)
{
    QTimer  _timer1HzTasks;
    QTimer  _timer10HzTasks;
    QTimer  _timer50HzTasks;
    
    QObject::connect(&_timer1HzTasks, &QTimer::timeout, this, &MockLink::_run1HzTasks);
    QObject::connect(&_timer10HzTasks, &QTimer::timeout, this, &MockLink::_run10HzTasks);
    QObject::connect(&_timer50HzTasks, &QTimer::timeout, this, &MockLink::_run50HzTasks);
    
    _timer1HzTasks.start(1000);
    _timer10HzTasks.start(100);
    _timer50HzTasks.start(20);
    
    exec();
    
    QObject::disconnect(&_timer1HzTasks, &QTimer::timeout, this, &MockLink::_run1HzTasks);
    QObject::disconnect(&_timer10HzTasks, &QTimer::timeout, this, &MockLink::_run10HzTasks);
    QObject::disconnect(&_timer50HzTasks, &QTimer::timeout, this, &MockLink::_run50HzTasks);
}

void MockLink::_run1HzTasks(void)
{
    if (_mavlinkStarted && _connected) {
        _sendHeartBeat();
    }
}

void MockLink::_run10HzTasks(void)
{
    if (_mavlinkStarted && _connected) {
    }
}

void MockLink::_run50HzTasks(void)
{
    if (_mavlinkStarted && _connected) {
    }
}

void MockLink::_loadParams(void)
{
    QFile paramFile(":/unittest/MockLink.param");
    
    bool success = paramFile.open(QFile::ReadOnly);
    Q_UNUSED(success);
    Q_ASSERT(success);
    
    QTextStream paramStream(&paramFile);
    
    while (!paramStream.atEnd()) {
        QString line = paramStream.readLine();
        
        if (line.startsWith("#")) {
            continue;
        }
        
        QStringList paramData = line.split("\t");
        Q_ASSERT(paramData.count() == 5);
        
        QString paramName = paramData.at(2);
        QString valStr = paramData.at(3);
        uint paramType = paramData.at(4).toUInt();
        
        QVariant paramValue;
        switch (paramType) {
            case MAV_PARAM_TYPE_REAL32:
                paramValue = QVariant(valStr.toFloat());
                break;
            case MAV_PARAM_TYPE_UINT32:
                paramValue = QVariant(valStr.toUInt());
                break;
            case MAV_PARAM_TYPE_INT32:
                paramValue = QVariant(valStr.toInt());
                break;
            case MAV_PARAM_TYPE_INT8:
                paramValue = QVariant((unsigned char)valStr.toUInt());
                break;
            default:
                Q_ASSERT(false);
                break;
        }
        
        qCDebug(MockLinkLog) << "Loading param" << paramName << paramValue;
        
        _mapParamName2Value[paramName] = paramValue;
        _mapParamName2MavParamType[paramName] = static_cast<MAV_PARAM_TYPE>(paramType);
    }
}

void MockLink::_sendHeartBeat(void)
{
    mavlink_message_t   msg;
    uint8_t             buffer[MAVLINK_MAX_PACKET_LEN];

    mavlink_msg_heartbeat_pack(_vehicleSystemId,
                               _vehicleComponentId,
                               &msg,
                               MAV_TYPE_QUADROTOR,  // MAV_TYPE
                               _autopilotType,      // MAV_AUTOPILOT
                               _mavBaseMode,        // MAV_MODE
                               _mavCustomMode,      // custom mode
                               _mavState);          // MAV_STATE

    int cBuffer = mavlink_msg_to_send_buffer(buffer, &msg);
    QByteArray bytes((char *)buffer, cBuffer);
    emit bytesReceived(this, bytes);
}

/// @brief Called when QGC wants to write bytes to the MAV
void MockLink::writeBytes(const char* bytes, qint64 cBytes)
{
    // Package up the data so we can signal it over to the right thread
    QByteArray byteArray(bytes, cBytes);
    
    emit _incomingBytes(byteArray);
}

/// @brief Handles bytes from QGC on the thread
void MockLink::_handleIncomingBytes(const QByteArray bytes)
{
    if (_inNSH) {
        _handleIncomingNSHBytes(bytes.constData(), bytes.count());
    } else {
        if (bytes.startsWith(QByteArray("\r\r\r"))) {
            _inNSH  = true;
            _handleIncomingNSHBytes(&bytes.constData()[3], bytes.count() - 3);
        }
        
        _handleIncomingMavlinkBytes((uint8_t *)bytes.constData(), bytes.count());
    }
}

/// @brief Handle incoming bytes which are meant to be interpreted by the NuttX shell
void MockLink::_handleIncomingNSHBytes(const char* bytes, int cBytes)
{
    Q_UNUSED(cBytes);
    
    // Drop back out of NSH
    if (cBytes == 4 && bytes[0] == '\r' && bytes[1] == '\r' && bytes[2] == '\r') {
        _inNSH  = false;
        return;
    }

    if (cBytes > 0) {
        qDebug() << "NSH:" << (const char*)bytes;
        
        if (strncmp(bytes, "sh /etc/init.d/rc.usb\n", cBytes) == 0) {
            // This is the mavlink start command
            _mavlinkStarted = true;
        }
    }
}

/// @brief Handle incoming bytes which are meant to be handled by the mavlink protocol
void MockLink::_handleIncomingMavlinkBytes(const uint8_t* bytes, int cBytes)
{
    mavlink_message_t msg;
    mavlink_status_t comm;
    
    for (qint64 i=0; i<cBytes; i++)
    {
        if (!mavlink_parse_char(_linkId, bytes[i], &msg, &comm)) {
            continue;
        }
        
        Q_ASSERT(_missionItemHandler);
        _missionItemHandler->handleMessage(msg);
        
        switch (msg.msgid) {
            case MAVLINK_MSG_ID_HEARTBEAT:
                _handleHeartBeat(msg);
                break;
                
            case MAVLINK_MSG_ID_PARAM_REQUEST_LIST:
                _handleParamRequestList(msg);
                break;
                
            case MAVLINK_MSG_ID_SET_MODE:
                _handleSetMode(msg);
                break;
                
            case MAVLINK_MSG_ID_PARAM_SET:
                _handleParamSet(msg);
                break;
                
            case MAVLINK_MSG_ID_PARAM_REQUEST_READ:
                _handleParamRequestRead(msg);
                break;
                
            case MAVLINK_MSG_ID_MISSION_REQUEST_LIST:
                _handleMissionRequestList(msg);
                break;
                
            case MAVLINK_MSG_ID_MISSION_REQUEST:
                _handleMissionRequest(msg);
                break;
                
            case MAVLINK_MSG_ID_MISSION_ITEM:
                _handleMissionItem(msg);
                break;
                
#if 0
            case MAVLINK_MSG_ID_MISSION_COUNT:
                _handleMissionCount(msg);
                break;
#endif
                
            default:
                qDebug() << "MockLink: Unhandled mavlink message, id:" << msg.msgid;
                break;
        }
    }
}

void MockLink::_emitMavlinkMessage(const mavlink_message_t& msg)
{
    uint8_t outputBuffer[MAVLINK_MAX_PACKET_LEN];
    
    int cBuffer = mavlink_msg_to_send_buffer(outputBuffer, &msg);
    QByteArray bytes((char *)outputBuffer, cBuffer);
    emit bytesReceived(this, bytes);
}

void MockLink::_handleHeartBeat(const mavlink_message_t& msg)
{
    Q_UNUSED(msg);
#if 0
    mavlink_heartbeat_t heartbeat;
    mavlink_msg_heartbeat_decode(&msg, &heartbeat);
#endif
}

void MockLink::_handleSetMode(const mavlink_message_t& msg)
{
    mavlink_set_mode_t request;
    mavlink_msg_set_mode_decode(&msg, &request);
    
    Q_ASSERT(request.target_system == _vehicleSystemId);
    
    _mavBaseMode = request.base_mode;
    _mavCustomMode = request.custom_mode;
}

void MockLink::_setParamFloatUnionIntoMap(const QString& paramName, float paramFloat)
{
    mavlink_param_union_t   valueUnion;
    
    Q_ASSERT(_mapParamName2Value.contains(paramName));
    Q_ASSERT(_mapParamName2MavParamType.contains(paramName));
    
    valueUnion.param_float = paramFloat;
    
    MAV_PARAM_TYPE paramType = _mapParamName2MavParamType[paramName];
    
    QVariant paramVariant;
    
    switch (paramType) {
        case MAV_PARAM_TYPE_INT8:
            paramVariant = QVariant::fromValue(valueUnion.param_int8);
            break;
            
        case MAV_PARAM_TYPE_INT32:
            paramVariant = QVariant::fromValue(valueUnion.param_int32);
            break;
            
        case MAV_PARAM_TYPE_UINT32:
            paramVariant = QVariant::fromValue(valueUnion.param_uint32);
            break;
            
        case MAV_PARAM_TYPE_REAL32:
            paramVariant = QVariant::fromValue(valueUnion.param_float);
            break;
            
        default:
            qCritical() << "Invalid parameter type" << paramType;
    }
    
    qCDebug(MockLinkLog) << "_setParamFloatUnionIntoMap" << paramName << paramVariant;
    _mapParamName2Value[paramName] = paramVariant;
}

/// Convert from a parameter variant to the float value from mavlink_param_union_t
float MockLink::_floatUnionForParam(const QString& paramName)
{
    mavlink_param_union_t   valueUnion;
    
    Q_ASSERT(_mapParamName2Value.contains(paramName));
    Q_ASSERT(_mapParamName2MavParamType.contains(paramName));
    
    MAV_PARAM_TYPE paramType = _mapParamName2MavParamType[paramName];
    QVariant paramVar = _mapParamName2Value[paramName];
    
    switch (paramType) {
        case MAV_PARAM_TYPE_INT8:
            valueUnion.param_int8 = (unsigned char)paramVar.toChar().toLatin1();
            break;
            
        case MAV_PARAM_TYPE_INT32:
            valueUnion.param_int32 = paramVar.toInt();
            break;
            
        case MAV_PARAM_TYPE_UINT32:
            valueUnion.param_uint32 = paramVar.toUInt();
            break;
            
        case MAV_PARAM_TYPE_REAL32:
            valueUnion.param_float = paramVar.toFloat();
            break;
            
        default:
            qCritical() << "Invalid parameter type" << paramType;
    }
    
    return valueUnion.param_float;
}

void MockLink::_handleParamRequestList(const mavlink_message_t& msg)
{
    uint16_t paramIndex = 0;
    mavlink_param_request_list_t request;
    
    mavlink_msg_param_request_list_decode(&msg, &request);
    
    int cParameters = _mapParamName2Value.count();
    
    Q_ASSERT(request.target_system == _vehicleSystemId);
    
    foreach(QString paramName, _mapParamName2Value.keys()) {
        char paramId[MAVLINK_MSG_ID_PARAM_VALUE_LEN];
        mavlink_message_t       responseMsg;
        
        Q_ASSERT(_mapParamName2Value.contains(paramName));
        Q_ASSERT(_mapParamName2MavParamType.contains(paramName));
        
        MAV_PARAM_TYPE paramType = _mapParamName2MavParamType[paramName];
        
        Q_ASSERT(paramName.length() <= MAVLINK_MSG_ID_PARAM_VALUE_LEN);
        strncpy(paramId, paramName.toLocal8Bit().constData(), MAVLINK_MSG_ID_PARAM_VALUE_LEN);
        
        qCDebug(MockLinkLog) << "Sending msg_param_value" << paramId << paramType;
        
        mavlink_msg_param_value_pack(_vehicleSystemId,
                                     _vehicleComponentId,
                                     &responseMsg,                      // Outgoing message
                                     paramId,                           // Parameter name
                                     _floatUnionForParam(paramName),    // Parameter value
                                     paramType,                         // MAV_PARAM_TYPE
                                     cParameters,                       // Total number of parameters
                                     paramIndex++);                     // Index of this parameter
        _emitMavlinkMessage(responseMsg);
    }
}

void MockLink::_handleParamSet(const mavlink_message_t& msg)
{
    mavlink_param_set_t request;
    mavlink_msg_param_set_decode(&msg, &request);
    
    Q_ASSERT(request.target_system == _vehicleSystemId);

    // Param may not be null terminated if exactly fits
    char paramId[MAVLINK_MSG_PARAM_SET_FIELD_PARAM_ID_LEN + 1];
    strncpy(paramId, request.param_id, MAVLINK_MSG_PARAM_SET_FIELD_PARAM_ID_LEN);
    
    Q_ASSERT(_mapParamName2Value.contains(paramId));
    Q_ASSERT(request.param_type == _mapParamName2MavParamType[paramId]);
    
    // Save the new value
    _setParamFloatUnionIntoMap(paramId, request.param_value);
    
    // Respond with a param_value to ack
    mavlink_message_t responseMsg;
    mavlink_msg_param_value_pack(_vehicleSystemId,
                                 _vehicleComponentId,
                                 &responseMsg,                  // Outgoing message
                                 paramId,                       // Parameter name
                                 request.param_value,           // Send same value back
                                 request.param_type,            // Send same type back
                                 _mapParamName2Value.count(),   // Total number of parameters
                                 _mapParamName2Value.keys().indexOf(paramId));  // Index of this parameter
    _emitMavlinkMessage(responseMsg);
}

void MockLink::_handleParamRequestRead(const mavlink_message_t& msg)
{
    mavlink_param_request_read_t request;
    mavlink_msg_param_request_read_decode(&msg, &request);
    
    char paramId[MAVLINK_MSG_PARAM_REQUEST_READ_FIELD_PARAM_ID_LEN + 1];
    paramId[0] = 0;
    
    Q_ASSERT(request.target_system == _vehicleSystemId);
    
    if (request.param_index == -1) {
        // Request is by param name. Param may not be null terminated if exactly fits
        strncpy(paramId, request.param_id, MAVLINK_MSG_PARAM_REQUEST_READ_FIELD_PARAM_ID_LEN);
    } else {
        // Request is by index
        
        Q_ASSERT(request.param_index >= 0 && request.param_index < _mapParamName2Value.count());
        
        QString key = _mapParamName2Value.keys().at(request.param_index);
        Q_ASSERT(key.length() <= MAVLINK_MSG_PARAM_REQUEST_READ_FIELD_PARAM_ID_LEN);
        strcpy(paramId, key.toLocal8Bit().constData());
    }

    Q_ASSERT(_mapParamName2Value.contains(paramId));
    Q_ASSERT(_mapParamName2MavParamType.contains(paramId));
    
    mavlink_message_t   responseMsg;
    
    mavlink_msg_param_value_pack(_vehicleSystemId,
                                 _vehicleComponentId,
                                 &responseMsg,                          // Outgoing message
                                 paramId,                               // Parameter name
                                 _floatUnionForParam(paramId),          // Parameter value
                                 _mapParamName2MavParamType[paramId],   // Parameter type
                                 _mapParamName2Value.count(),           // Total number of parameters
                                 _mapParamName2Value.keys().indexOf(paramId));  // Index of this parameter
    _emitMavlinkMessage(responseMsg);
}

void MockLink::_handleMissionRequestList(const mavlink_message_t& msg)
{
    mavlink_mission_request_list_t request;
    
    mavlink_msg_mission_request_list_decode(&msg, &request);
    
    Q_ASSERT(request.target_system == _vehicleSystemId);
    
    mavlink_message_t   responseMsg;

    mavlink_msg_mission_count_pack(_vehicleSystemId,
                                   _vehicleComponentId,
                                   &responseMsg,            // Outgoing message
                                   msg.sysid,               // Target is original sender
                                   msg.compid,              // Target is original sender
                                   _missionItems.count());  // Number of mission items
    _emitMavlinkMessage(responseMsg);
}

void MockLink::_handleMissionRequest(const mavlink_message_t& msg)
{
    mavlink_mission_request_t request;
    
    mavlink_msg_mission_request_decode(&msg, &request);
    
    Q_ASSERT(request.target_system == _vehicleSystemId);
    Q_ASSERT(request.seq < _missionItems.count());
    
    mavlink_message_t   responseMsg;

    mavlink_mission_item_t item = _missionItems[request.seq];

    mavlink_msg_mission_item_pack(_vehicleSystemId,
                                  _vehicleComponentId,
                                  &responseMsg,            // Outgoing message
                                  msg.sysid,               // Target is original sender
                                  msg.compid,              // Target is original sender
                                  request.seq,             // Index of mission item being sent
                                  item.frame,
                                  item.command,
                                  item.current,
                                  item.autocontinue,
                                  item.param1, item.param2, item.param3, item.param4,
                                  item.x, item.y, item.z);
    _emitMavlinkMessage(responseMsg);
}

void MockLink::_handleMissionItem(const mavlink_message_t& msg)
{
    mavlink_mission_item_t request;
    
    mavlink_msg_mission_item_decode(&msg, &request);
    
    Q_ASSERT(request.target_system == _vehicleSystemId);
    
    // FIXME: What do you do with duplication sequence numbers?
    Q_ASSERT(!_missionItems.contains(request.seq));
    
    _missionItems[request.seq] = request;
}
