/*!
 * @file
 *   @brief Camera Controller
 *   @author Gus Grubba <gus@auterion.com>
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
    , _updateOnSet(false)
    , _forceUIUpdate(false)
{
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    _paramWriteTimer.setSingleShot(true);
    _paramWriteTimer.setInterval(3000);
    _paramRequestTimer.setSingleShot(true);
    _paramRequestTimer.setInterval(3500);
    if(_fact->writeOnly()) {
        //-- Write mode is always "done" as it won't ever read
        _done = true;
    } else {
        connect(&_paramRequestTimer, &QTimer::timeout, this, &QGCCameraParamIO::_paramRequestTimeout);
    }
    connect(&_paramWriteTimer,   &QTimer::timeout, this, &QGCCameraParamIO::_paramWriteTimeout);
    connect(_fact, &Fact::rawValueChanged, this, &QGCCameraParamIO::_factChanged);
    connect(_fact, &Fact::_containerRawValueChanged, this, &QGCCameraParamIO::_containerRawValueChanged);
    _pMavlink = qgcApp()->toolbox()->mavlinkProtocol();
    //-- TODO: Even though we don't use anything larger than 32-bit, this should
    //   probably be updated.
    switch (_fact->type()) {
        case FactMetaData::valueTypeUint8:
        case FactMetaData::valueTypeBool:
            _mavParamType = MAV_PARAM_EXT_TYPE_UINT8;
            break;
        case FactMetaData::valueTypeInt8:
            _mavParamType = MAV_PARAM_EXT_TYPE_INT8;
            break;
        case FactMetaData::valueTypeUint16:
            _mavParamType = MAV_PARAM_EXT_TYPE_UINT16;
            break;
        case FactMetaData::valueTypeInt16:
            _mavParamType = MAV_PARAM_EXT_TYPE_INT16;
            break;
        case FactMetaData::valueTypeUint32:
            _mavParamType = MAV_PARAM_EXT_TYPE_UINT32;
            break;
        case FactMetaData::valueTypeUint64:
            _mavParamType = MAV_PARAM_EXT_TYPE_UINT64;
            break;
        case FactMetaData::valueTypeInt64:
            _mavParamType = MAV_PARAM_EXT_TYPE_INT64;
            break;
        case FactMetaData::valueTypeFloat:
            _mavParamType = MAV_PARAM_EXT_TYPE_REAL32;
            break;
        case FactMetaData::valueTypeDouble:
            _mavParamType = MAV_PARAM_EXT_TYPE_REAL64;
            break;
            //-- String and custom are the same for now
        case FactMetaData::valueTypeString:
        case FactMetaData::valueTypeCustom:
            _mavParamType = MAV_PARAM_EXT_TYPE_CUSTOM;
            break;
        default:
            qWarning() << "Unsupported fact type" << _fact->type() << "for" << _fact->name();
            //-- Fall Through (screw clang)
        case FactMetaData::valueTypeInt32:
            _mavParamType = MAV_PARAM_EXT_TYPE_INT32;
            break;
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraParamIO::setParamRequest()
{
    if(!_fact->writeOnly()) {
        _paramRequestReceived = false;
        _requestRetries = 0;
        _paramRequestTimer.start();
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraParamIO::_factChanged(QVariant value)
{
    if(!_forceUIUpdate) {
        Q_UNUSED(value);
        qCDebug(CameraIOLog) << "UI Fact" << _fact->name() << "changed to" << value;
        _control->factChanged(_fact);
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraParamIO::_containerRawValueChanged(const QVariant value)
{
    if(!_fact->readOnly()) {
        Q_UNUSED(value);
        qCDebug(CameraIOLog) << "Update Fact from camera" << _fact->name();
        _sentRetries = 0;
        _sendParameter();
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraParamIO::sendParameter(bool updateUI)
{
    qCDebug(CameraIOLog) << "Send Fact" << _fact->name();
    _sentRetries = 0;
    _updateOnSet = updateUI;
    _sendParameter();
}

//-----------------------------------------------------------------------------
void
QGCCameraParamIO::_sendParameter()
{
    mavlink_param_ext_set_t p;
    memset(&p, 0, sizeof(mavlink_param_ext_set_t));
    param_ext_union_t   union_value;
    mavlink_message_t   msg;
    FactMetaData::ValueType_t factType = _fact->type();
    p.param_type = _mavParamType;
    switch (factType) {
        case FactMetaData::valueTypeUint8:
        case FactMetaData::valueTypeBool:
            union_value.param_uint8 = static_cast<uint8_t>(_fact->rawValue().toUInt());
            break;
        case FactMetaData::valueTypeInt8:
            union_value.param_int8 = static_cast<int8_t>(_fact->rawValue().toInt());
            break;
        case FactMetaData::valueTypeUint16:
            union_value.param_uint16 = static_cast<uint16_t>(_fact->rawValue().toUInt());
            break;
        case FactMetaData::valueTypeInt16:
            union_value.param_int16 = static_cast<int16_t>(_fact->rawValue().toInt());
            break;
        case FactMetaData::valueTypeUint32:
            union_value.param_uint32 = static_cast<uint32_t>(_fact->rawValue().toUInt());
            break;
        case FactMetaData::valueTypeInt64:
            union_value.param_int64 = static_cast<int64_t>(_fact->rawValue().toLongLong());
            break;
        case FactMetaData::valueTypeUint64:
            union_value.param_uint64 = static_cast<uint64_t>(_fact->rawValue().toULongLong());
            break;
        case FactMetaData::valueTypeFloat:
            union_value.param_float = _fact->rawValue().toFloat();
            break;
        case FactMetaData::valueTypeDouble:
            union_value.param_double = _fact->rawValue().toDouble();
            break;
            //-- String and custom are the same for now
        case FactMetaData::valueTypeString:
        case FactMetaData::valueTypeCustom:
            {
                QByteArray custom = _fact->rawValue().toByteArray();
                memcpy(union_value.bytes, custom.data(), static_cast<size_t>(std::max(custom.size(), MAVLINK_MSG_PARAM_EXT_SET_FIELD_PARAM_VALUE_LEN)));
            }
            break;
        default:
            qCritical() << "Unsupported fact type" << factType << "for" << _fact->name();
            //-- Fall Through (screw clang)
        case FactMetaData::valueTypeInt32:
            union_value.param_int32 = static_cast<int32_t>(_fact->rawValue().toInt());
            break;
    }
    memcpy(&p.param_value[0], &union_value.bytes[0], MAVLINK_MSG_PARAM_EXT_SET_FIELD_PARAM_VALUE_LEN);
    p.target_system     = static_cast<uint8_t>(_vehicle->id());
    p.target_component  = static_cast<uint8_t>(_control->compID());
    strncpy(p.param_id, _fact->name().toStdString().c_str(), MAVLINK_MSG_PARAM_EXT_SET_FIELD_PARAM_ID_LEN);
    mavlink_msg_param_ext_set_encode_chan(
        static_cast<uint8_t>(_pMavlink->getSystemId()),
        static_cast<uint8_t>(_pMavlink->getComponentId()),
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
        _updateOnSet = false;
    } else {
        //-- Send it again
        qCDebug(CameraIOLog) << "Param set retry:" << _fact->name() << _sentRetries;
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
            if(_updateOnSet) {
                _updateOnSet = false;
                _control->factChanged(_fact);
            }
        }
    } else if(ack.param_result == PARAM_ACK_IN_PROGRESS) {
        //-- Wait a bit longer for this one
        qCDebug(CameraIOLogVerbose) << "Param set in progress:" << _fact->name();
        _paramWriteTimer.start();
    } else {
        if(ack.param_result == PARAM_ACK_FAILED) {
            if(++_sentRetries < 3) {
                //-- Try again
                qCWarning(CameraIOLog) << "Param set failed:" << _fact->name() << _sentRetries;
                _paramWriteTimer.start();
            }
            return;
        } else if(ack.param_result == PARAM_ACK_VALUE_UNSUPPORTED) {
            qCWarning(CameraIOLog) << "Param set unsuported:" << _fact->name();
        }
        //-- If UI changed and value was not set, restore UI
        QVariant val = _valueFromMessage(ack.param_value, ack.param_type);
        if(_fact->rawValue() != val) {
            if(_control->validateParameter(_fact, val)) {
                _fact->_containerSetRawValue(val);
            }
        }
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraParamIO::handleParamValue(const mavlink_param_ext_value_t& value)
{
    _paramRequestTimer.stop();
    QVariant newValue = _valueFromMessage(value.param_value, value.param_type);
    if(_control->incomingParameter(_fact, newValue)) {
        _fact->_containerSetRawValue(newValue);
    }
    _paramRequestReceived = true;
    if(_forceUIUpdate) {
        emit _fact->rawValueChanged(_fact->rawValue());
        emit _fact->valueChanged(_fact->rawValue());
        _forceUIUpdate = false;
    }
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
    QVariant var;
    param_ext_union_t u;
    memcpy(u.bytes, value, MAVLINK_MSG_PARAM_EXT_SET_FIELD_PARAM_VALUE_LEN);
    switch (param_type) {
        case MAV_PARAM_EXT_TYPE_REAL32:
            var = QVariant(u.param_float);
            break;
        case MAV_PARAM_EXT_TYPE_UINT8:
            var = QVariant(u.param_uint8);
            break;
        case MAV_PARAM_EXT_TYPE_INT8:
            var = QVariant(u.param_int8);
            break;
        case MAV_PARAM_EXT_TYPE_UINT16:
            var = QVariant(u.param_uint16);
            break;
        case MAV_PARAM_EXT_TYPE_INT16:
            var = QVariant(u.param_int16);
            break;
        case MAV_PARAM_EXT_TYPE_UINT32:
            var = QVariant(u.param_uint32);
            break;
        case MAV_PARAM_EXT_TYPE_INT32:
            var = QVariant(u.param_int32);
            break;
        case MAV_PARAM_EXT_TYPE_UINT64:
            var = QVariant(static_cast<qulonglong>(u.param_uint64));
            break;
        case MAV_PARAM_EXT_TYPE_INT64:
            var = QVariant(static_cast<qulonglong>(u.param_int64));
            break;
        case MAV_PARAM_EXT_TYPE_CUSTOM:
            var = QVariant(QByteArray(value, MAVLINK_MSG_PARAM_EXT_SET_FIELD_PARAM_VALUE_LEN));
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
        paramRequest(false);
        _paramRequestTimer.start();
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraParamIO::paramRequest(bool reset)
{
    //-- If it's write only, we don't request it.
    if(_fact->writeOnly()) {
        if(!_done) {
            _done = true;
            _control->_paramDone();
        }
        return;
    }
    if(reset) {
        _requestRetries = 0;
        _forceUIUpdate  = true;
    }
    qCDebug(CameraIOLog) << "Request parameter:" << _fact->name();
    char param_id[MAVLINK_MSG_PARAM_EXT_REQUEST_READ_FIELD_PARAM_ID_LEN + 1];
    memset(param_id, 0, sizeof(param_id));
    strncpy(param_id, _fact->name().toStdString().c_str(), MAVLINK_MSG_PARAM_EXT_REQUEST_READ_FIELD_PARAM_ID_LEN);
    mavlink_message_t msg;
    mavlink_msg_param_ext_request_read_pack_chan(
        static_cast<uint8_t>(_pMavlink->getSystemId()),
        static_cast<uint8_t>(_pMavlink->getComponentId()),
        _vehicle->priorityLink()->mavlinkChannel(),
        &msg,
        static_cast<uint8_t>(_vehicle->id()),
        static_cast<uint8_t>(_control->compID()),
        param_id,
        -1);
    _vehicle->sendMessageOnLink(_vehicle->priorityLink(), msg);
    _paramRequestTimer.start();
}
