/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "MockLink.h"
#include "QGCLoggingCategory.h"
#include "QGCApplication.h"

#include <QTimer>
#include <QDebug>
#include <QFile>

#include <string.h>

QGC_LOGGING_CATEGORY(MockLinkLog, "MockLinkLog")
QGC_LOGGING_CATEGORY(MockLinkVerboseLog, "MockLinkVerboseLog")

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
    PX4_CUSTOM_MAIN_MODE_STABILIZED,
    PX4_CUSTOM_MAIN_MODE_RATTITUDE
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

float MockLink::_vehicleLatitude =  47.633033f;
float MockLink::_vehicleLongitude = -122.08794f;
float MockLink::_vehicleAltitude =  3.5f;

const char* MockConfiguration::_firmwareTypeKey =   "FirmwareType";
const char* MockConfiguration::_vehicleTypeKey =    "VehicleType";
const char* MockConfiguration::_sendStatusTextKey = "SendStatusText";

MockLink::MockLink(MockConfiguration* config)
    : _missionItemHandler(this, qgcApp()->toolbox()->mavlinkProtocol())
    , _name("MockLink")
    , _connected(false)
    , _vehicleSystemId(128)     // FIXME: Pull from eventual parameter manager
    , _vehicleComponentId(200)  // FIXME: magic number?
    , _inNSH(false)
    , _mavlinkStarted(true)
    , _mavBaseMode(MAV_MODE_FLAG_MANUAL_INPUT_ENABLED | MAV_MODE_FLAG_CUSTOM_MODE_ENABLED)
    , _mavState(MAV_STATE_STANDBY)
    , _firmwareType(MAV_AUTOPILOT_PX4)
    , _vehicleType(MAV_TYPE_QUADROTOR)
    , _fileServer(NULL)
    , _sendStatusText(false)
    , _apmSendHomePositionOnEmptyList(false)
    , _sendHomePositionDelayCount(2)
{
    _config = config;
    if (_config) {
        _firmwareType = config->firmwareType();
        _vehicleType = config->vehicleType();
        _sendStatusText = config->sendStatusText();
        _config->setLink(this);
    }

    union px4_custom_mode   px4_cm;

    px4_cm.data = 0;
    px4_cm.main_mode = PX4_CUSTOM_MAIN_MODE_MANUAL;
    _mavCustomMode = px4_cm.data;

    _fileServer = new MockLinkFileServer(_vehicleSystemId, _vehicleComponentId, this);
    Q_CHECK_PTR(_fileServer);

    moveToThread(this);

    _loadParams();
}

