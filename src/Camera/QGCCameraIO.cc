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
    _paramWriteTimer.setSingleShot(true);
    _paramWriteTimer.setInterval(3000);
    _paramRequestTimer.setSingleShot(true);
    _paramRequestTimer.setInterval(3000);
    connect(&_paramWriteTimer,   &QTimer::timeout, this, &QGCCameraParamIO::_paramWriteTimeout);
    connect(&_paramRequestTimer, &QTimer::timeout, this, &QGCCameraParamIO::_paramRequestTimeout);
    connect(_fact, &Fact::rawValueChanged, this, &QGCCameraParamIO::_factChanged);
    connect(_fact, &Fact::_containerRawValueChanged, this, &QGCCameraParamIO::_containerRawValueChanged);
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
    qCDebug(CameraIOLog) << "UI Fact" << _fact->name() << "changed";
    //-- TODO: Do we really want to update the UI now or only when we receive the ACK?
    _control->factChanged(_fact);
}

//-----------------------------------------------------------------------------
void
QGCCameraParamIO::_containerRawValueChanged(const QVariant value)
{
    Q_UNUSED(value);
    qCDebug(CameraIOLog) << "Send Fact" << _fact->name() << "changed";
    _sendParameter();
}

//-----------------------------------------------------------------------------
void
QGCCameraParamIO::_sendParameter()
{
    //-- TODO: We should use something other than mavlink_param_union_t for PARAM_EXT_SET
    mavlink_param_ext_set_t     p;
    mavlink_param_union_t       union_value;
    memset(&p, 0, sizeof(p));
    FactMetaData::ValueType_t factType = _fact->type();
    p.param_type = _factTypeToMavType(factType);
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
    memcpy(&p.param_value, &union_value.param_float, sizeof(float));
    p.target_system = (uint8_t)_vehicle->id();
    p.target_component = (uint8_t)_control->compID();
    strncpy(p.param_id, _fact->name().toStdString().c_str(), sizeof(p.param_id));
    MAVLinkProtocol* pMavlink = qgcApp()->toolbox()->mavlinkProtocol();
    mavlink_msg_param_ext_set_encode_chan(pMavlink->getSystemId(),
                                      pMavlink->getComponentId(),
                                      _vehicle->priorityLink()->mavlinkChannel(),
                                      &_msg,
                                      &p);
    _vehicle->sendMessageOnLink(_vehicle->priorityLink(), _msg);
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
        _vehicle->sendMessageOnLink(_vehicle->priorityLink(), _msg);
        _paramWriteTimer.start();
    }
}

//-----------------------------------------------------------------------------
MAV_PARAM_TYPE
QGCCameraParamIO::_factTypeToMavType(FactMetaData::ValueType_t factType)
{
    //-- TODO: Even though we don't use anything larger than 32-bit, this should
    //   probably be updated.
    switch (factType) {
        case FactMetaData::valueTypeUint8:
            return MAV_PARAM_TYPE_UINT8;
        case FactMetaData::valueTypeInt8:
            return MAV_PARAM_TYPE_INT8;
        case FactMetaData::valueTypeUint16:
            return MAV_PARAM_TYPE_UINT16;
        case FactMetaData::valueTypeInt16:
            return MAV_PARAM_TYPE_INT16;
        case FactMetaData::valueTypeUint32:
            return MAV_PARAM_TYPE_UINT32;
        case FactMetaData::valueTypeFloat:
            return MAV_PARAM_TYPE_REAL32;
        default:
            qWarning() << "Unsupported fact type" << factType;
            // fall through
        case FactMetaData::valueTypeInt32:
            return MAV_PARAM_TYPE_INT32;
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraParamIO::handleParamAck(const mavlink_param_ext_ack_t& ack)
{
    _paramWriteTimer.stop();
    if(ack.param_result == PARAM_ACK_ACCEPTED) {
        _fact->_containerSetRawValue(_valueFromMessage(ack.param_value, ack.param_type));
    }
    if(ack.param_result == PARAM_ACK_IN_PROGRESS) {
        //-- Wait a bit longer for this one
        qCDebug(CameraIOLogVerbose) << "Param set in progress:" << _fact->name();
        _paramWriteTimer.start();
    } else {
        if(ack.param_result == PARAM_ACK_FAILED) {
            qCWarning(CameraIOLog) << "Param set failed:" << _fact->name();
        } else if(ack.param_result == PARAM_ACK_VALUE_UNSUPPORTED) {
            qCWarning(CameraIOLog) << "Param set unsuported:" << _fact->name();
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
        MAVLinkProtocol* mavlink = qgcApp()->toolbox()->mavlinkProtocol();
        mavlink_message_t msg;
        mavlink_msg_param_ext_request_read_pack_chan(
            mavlink->getSystemId(),
            mavlink->getComponentId(),
            _vehicle->priorityLink()->mavlinkChannel(),
            &msg,
            _vehicle->id(),
            _control->compID(),
            _fact->name().toStdString().c_str(),
            -1);
        _vehicle->sendMessageOnLink(_vehicle->priorityLink(), msg);
        _paramRequestTimer.start();
    }
}
