/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCCameraIO.h"
#include "MavlinkCameraControl.h"
#include "LinkInterface.h"
#include "MAVLinkProtocol.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"

QGC_LOGGING_CATEGORY(QGCCameraParamIOLog, "Camera.QGCCameraParamIO")
QGC_LOGGING_CATEGORY(QGCCameraParamIOVerbose, "Camera.QGCCameraParamIO:verbose")

namespace {
    constexpr int kMaxRetries = 3;
}

QGCCameraParamIO::QGCCameraParamIO(MavlinkCameraControl *control, Fact *fact, Vehicle *vehicle)
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

        QGCMAVLink::param_ext_union_t union_value{};
        const FactMetaData::ValueType_t factType = _fact->type();
        bool ok = true;
        switch (factType) {
        case FactMetaData::valueTypeUint8:
        case FactMetaData::valueTypeBool:
            union_value.param_uint8 = static_cast<uint8_t>(_fact->rawValue().toUInt(&ok));
            break;
        case FactMetaData::valueTypeInt8:
            union_value.param_int8 = static_cast<int8_t>(_fact->rawValue().toInt(&ok));
            break;
        case FactMetaData::valueTypeUint16:
            union_value.param_uint16 = static_cast<uint16_t>(_fact->rawValue().toUInt(&ok));
            break;
        case FactMetaData::valueTypeInt16:
            union_value.param_int16 = static_cast<int16_t>(_fact->rawValue().toInt(&ok));
            break;
        case FactMetaData::valueTypeUint32:
            union_value.param_uint32 = static_cast<uint32_t>(_fact->rawValue().toUInt(&ok));
            break;
        case FactMetaData::valueTypeInt64:
            union_value.param_int64 = static_cast<int64_t>(_fact->rawValue().toLongLong(&ok));
            break;
        case FactMetaData::valueTypeUint64:
            union_value.param_uint64 = static_cast<uint64_t>(_fact->rawValue().toULongLong(&ok));
            break;
        case FactMetaData::valueTypeFloat:
            union_value.param_float = _fact->rawValue().toFloat(&ok);
            break;
        case FactMetaData::valueTypeDouble:
            union_value.param_double = _fact->rawValue().toDouble(&ok);
            break;
            // String and custom are the same for now
        case FactMetaData::valueTypeString:
        case FactMetaData::valueTypeCustom: {
            const QByteArray custom = _fact->rawValue().toByteArray();
            (void) memcpy(union_value.bytes, custom.constData(), static_cast<size_t>(std::max(custom.size(), static_cast<qsizetype>(MAVLINK_MSG_PARAM_EXT_SET_FIELD_PARAM_VALUE_LEN))));
            break;
        }
        default:
            qCCritical(QGCCameraParamIOLog) << "Unsupported fact type" << factType << "for" << _fact->name();
            Q_FALLTHROUGH();
        case FactMetaData::valueTypeInt32:
            union_value.param_int32 = static_cast<int32_t>(_fact->rawValue().toInt(&ok));
            break;
        }

        if (!ok) {
            qCCritical(QGCCameraParamIOLog) << "Invalid value for" << _fact->name() << ":" << _fact->rawValue();
        }

        (void) memcpy(&p.param_value[0], &union_value.bytes[0], MAVLINK_MSG_PARAM_EXT_SET_FIELD_PARAM_VALUE_LEN);
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
    QVariant var;
    QGCMAVLink::param_ext_union_t u{};
    (void) memcpy(u.bytes, value, MAVLINK_MSG_PARAM_EXT_SET_FIELD_PARAM_VALUE_LEN);
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
            var = QVariant(static_cast<quint64>(u.param_uint64));
            break;
        case MAV_PARAM_EXT_TYPE_INT64:
            var = QVariant(static_cast<qint64>(u.param_int64));
            break;
        case MAV_PARAM_EXT_TYPE_CUSTOM: {
            // This will null terminate the name string
            char strValueWithNull[MAVLINK_MSG_PARAM_EXT_SET_FIELD_PARAM_VALUE_LEN + 1] = {};
            (void) strncpy(strValueWithNull, value, MAVLINK_MSG_PARAM_EXT_SET_FIELD_PARAM_VALUE_LEN);
            const QString strValue(strValueWithNull);
            var = QVariant(strValue);
            break;
        }
        default:
            var = QVariant(0);
            qCCritical(QGCCameraParamIOLog) << "Invalid param_type used for camera setting:" << param_type;
            break;
    }

    return var;
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