MockLink::~MockLink(void)
{
    _disconnect();
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

void MockLink::_disconnect(void)
{
    if (_connected) {
        _connected = false;
        quit();
        wait();
        emit disconnected();
    }
}

void MockLink::run(void)
{
    QTimer  _timer1HzTasks;
    QTimer  _timer10HzTasks;
    QTimer  _timer50HzTasks;

    QObject::connect(&_timer1HzTasks,  &QTimer::timeout, this, &MockLink::_run1HzTasks);
    QObject::connect(&_timer10HzTasks, &QTimer::timeout, this, &MockLink::_run10HzTasks);
    QObject::connect(&_timer50HzTasks, &QTimer::timeout, this, &MockLink::_run50HzTasks);

    _timer1HzTasks.start(1000);
    _timer10HzTasks.start(100);
    _timer50HzTasks.start(20);

    exec();

    QObject::disconnect(&_timer1HzTasks,  &QTimer::timeout, this, &MockLink::_run1HzTasks);
    QObject::disconnect(&_timer10HzTasks, &QTimer::timeout, this, &MockLink::_run10HzTasks);
    QObject::disconnect(&_timer50HzTasks, &QTimer::timeout, this, &MockLink::_run50HzTasks);

    _missionItemHandler.shutdown();
}

void MockLink::_run1HzTasks(void)
{
    if (_mavlinkStarted && _connected) {
        _sendHeartBeat();
        _sendVibration();
        if (_sendHomePositionDelayCount > 0) {
            // We delay home position a bit to be more realistic
            _sendHomePositionDelayCount--;
        } else {
            _sendHomePosition();
        }
        if (_sendStatusText) {
            _sendStatusText = false;
            _sendStatusTextMessages();
        }
    }
}

void MockLink::_run10HzTasks(void)
{
    if (_mavlinkStarted && _connected) {
        _sendGpsRawInt();
    }
}

void MockLink::_run50HzTasks(void)
{
    if (_mavlinkStarted && _connected) {
    }
}

void MockLink::_loadParams(void)
{
    QFile paramFile;

    if (_firmwareType == MAV_AUTOPILOT_ARDUPILOTMEGA) {
        if (_vehicleType == MAV_TYPE_FIXED_WING) {
            paramFile.setFileName(":/unittest/APMArduPlaneMockLink.params");
        } else {
            paramFile.setFileName(":/unittest/APMArduCopterMockLink.params");
        }
    } else {
        paramFile.setFileName(":/unittest/PX4MockLink.params");
    }


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

        int componentId = paramData.at(1).toInt();
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
            case MAV_PARAM_TYPE_UINT16:
                paramValue = QVariant((quint16)valStr.toUInt());
                break;
            case MAV_PARAM_TYPE_INT16:
                paramValue = QVariant((qint16)valStr.toInt());
                break;
            case MAV_PARAM_TYPE_UINT8:
                paramValue = QVariant((quint8)valStr.toUInt());
                break;
            case MAV_PARAM_TYPE_INT8:
                paramValue = QVariant((qint8)valStr.toUInt());
                break;
            default:
                qCritical() << "Unknown type" << paramType;
                paramValue = QVariant(valStr.toInt());
                break;
        }

        qCDebug(MockLinkVerboseLog) << "Loading param" << paramName << paramValue;

        _mapParamName2Value[componentId][paramName] = paramValue;
        _mapParamName2MavParamType[paramName] = static_cast<MAV_PARAM_TYPE>(paramType);
    }
}

void MockLink::_sendHeartBeat(void)
{
    mavlink_message_t   msg;

    mavlink_msg_heartbeat_pack(_vehicleSystemId,
                               _vehicleComponentId,
                               &msg,
                               _vehicleType,        // MAV_TYPE
                               _firmwareType,      // MAV_AUTOPILOT
                               _mavBaseMode,        // MAV_MODE
                               _mavCustomMode,      // custom mode
                               _mavState);          // MAV_STATE

    respondWithMavlinkMessage(msg);
}

void MockLink::_sendVibration(void)
{
    mavlink_message_t   msg;

    mavlink_msg_vibration_pack(_vehicleSystemId,
                               _vehicleComponentId,
                               &msg,
                               0,       // time_usec
                               50.5,    // vibration_x,
                               10.5,    // vibration_y,
                               60.0,    // vibration_z,
                               1,       // clipping_0
                               2,       // clipping_0
                               3);      // clipping_0

    respondWithMavlinkMessage(msg);
}

void MockLink::respondWithMavlinkMessage(const mavlink_message_t& msg)
{
    uint8_t buffer[MAVLINK_MAX_PACKET_LEN];

    int cBuffer = mavlink_msg_to_send_buffer(buffer, &msg);
    QByteArray bytes((char *)buffer, cBuffer);
    emit bytesReceived(this, bytes);
}

/// @brief Called when QGC wants to write bytes to the MAV
void MockLink::_writeBytes(const QByteArray bytes)
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

#if 0
        // MockLink not quite ready to handle this correctly yet
        if (strncmp(bytes, "sh /etc/init.d/rc.usb\n", cBytes) == 0) {
            // This is the mavlink start command
            _mavlinkStarted = true;
        }
#endif
    }
}

/// @brief Handle incoming bytes which are meant to be handled by the mavlink protocol
void MockLink::_handleIncomingMavlinkBytes(const uint8_t* bytes, int cBytes)
{
    mavlink_message_t msg;
    mavlink_status_t comm;

    for (qint64 i=0; i<cBytes; i++)
    {
        if (!mavlink_parse_char(getMavlinkChannel(), bytes[i], &msg, &comm)) {
            continue;
        }

        if (_missionItemHandler.handleMessage(msg)) {
            continue;
        }

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

            case MAVLINK_MSG_ID_FILE_TRANSFER_PROTOCOL:
                _handleFTP(msg);
                break;

            case MAVLINK_MSG_ID_COMMAND_LONG:
                _handleCommandLong(msg);
                break;

            case MAVLINK_MSG_ID_MANUAL_CONTROL:
                _handleManualControl(msg);
                break;

            default:
                break;
        }
    }
}

