#include "QGCCameraIO.h"
#include "MAVLinkLib.h"
#include "QGCMAVLink.h"
#include "MavlinkCameraControlInterface.h"
#include "LinkInterface.h"
#include "MAVLinkProtocol.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

QGC_LOGGING_CATEGORY(QGCCameraParamIOLog, "Camera.QGCCameraParamIO")
QGC_LOGGING_CATEGORY(QGCCameraParamIOVerbose, "Camera.QGCCameraParamIO:verbose")

namespace {
    constexpr int kMaxRetries = 3;
}

QGCCameraParamIO::QGCCameraParamIO(MavlinkCameraControlInterface *control, Fact *fact, Vehicle *vehicle)
    : QObject(control)
    , _control(control)
    , _fact(fact)
    , _vehicle(vehicle)
{
    qCDebug(QGCCameraParamIOLog) << this;

    _paramWriteTimer.setSingleShot(true);
    _paramWriteTimer.setInterval(3000);
    _paramRequestTimer.setSingleShot(true);
    _paramRequestTimer.setInterval(3500);

    if (_fact->writeOnly()) {
        // Write mode is always "done" as it won't ever read
        _done = true;
    } else {
        (void) connect(&_paramRequestTimer, &QTimer::timeout, this, &QGCCameraParamIO::_paramRequestTimeout);
    }
    (void) connect(&_paramWriteTimer,   &QTimer::timeout, this, &QGCCameraParamIO::_paramWriteTimeout);
    (void) connect(_fact, &Fact::rawValueChanged, this, &QGCCameraParamIO::_factChanged);
    (void) connect(_fact, &Fact::containerRawValueChanged, this, &QGCCameraParamIO::_containerRawValueChanged);

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
            // String and custom are the same for now
        case FactMetaData::valueTypeString:
        case FactMetaData::valueTypeCustom:
            _mavParamType = MAV_PARAM_EXT_TYPE_CUSTOM;
            break;
        default:
            qCWarning(QGCCameraParamIOLog) << "Unsupported fact type" << _fact->type() << "for" << _fact->name();
            Q_FALLTHROUGH();
        case FactMetaData::valueTypeInt32:
            _mavParamType = MAV_PARAM_EXT_TYPE_INT32;
            break;
    }
}

QGCCameraParamIO::~QGCCameraParamIO()
{
    qCDebug(QGCCameraParamIOLog) << this;
}

void QGCCameraParamIO::_paramRequestTimeout()
{
    if (++_requestRetries > kMaxRetries) {
        qCWarning(QGCCameraParamIOLog) << "No response for param request:" << _fact->name();
        if (!_done) {
            _done = true;
            _control->_paramDone();
        }
    } else {
        // Request it again
        qCDebug(QGCCameraParamIOLog) << "Param request retry:" << _fact->name();
        paramRequest(false);
        _paramRequestTimer.start();
    }
}

void QGCCameraParamIO::_paramWriteTimeout()
{
    if (++_sentRetries > kMaxRetries) {
        qCWarning(QGCCameraParamIOLog) << "No response for param set:" << _fact->name();
        _updateOnSet = false;
    } else {
        // Send it again
        qCDebug(QGCCameraParamIOLog) << "Param set retry:" << _fact->name() << _sentRetries;
        _sendParameter();
        _paramWriteTimer.start();
    }
}

void QGCCameraParamIO::_factChanged(const QVariant &value)
{
    Q_UNUSED(value);

    if (!_forceUIUpdate) {
        qCDebug(QGCCameraParamIOLog) << "UI Fact" << _fact->name() << "changed to" << value;
        _control->factChanged(_fact);
    }
}

void QGCCameraParamIO::_containerRawValueChanged(const QVariant &value)
{
    Q_UNUSED(value);

    if (!_fact->readOnly()) {
        qCDebug(QGCCameraParamIOLog) << "Update Fact from camera" << _fact->name();
        _sentRetries = 0;
        _sendParameter();
    }
}

void QGCCameraParamIO::_sendParameter()
{
    SharedLinkInterfacePtr sharedLink = _vehicle->vehicleLinkManager()->primaryLink().lock();
    if (sharedLink) {
        mavlink_param_ext_set_t p{};
        p.param_type = _mavParamType;

        if (!QGCMAVLink::variantToParamExtValue(_fact->rawValue(), _mavParamType, &p.param_value[0])) {
            qCCritical(QGCCameraParamIOLog) << "Invalid value for" << _fact->name() << ":" << _fact->rawValue();
        }

        p.target_system = static_cast<uint8_t>(_vehicle->id());
        p.target_component = static_cast<uint8_t>(_control->compID());
        (void) qstrncpy(p.param_id, _fact->name().toStdString().c_str(), MAVLINK_MSG_PARAM_EXT_SET_FIELD_PARAM_ID_LEN);

        mavlink_message_t msg{};
        (void) mavlink_msg_param_ext_set_encode_chan(
            static_cast<uint8_t>(MAVLinkProtocol::instance()->getSystemId()),
            static_cast<uint8_t>(MAVLinkProtocol::getComponentId()),
            sharedLink->mavlinkChannel(),
            &msg,
            &p
        );
        (void) _vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
    }

    _paramWriteTimer.start();
}

