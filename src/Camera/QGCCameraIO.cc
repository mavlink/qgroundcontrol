/*!
 * @file
 *   @brief Camera Controller
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

#include "QGCCameraControl.h"
#include "QGCCameraIO.h"

QGC_LOGGING_CATEGORY(CameraIOLog, "CameraIOLog")
QGC_LOGGING_CATEGORY(CameraIOLogVerbose, "CameraIOLogVerbose")

//-----------------------------------------------------------------------------
QGCCameraParamIO::QGCCameraParamIO(QGCCameraControl *control, Fact* fact, Vehicle *vehicle)
    : QObject(control)
    , _control(control)
    , _fact(fact)
    , _vehicle(vehicle)
    , _sentRetries(0)
    , _requestRetries(0)
    , _done(false)
{
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    _paramWriteTimer.setSingleShot(true);
    _paramWriteTimer.setInterval(1000);
    _paramRequestTimer.setSingleShot(true);
    _paramRequestTimer.setInterval(2000);
    connect(&_paramWriteTimer,   &QTimer::timeout, this, &QGCCameraParamIO::_paramWriteTimeout);
    connect(&_paramRequestTimer, &QTimer::timeout, this, &QGCCameraParamIO::_paramRequestTimeout);
    connect(_fact, &Fact::rawValueChanged, this, &QGCCameraParamIO::_factChanged);
    connect(_fact, &Fact::_containerRawValueChanged, this, &QGCCameraParamIO::_containerRawValueChanged);
    _pMavlink = qgcApp()->toolbox()->mavlinkProtocol();
    //-- TODO: Even though we don't use anything larger than 32-bit, this should
    //   probably be updated.
    switch (_fact->type()) {
        case FactMetaData::valueTypeUint8:
            _mavParamType = MAV_PARAM_TYPE_UINT8;
            break;
        case FactMetaData::valueTypeInt8:
            _mavParamType = MAV_PARAM_TYPE_INT8;
            break;
        case FactMetaData::valueTypeUint16:
            _mavParamType = MAV_PARAM_TYPE_UINT16;
            break;
        case FactMetaData::valueTypeInt16:
            _mavParamType = MAV_PARAM_TYPE_INT16;
            break;
        case FactMetaData::valueTypeUint32:
            _mavParamType = MAV_PARAM_TYPE_UINT32;
            break;
        case FactMetaData::valueTypeFloat:
            _mavParamType = MAV_PARAM_TYPE_REAL32;
            break;
        default:
            qWarning() << "Unsupported fact type" << _fact->type();
            // Fall through
        case FactMetaData::valueTypeInt32:
            _mavParamType = MAV_PARAM_TYPE_INT32;
            break;
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraParamIO::setParamRequest()
{
    _paramRequestReceived = false;
    _requestRetries = 0;
    _paramRequestTimer.start();
}

//-----------------------------------------------------------------------------
void
QGCCameraParamIO::_factChanged(QVariant value)
{
    Q_UNUSED(value);
    qCDebug(CameraControlLog) << "UI Fact" << _fact->name() << "changed";
    //-- TODO: Do we really want to update the UI now or only when we receive the ACK?
    _control->factChanged(_fact);
}

//-----------------------------------------------------------------------------
void
QGCCameraParamIO::_containerRawValueChanged(const QVariant value)
{
    Q_UNUSED(value);
    qCDebug(CameraControlLog) << "Send Fact" << _fact->name();
    _sentRetries = 0;
    _sendParameter();
}

//-----------------------------------------------------------------------------
void
QGCCameraParamIO::_sendParameter()
{
    //-- TODO: We should use something other than mavlink_param_union_t for PARAM_EXT_SET
    mavlink_param_ext_set_t p;
    memset(&p, 0, sizeof(mavlink_param_ext_set_t));
    mavlink_param_union_t       union_value;
    mavlink_message_t           msg;
    FactMetaData::ValueType_t factType = _fact->type();
    p.param_type = _mavParamType;
    switch (factType) {
        case FactMetaData::valueTypeUint8:
            union_value.param_uint8 = (uint8_t)_fact->rawValue().toUInt();
            break;
        case FactMetaData::valueTypeInt8:
            union_value.param_int8 = (int8_t)_fact->rawValue().toInt();
            break;
        case FactMetaData::valueTypeUint16:
            union_value.param_uint16 = (uint16_t)_fact->rawValue().toUInt();
            break;
        case FactMetaData::valueTypeInt16:
            union_value.param_int16 = (int16_t)_fact->rawValue().toInt();
            break;
        case FactMetaData::valueTypeUint32:
            union_value.param_uint32 = (uint32_t)_fact->rawValue().toUInt();
            break;
        case FactMetaData::valueTypeFloat:
            union_value.param_float = _fact->rawValue().toFloat();
            break;
        default:
            qCritical() << "Unsupported fact type" << factType;
            // fall through
        case FactMetaData::valueTypeInt32:
            union_value.param_int32 = (int32_t)_fact->rawValue().toInt();
            break;
    }
    memcpy(&p.param_value[0], &union_value.param_float, sizeof(float));
    p.target_system = (uint8_t)_vehicle->id();
    p.target_component = (uint8_t)_control->compID();
    strncpy(p.param_id, _fact->name().toStdString().c_str(), MAVLINK_MSG_PARAM_EXT_SET_FIELD_PARAM_ID_LEN);
    mavlink_msg_param_ext_set_encode_chan(
        _pMavlink->getSystemId(),
        _pMavlink->getComponentId(),
        _vehicle->priorityLink()->mavlinkChannel(),
        &msg,
        &p);
    _vehicle->sendMessageOnLink(_vehicle->priorityLink(), msg);
    _paramWriteTimer.start();
}

//-----------------------------------------------------------------------------
void
QGCCameraParamIO::_paramWriteTimeout()
{
    if(++_sentRetries > 3) {
        qCWarning(CameraIOLog) << "No response for param set:" << _fact->name();
    } else {
        //-- Send it again
        qCDebug(CameraIOLog) << "Param set retry:" << _fact->name();
        _sendParameter();
        _paramWriteTimer.start();
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraParamIO::handleParamAck(const mavlink_param_ext_ack_t& ack)
{
    _paramWriteTimer.stop();
    if(ack.param_result == PARAM_ACK_ACCEPTED) {
        QVariant val = _valueFromMessage(ack.param_value, ack.param_type);
        if(_fact->rawValue() != val) {
            _fact->_containerSetRawValue(val);
        }
    } else if(ack.param_result == PARAM_ACK_IN_PROGRESS) {
        //-- Wait a bit longer for this one
        qCDebug(CameraIOLogVerbose) << "Param set in progress:" << _fact->name();
        _paramWriteTimer.start();
    } else {
        if(ack.param_result == PARAM_ACK_FAILED) {
            qCWarning(CameraIOLog) << "Param set failed:" << _fact->name();
        } else if(ack.param_result == PARAM_ACK_VALUE_UNSUPPORTED) {
            qCWarning(CameraIOLog) << "Param set unsuported:" << _fact->name();
        }
        //-- If UI changed and value was not set, restore UI
        QVariant val = _valueFromMessage(ack.param_value, ack.param_type);
        if(_fact->rawValue() != val) {
            _fact->_containerSetRawValue(val);
        }
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraParamIO::handleParamValue(const mavlink_param_ext_value_t& value)
{
    _paramRequestTimer.stop();
    _fact->_containerSetRawValue(_valueFromMessage(value.param_value, value.param_type));
    _paramRequestReceived = true;
    if(!_done) {
        _done = true;
        _control->_paramDone();
    }
    qCDebug(CameraIOLog) << QString("handleParamValue() %1 %2").arg(_fact->name()).arg(_fact->rawValueString());
}

//-----------------------------------------------------------------------------
QVariant
QGCCameraParamIO::_valueFromMessage(const char* value, uint8_t param_type)
{
    //-- TODO: Even though we don't use anything larger than 32-bit, this should
    //   probably be updated.
    QVariant var;
    mavlink_param_union_t u;
    memcpy(&u.param_float, value, sizeof(float));
    switch (param_type) {
        case MAV_PARAM_TYPE_REAL32:
            var = QVariant(u.param_float);
            break;
        case MAV_PARAM_TYPE_UINT8:
            var = QVariant(u.param_uint8);
            break;
        case MAV_PARAM_TYPE_INT8:
            var = QVariant(u.param_int8);
            break;
        case MAV_PARAM_TYPE_UINT16:
            var = QVariant(u.param_uint16);
            break;
        case MAV_PARAM_TYPE_INT16:
            var = QVariant(u.param_int16);
            break;
        case MAV_PARAM_TYPE_UINT32:
            var = QVariant(u.param_uint32);
            break;
        case MAV_PARAM_TYPE_INT32:
            var = QVariant(u.param_int32);
            break;
        default:
            var = QVariant(0);
            qCritical() << "Invalid param_type used for camera setting:" << param_type;
    }
    return var;
}

//-----------------------------------------------------------------------------
void
QGCCameraParamIO::_paramRequestTimeout()
{
    if(++_requestRetries > 3) {
        qCWarning(CameraIOLog) << "No response for param request:" << _fact->name();
        if(!_done) {
            _done = true;
            _control->_paramDone();
        }
    } else {
        //-- Request it again
        qCDebug(CameraIOLog) << "Param request retry:" << _fact->name();
        char param_id[MAVLINK_MSG_PARAM_EXT_REQUEST_READ_FIELD_PARAM_ID_LEN + 1];
        memset(param_id, 0, sizeof(param_id));
        strncpy(param_id, _fact->name().toStdString().c_str(), MAVLINK_MSG_PARAM_EXT_REQUEST_READ_FIELD_PARAM_ID_LEN);
        mavlink_message_t msg;
        mavlink_msg_param_ext_request_read_pack_chan(
            _pMavlink->getSystemId(),
            _pMavlink->getComponentId(),
            _vehicle->priorityLink()->mavlinkChannel(),
            &msg,
            _vehicle->id(),
            _control->compID(),
            param_id,
            -1);
        _vehicle->sendMessageOnLink(_vehicle->priorityLink(), msg);
        _paramRequestTimer.start();
    }
}