void MockLink::_handleHeartBeat(const mavlink_message_t& msg)
{
    Q_UNUSED(msg);
    qCDebug(MockLinkLog) << "Heartbeat";
}

void MockLink::_handleSetMode(const mavlink_message_t& msg)
{
    mavlink_set_mode_t request;
    mavlink_msg_set_mode_decode(&msg, &request);

    Q_ASSERT(request.target_system == _vehicleSystemId);

    _mavBaseMode = request.base_mode;
    _mavCustomMode = request.custom_mode;
}

void MockLink::_handleManualControl(const mavlink_message_t& msg)
{
    mavlink_manual_control_t manualControl;
    mavlink_msg_manual_control_decode(&msg, &manualControl);

    qDebug() << "MANUAL_CONTROL" << manualControl.x << manualControl.y << manualControl.z << manualControl.r;
}

void MockLink::_setParamFloatUnionIntoMap(int componentId, const QString& paramName, float paramFloat)
{
    mavlink_param_union_t   valueUnion;

    Q_ASSERT(_mapParamName2Value.contains(componentId));
    Q_ASSERT(_mapParamName2Value[componentId].contains(paramName));
    Q_ASSERT(_mapParamName2MavParamType.contains(paramName));

    valueUnion.param_float = paramFloat;

    MAV_PARAM_TYPE paramType = _mapParamName2MavParamType[paramName];

    QVariant paramVariant;

    switch (paramType) {
    case MAV_PARAM_TYPE_REAL32:
        paramVariant = QVariant::fromValue(valueUnion.param_float);
        break;

    case MAV_PARAM_TYPE_UINT32:
        paramVariant = QVariant::fromValue(valueUnion.param_uint32);
        break;

    case MAV_PARAM_TYPE_INT32:
        paramVariant = QVariant::fromValue(valueUnion.param_int32);
        break;

    case MAV_PARAM_TYPE_UINT16:
        paramVariant = QVariant::fromValue(valueUnion.param_uint16);
        break;

    case MAV_PARAM_TYPE_INT16:
        paramVariant = QVariant::fromValue(valueUnion.param_int16);
        break;

    case MAV_PARAM_TYPE_UINT8:
        paramVariant = QVariant::fromValue(valueUnion.param_uint8);
        break;

    case MAV_PARAM_TYPE_INT8:
        paramVariant = QVariant::fromValue(valueUnion.param_int8);
        break;

    default:
        qCritical() << "Invalid parameter type" << paramType;
        paramVariant = QVariant::fromValue(valueUnion.param_int32);
        break;
    }

    qCDebug(MockLinkLog) << "_setParamFloatUnionIntoMap" << paramName << paramVariant;
    _mapParamName2Value[componentId][paramName] = paramVariant;
}