void QGCCameraParamIO::handleParamAck(const mavlink_param_ext_ack_t &ack)
{
    _paramWriteTimer.stop();

    switch (ack.param_result) {
    case PARAM_ACK_ACCEPTED: {
        QVariant val = _valueFromMessage(ack.param_value, ack.param_type);
        if (_fact->rawValue() != val) {
            _fact->containerSetRawValue(val);
            if(_updateOnSet) {
                _updateOnSet = false;
                _control->factChanged(_fact);
            }
        }
        break;
    }
    case PARAM_ACK_IN_PROGRESS:
        // Wait a bit longer for this one
        qCDebug(QGCCameraParamIOLog) << "Param set in progress:" << _fact->name();
        _paramWriteTimer.start();
        break;
    case PARAM_ACK_FAILED:
        if (++_sentRetries < kMaxRetries) {
            // Try again
            qCWarning(QGCCameraParamIOLog) << "Param set failed:" << _fact->name() << _sentRetries;
            _paramWriteTimer.start();
        }
        break;
    case PARAM_ACK_VALUE_UNSUPPORTED:
        qCWarning(QGCCameraParamIOLog) << "Param set unsuported:" << _fact->name();
        Q_FALLTHROUGH();
    default: {
        // If UI changed and value was not set, restore UI
        QVariant val = _valueFromMessage(ack.param_value, ack.param_type);
        if (_fact->rawValue() != val) {
            if (_control->validateParameter(_fact, val)) {
                _fact->containerSetRawValue(val);
            }
        }
        break;
    }
    }
}

QVariant QGCCameraParamIO::_valueFromMessage(const char *value, uint8_t param_type)
{
    const QVariant var = QGCMAVLink::paramExtValueToVariant(value, param_type);

    return var.isValid() ? var : QVariant(0);
}

void QGCCameraParamIO::handleParamValue(const mavlink_param_ext_value_t &value)
{
    _paramRequestTimer.stop();

    QVariant newValue = _valueFromMessage(value.param_value, value.param_type);
    if (_control->incomingParameter(_fact, newValue)) {
        _fact->containerSetRawValue(newValue);
        _control->factChanged(_fact);
    }
    _paramRequestReceived = true;

    if (_forceUIUpdate) {
        emit _fact->rawValueChanged(_fact->rawValue());
        emit _fact->valueChanged(_fact->rawValue());
        _forceUIUpdate = false;
    }

    if (!_done) {
        _done = true;
        _control->_paramDone();
    }

    qCDebug(QGCCameraParamIOLog) << QStringLiteral("handleParamValue() %1 %2").arg(_fact->name(), _fact->rawValueString());
}

void QGCCameraParamIO::paramRequest(bool reset)
{
    // If it's write only, we don't request it.
    if (_fact->writeOnly()) {
        if (!_done) {
            _done = true;
            _control->_paramDone();
        }
        return;
    }

    if (reset) {
        _requestRetries = 0;
        _forceUIUpdate  = true;
    }

    qCDebug(QGCCameraParamIOLog) << "Request parameter:" << _fact->name();
    SharedLinkInterfacePtr sharedLink = _vehicle->vehicleLinkManager()->primaryLink().lock();
    if (sharedLink) {
        char param_id[MAVLINK_MSG_PARAM_EXT_REQUEST_READ_FIELD_PARAM_ID_LEN + 1] = {};
        (void) strncpy(param_id, _fact->name().toStdString().c_str(), MAVLINK_MSG_PARAM_EXT_REQUEST_READ_FIELD_PARAM_ID_LEN);
        mavlink_message_t msg{};
        (void) mavlink_msg_param_ext_request_read_pack_chan(
            static_cast<uint8_t>(MAVLinkProtocol::instance()->getSystemId()),
            static_cast<uint8_t>(MAVLinkProtocol::getComponentId()),
            sharedLink->mavlinkChannel(),
            &msg,
            static_cast<uint8_t>(_vehicle->id()),
            static_cast<uint8_t>(_control->compID()),
            param_id,
            -1
        );
        _vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
    }
    _paramRequestTimer.start();
}

void QGCCameraParamIO::sendParameter(bool updateUI)
{
    qCDebug(QGCCameraParamIOLog) << "Send Fact" << _fact->name();
    _sentRetries = 0;
    _updateOnSet = updateUI;
    _sendParameter();
}

void QGCCameraParamIO::setParamRequest()
{
    if (!_fact->writeOnly()) {
        _paramRequestReceived = false;
        _requestRetries = 0;
        _paramRequestTimer.start();
    }
}