/// Convert from a parameter variant to the float value from mavlink_param_union_t
float MockLink::_floatUnionForParam(int componentId, const QString& paramName)
{
    mavlink_param_union_t   valueUnion;

    Q_ASSERT(_mapParamName2Value.contains(componentId));
    Q_ASSERT(_mapParamName2Value[componentId].contains(paramName));
    Q_ASSERT(_mapParamName2MavParamType.contains(paramName));

    MAV_PARAM_TYPE paramType = _mapParamName2MavParamType[paramName];
    QVariant paramVar = _mapParamName2Value[componentId][paramName];

    switch (paramType) {
    case MAV_PARAM_TYPE_REAL32:
            valueUnion.param_float = paramVar.toFloat();
        break;

    case MAV_PARAM_TYPE_UINT32:
        if (_firmwareType == MAV_AUTOPILOT_ARDUPILOTMEGA) {
            valueUnion.param_float = paramVar.toUInt();
        } else {
            valueUnion.param_uint32 = paramVar.toUInt();
        }
        break;

    case MAV_PARAM_TYPE_INT32:
        if (_firmwareType == MAV_AUTOPILOT_ARDUPILOTMEGA) {
            valueUnion.param_float = paramVar.toInt();
        } else {
            valueUnion.param_int32 = paramVar.toInt();
        }
        break;

    case MAV_PARAM_TYPE_UINT16:
        if (_firmwareType == MAV_AUTOPILOT_ARDUPILOTMEGA) {
            valueUnion.param_float = paramVar.toUInt();
        } else {
            valueUnion.param_uint16 = paramVar.toUInt();
        }
        break;

    case MAV_PARAM_TYPE_INT16:
        if (_firmwareType == MAV_AUTOPILOT_ARDUPILOTMEGA) {
            valueUnion.param_float = paramVar.toInt();
        } else {
            valueUnion.param_int16 = paramVar.toInt();
        }
        break;

    case MAV_PARAM_TYPE_UINT8:
        if (_firmwareType == MAV_AUTOPILOT_ARDUPILOTMEGA) {
            valueUnion.param_float = paramVar.toUInt();
        } else {
            valueUnion.param_uint8 = paramVar.toUInt();
        }
        break;

    case MAV_PARAM_TYPE_INT8:
        if (_firmwareType == MAV_AUTOPILOT_ARDUPILOTMEGA) {
            valueUnion.param_float = (unsigned char)paramVar.toChar().toLatin1();
        } else {
            valueUnion.param_int8 = (unsigned char)paramVar.toChar().toLatin1();
        }
        break;

    default:
        if (_firmwareType == MAV_AUTOPILOT_ARDUPILOTMEGA) {
            valueUnion.param_float = paramVar.toInt();
        } else {
            valueUnion.param_int32 = paramVar.toInt();
        }
        qCritical() << "Invalid parameter type" << paramType;
    }

    return valueUnion.param_float;
}

void MockLink::_handleParamRequestList(const mavlink_message_t& msg)
{
    mavlink_param_request_list_t request;

    mavlink_msg_param_request_list_decode(&msg, &request);

    Q_ASSERT(request.target_system == _vehicleSystemId);
    Q_ASSERT(request.target_component == MAV_COMP_ID_ALL);

    // We must send the first parameter for each component first. Otherwise system won't correctly know
    // when all parameters are loaded.

    foreach (int componentId, _mapParamName2Value.keys()) {
        uint16_t paramIndex = 0;
        int cParameters = _mapParamName2Value[componentId].count();

        foreach(const QString &paramName, _mapParamName2Value[componentId].keys()) {
            char paramId[MAVLINK_MSG_ID_PARAM_VALUE_LEN];
            mavlink_message_t       responseMsg;

            Q_ASSERT(_mapParamName2Value[componentId].contains(paramName));
            Q_ASSERT(_mapParamName2MavParamType.contains(paramName));

            MAV_PARAM_TYPE paramType = _mapParamName2MavParamType[paramName];

            Q_ASSERT(paramName.length() <= MAVLINK_MSG_ID_PARAM_VALUE_LEN);
            strncpy(paramId, paramName.toLocal8Bit().constData(), MAVLINK_MSG_ID_PARAM_VALUE_LEN);

            qCDebug(MockLinkLog) << "Sending msg_param_value" << componentId << paramId << paramType << _mapParamName2Value[componentId][paramId];

            mavlink_msg_param_value_pack(_vehicleSystemId,
                                         componentId,                       // component id
                                         &responseMsg,                      // Outgoing message
                                         paramId,                           // Parameter name
                                         _floatUnionForParam(componentId, paramName),    // Parameter value
                                         paramType,                         // MAV_PARAM_TYPE
                                         cParameters,                       // Total number of parameters
                                         paramIndex++);                     // Index of this parameter
            respondWithMavlinkMessage(responseMsg);

            // Only first parameter the first time through
            break;
        }
    }

    foreach (int componentId, _mapParamName2Value.keys()) {
        uint16_t paramIndex = 0;
        int cParameters = _mapParamName2Value[componentId].count();
        bool skipParam = true;

        foreach(const QString &paramName, _mapParamName2Value[componentId].keys()) {
            if (skipParam) {
                // We've already sent the first param
                skipParam = false;
                paramIndex++;
            } else {
                char paramId[MAVLINK_MSG_ID_PARAM_VALUE_LEN];
                mavlink_message_t       responseMsg;

                Q_ASSERT(_mapParamName2Value[componentId].contains(paramName));
                Q_ASSERT(_mapParamName2MavParamType.contains(paramName));

                MAV_PARAM_TYPE paramType = _mapParamName2MavParamType[paramName];

                Q_ASSERT(paramName.length() <= MAVLINK_MSG_ID_PARAM_VALUE_LEN);
                strncpy(paramId, paramName.toLocal8Bit().constData(), MAVLINK_MSG_ID_PARAM_VALUE_LEN);

                qCDebug(MockLinkLog) << "Sending msg_param_value" << componentId << paramId << paramType << _mapParamName2Value[componentId][paramId];

                mavlink_msg_param_value_pack(_vehicleSystemId,
                                             componentId,                       // component id
                                             &responseMsg,                      // Outgoing message
                                             paramId,                           // Parameter name
                                             _floatUnionForParam(componentId, paramName),    // Parameter value
                                             paramType,                         // MAV_PARAM_TYPE
                                             cParameters,                       // Total number of parameters
                                             paramIndex++);                     // Index of this parameter
                respondWithMavlinkMessage(responseMsg);
            }
        }
    }
}

void MockLink::_handleParamSet(const mavlink_message_t& msg)
{
    mavlink_param_set_t request;
    mavlink_msg_param_set_decode(&msg, &request);

    Q_ASSERT(request.target_system == _vehicleSystemId);
    int componentId = request.target_component;

    // Param may not be null terminated if exactly fits
    char paramId[MAVLINK_MSG_PARAM_SET_FIELD_PARAM_ID_LEN + 1];
    paramId[MAVLINK_MSG_PARAM_SET_FIELD_PARAM_ID_LEN] = 0;
    strncpy(paramId, request.param_id, MAVLINK_MSG_PARAM_SET_FIELD_PARAM_ID_LEN);

    qCDebug(MockLinkLog) << "_handleParamSet" << componentId << paramId << request.param_type;

    Q_ASSERT(_mapParamName2Value.contains(componentId));
    Q_ASSERT(_mapParamName2Value[componentId].contains(paramId));
    Q_ASSERT(request.param_type == _mapParamName2MavParamType[paramId]);

    // Save the new value
    _setParamFloatUnionIntoMap(componentId, paramId, request.param_value);

    // Respond with a param_value to ack
    mavlink_message_t responseMsg;
    mavlink_msg_param_value_pack(_vehicleSystemId,
                                 componentId,                                               // component id
                                 &responseMsg,                                              // Outgoing message
                                 paramId,                                                   // Parameter name
                                 request.param_value,                                       // Send same value back
                                 request.param_type,                                        // Send same type back
                                 _mapParamName2Value[componentId].count(),                  // Total number of parameters
                                 _mapParamName2Value[componentId].keys().indexOf(paramId)); // Index of this parameter
    respondWithMavlinkMessage(responseMsg);
}

void MockLink::_handleParamRequestRead(const mavlink_message_t& msg)
{
    mavlink_message_t   responseMsg;
    mavlink_param_request_read_t request;
    mavlink_msg_param_request_read_decode(&msg, &request);

    const QString param_name(QString::fromLocal8Bit(request.param_id, strnlen(request.param_id, MAVLINK_MSG_PARAM_REQUEST_READ_FIELD_PARAM_ID_LEN)));
    int componentId = request.target_component;

    // special case for magic _HASH_CHECK value
    if (request.target_component == MAV_COMP_ID_ALL && param_name == "_HASH_CHECK") {
        mavlink_param_union_t   valueUnion;
        valueUnion.type = MAV_PARAM_TYPE_UINT32;
        valueUnion.param_uint32 = 0;
        // Special case of magic hash check value
        mavlink_msg_param_value_pack(_vehicleSystemId,
                                     componentId,
                                     &responseMsg,
                                     request.param_id,
                                     valueUnion.param_float,
                                     MAV_PARAM_TYPE_UINT32,
                                     0,
                                     -1);
        respondWithMavlinkMessage(responseMsg);
        return;
    }

    Q_ASSERT(_mapParamName2Value.contains(componentId));

    char paramId[MAVLINK_MSG_PARAM_REQUEST_READ_FIELD_PARAM_ID_LEN + 1];
    paramId[0] = 0;

    Q_ASSERT(request.target_system == _vehicleSystemId);

    if (request.param_index == -1) {
        // Request is by param name. Param may not be null terminated if exactly fits
        strncpy(paramId, request.param_id, MAVLINK_MSG_PARAM_REQUEST_READ_FIELD_PARAM_ID_LEN);
    } else {
        // Request is by index

        Q_ASSERT(request.param_index >= 0 && request.param_index < _mapParamName2Value[componentId].count());

        QString key = _mapParamName2Value[componentId].keys().at(request.param_index);
        Q_ASSERT(key.length() <= MAVLINK_MSG_PARAM_REQUEST_READ_FIELD_PARAM_ID_LEN);
        strcpy(paramId, key.toLocal8Bit().constData());
    }

    Q_ASSERT(_mapParamName2Value[componentId].contains(paramId));
    Q_ASSERT(_mapParamName2MavParamType.contains(paramId));

    mavlink_msg_param_value_pack(_vehicleSystemId,
                                 componentId,                                               // component id
                                 &responseMsg,                                              // Outgoing message
                                 paramId,                                                   // Parameter name
                                 _floatUnionForParam(componentId, paramId),                 // Parameter value
                                 _mapParamName2MavParamType[paramId],                       // Parameter type
                                 _mapParamName2Value[componentId].count(),                  // Total number of parameters
                                 _mapParamName2Value[componentId].keys().indexOf(paramId)); // Index of this parameter
    respondWithMavlinkMessage(responseMsg);
}

void MockLink::emitRemoteControlChannelRawChanged(int channel, uint16_t raw)
{
    uint16_t chanRaw[18];

    for (int i=0; i<18; i++) {
        chanRaw[i] = UINT16_MAX;
    }
    chanRaw[channel] = raw;

    mavlink_message_t responseMsg;
    mavlink_msg_rc_channels_pack(_vehicleSystemId,
                                 _vehicleComponentId,
                                 &responseMsg,          // Outgoing message
                                 0,                     // time since boot, ignored
                                 18,                    // channel count
                                 chanRaw[0],            // channel raw value
                                 chanRaw[1],            // channel raw value
                                 chanRaw[2],            // channel raw value
                                 chanRaw[3],            // channel raw value
                                 chanRaw[4],            // channel raw value
                                 chanRaw[5],            // channel raw value
                                 chanRaw[6],            // channel raw value
                                 chanRaw[7],            // channel raw value
                                 chanRaw[8],            // channel raw value
                                 chanRaw[9],            // channel raw value
                                 chanRaw[10],           // channel raw value
                                 chanRaw[11],           // channel raw value
                                 chanRaw[12],           // channel raw value
                                 chanRaw[13],           // channel raw value
                                 chanRaw[14],           // channel raw value
                                 chanRaw[15],           // channel raw value
                                 chanRaw[16],           // channel raw value
                                 chanRaw[17],           // channel raw value
                                 0);                    // rss
    respondWithMavlinkMessage(responseMsg);
}

void MockLink::_handleFTP(const mavlink_message_t& msg)
{
    Q_ASSERT(_fileServer);
    _fileServer->handleFTPMessage(msg);
}

void MockLink::_handleCommandLong(const mavlink_message_t& msg)
{
    mavlink_command_long_t request;
    uint8_t commandResult = MAV_RESULT_UNSUPPORTED;

    mavlink_msg_command_long_decode(&msg, &request);

    switch (request.command) {
    case MAV_CMD_COMPONENT_ARM_DISARM:
        if (request.param1 == 0.0f) {
            _mavBaseMode &= ~MAV_MODE_FLAG_SAFETY_ARMED;
        } else {
            _mavBaseMode |= MAV_MODE_FLAG_SAFETY_ARMED;
        }
        commandResult = MAV_RESULT_ACCEPTED;
        break;
    case MAV_CMD_PREFLIGHT_CALIBRATION:
    case MAV_CMD_PREFLIGHT_STORAGE:
        commandResult = MAV_RESULT_ACCEPTED;
        break;
    case MAV_CMD_REQUEST_AUTOPILOT_CAPABILITIES:
        commandResult = MAV_RESULT_ACCEPTED;
        _respondWithAutopilotVersion();
        break;
    }

    mavlink_message_t commandAck;
    mavlink_msg_command_ack_pack(_vehicleSystemId,
                                 _vehicleComponentId,
                                 &commandAck,
                                 request.command,
                                 commandResult);
    respondWithMavlinkMessage(commandAck);
}

void MockLink::_respondWithAutopilotVersion(void)
{
    mavlink_message_t msg;

    uint8_t customVersion[8];
    memset(customVersion, 0, sizeof(customVersion));

    // Only flight_sw_version is supported a this point
    mavlink_msg_autopilot_version_pack(_vehicleSystemId,
                                       _vehicleComponentId,
                                       &msg,
                                       0,                                           // capabilities,
                                       (1 << (8*3)) | FIRMWARE_VERSION_TYPE_DEV,    // flight_sw_version,
                                       0,                                           // middleware_sw_version,
                                       0,                                           // os_sw_version,
                                       0,                                           // board_version,
                                       (uint8_t *)&customVersion,                   // flight_custom_version,
                                       (uint8_t *)&customVersion,                   // middleware_custom_version,
                                       (uint8_t *)&customVersion,                   // os_custom_version,
                                       0,                                           // vendor_id,
                                       0,                                           // product_id,
                                       0);                                          // uid
    respondWithMavlinkMessage(msg);
}

void MockLink::setMissionItemFailureMode(MockLinkMissionItemHandler::FailureMode_t failureMode)
{
    _missionItemHandler.setMissionItemFailureMode(failureMode);
}

void MockLink::_sendHomePosition(void)
{
    mavlink_message_t msg;

    float bogus[4];
    bogus[0] = 0.0f;
    bogus[1] = 0.0f;
    bogus[2] = 0.0f;
    bogus[3] = 0.0f;

    mavlink_msg_home_position_pack(_vehicleSystemId,
                                   _vehicleComponentId,
                                   &msg,
                                   (int32_t)(_vehicleLatitude * 1E7),
                                   (int32_t)(_vehicleLongitude * 1E7),
                                   (int32_t)(_vehicleAltitude * 1000),
                                   0.0f, 0.0f, 0.0f,
                                   &bogus[0],
            0.0f, 0.0f, 0.0f);
    respondWithMavlinkMessage(msg);
}

void MockLink::_sendGpsRawInt(void)
{
    static uint64_t timeTick = 0;
    mavlink_message_t msg;

    mavlink_msg_gps_raw_int_pack(_vehicleSystemId,
                                 _vehicleComponentId,
                                 &msg,
                                 timeTick++,                            // time since boot
                                 3,                                     // 3D fix
                                 (int32_t)(_vehicleLatitude  * 1E7),
                                 (int32_t)(_vehicleLongitude * 1E7),
                                 (int32_t)(_vehicleAltitude  * 1000),
                                 UINT16_MAX, UINT16_MAX,                // HDOP/VDOP not known
                                 UINT16_MAX,                            // velocity not known
                                 UINT16_MAX,                            // course over ground not known
                                 8);                                    // satellite count
    respondWithMavlinkMessage(msg);
}

void MockLink::_sendStatusTextMessages(void)
{
    struct StatusMessage {
        MAV_SEVERITY        severity;
        const char*         msg;
    };

    static const struct StatusMessage rgMessages[] = {
        { MAV_SEVERITY_INFO,        "#Testing audio output" },
        { MAV_SEVERITY_EMERGENCY,   "Status text emergency" },
        { MAV_SEVERITY_ALERT,       "Status text alert" },
        { MAV_SEVERITY_CRITICAL,    "Status text critical" },
        { MAV_SEVERITY_ERROR,       "Status text error" },
        { MAV_SEVERITY_WARNING,     "Status text warning" },
        { MAV_SEVERITY_NOTICE,      "Status text notice" },
        { MAV_SEVERITY_INFO,        "Status text info" },
        { MAV_SEVERITY_DEBUG,       "Status text debug" },
    };

    for (size_t i=0; i<sizeof(rgMessages)/sizeof(rgMessages[0]); i++) {
        mavlink_message_t msg;
        const struct StatusMessage* status = &rgMessages[i];

        mavlink_msg_statustext_pack(_vehicleSystemId,
                                    _vehicleComponentId,
                                    &msg,
                                    status->severity,
                                    status->msg);
        respondWithMavlinkMessage(msg);
    }
}

MockConfiguration::MockConfiguration(const QString& name)
    : LinkConfiguration(name)
    , _firmwareType(MAV_AUTOPILOT_PX4)
    , _vehicleType(MAV_TYPE_QUADROTOR)
    , _sendStatusText(false)
{

}

MockConfiguration::MockConfiguration(MockConfiguration* source)
    : LinkConfiguration(source)
{
    _firmwareType =     source->_firmwareType;
    _vehicleType =      source->_vehicleType;
    _sendStatusText =   source->_sendStatusText;
}

void MockConfiguration::copyFrom(LinkConfiguration *source)
{
    LinkConfiguration::copyFrom(source);
    MockConfiguration* usource = dynamic_cast<MockConfiguration*>(source);

    if (!usource) {
        qWarning() << "dynamic_cast failed" << source << usource;
        return;
    }

    _firmwareType =     usource->_firmwareType;
    _vehicleType =      usource->_vehicleType;
    _sendStatusText =   usource->_sendStatusText;
}

void MockConfiguration::saveSettings(QSettings& settings, const QString& root)
{
    settings.beginGroup(root);
    settings.setValue(_firmwareTypeKey, (int)_firmwareType);
    settings.setValue(_vehicleTypeKey, (int)_vehicleType);
    settings.setValue(_sendStatusTextKey, _sendStatusText);
    settings.sync();
    settings.endGroup();
}

void MockConfiguration::loadSettings(QSettings& settings, const QString& root)
{
    settings.beginGroup(root);
    _firmwareType = (MAV_AUTOPILOT)settings.value(_firmwareTypeKey, (int)MAV_AUTOPILOT_PX4).toInt();
    _vehicleType = (MAV_TYPE)settings.value(_vehicleTypeKey, (int)MAV_TYPE_QUADROTOR).toInt();
    _sendStatusText = settings.value(_sendStatusTextKey, false).toBool();
    settings.endGroup();
}

void MockConfiguration::updateSettings()
{
    if (_link) {
        MockLink* ulink = dynamic_cast<MockLink*>(_link);
        if (ulink) {
            // Restart connect not supported
            qWarning() << "updateSettings not supported";
            //ulink->_restartConnection();
        }
    }
}

MockLink*  MockLink::_startMockLink(MockConfiguration* mockConfig)
{
    LinkManager* linkManager = qgcApp()->toolbox()->linkManager();

    mockConfig->setDynamic(true);
    linkManager->linkConfigurations()->append(mockConfig);

    return qobject_cast<MockLink*>(linkManager->createConnectedLink(mockConfig));
}

MockLink*  MockLink::startPX4MockLink(bool sendStatusText)
{
    MockConfiguration* mockConfig = new MockConfiguration("PX4 MockLink");

    mockConfig->setFirmwareType(MAV_AUTOPILOT_PX4);
    mockConfig->setVehicleType(MAV_TYPE_QUADROTOR);
    mockConfig->setSendStatusText(sendStatusText);

    return _startMockLink(mockConfig);
}

MockLink*  MockLink::startGenericMockLink(bool sendStatusText)
{
    MockConfiguration* mockConfig = new MockConfiguration("Generic MockLink");

    mockConfig->setFirmwareType(MAV_AUTOPILOT_GENERIC);
    mockConfig->setVehicleType(MAV_TYPE_QUADROTOR);
    mockConfig->setSendStatusText(sendStatusText);

    return _startMockLink(mockConfig);
}

MockLink*  MockLink::startAPMArduCopterMockLink(bool sendStatusText)
{
    MockConfiguration* mockConfig = new MockConfiguration("APM ArduCopter MockLink");

    mockConfig->setFirmwareType(MAV_AUTOPILOT_ARDUPILOTMEGA);
    mockConfig->setVehicleType(MAV_TYPE_QUADROTOR);
    mockConfig->setSendStatusText(sendStatusText);

    return _startMockLink(mockConfig);
}

MockLink*  MockLink::startAPMArduPlaneMockLink(bool sendStatusText)
{
    MockConfiguration* mockConfig = new MockConfiguration("APM ArduPlane MockLink");

    mockConfig->setFirmwareType(MAV_AUTOPILOT_ARDUPILOTMEGA);
    mockConfig->setVehicleType(MAV_TYPE_FIXED_WING);
    mockConfig->setSendStatusText(sendStatusText);

    return _startMockLink(mockConfig);
}

