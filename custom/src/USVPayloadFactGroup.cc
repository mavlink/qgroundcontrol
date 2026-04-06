/****************************************************************************
 *
 * USV Payload Fact Group Implementation
 * 通信诊断增强: 追踪 sysid/compid/msgid 统计
 *
 ****************************************************************************/

#include "USVPayloadFactGroup.h"
#include "QGCMAVLink.h"

#include <cstring>
#include <QtCore/QLoggingCategory>

Q_LOGGING_CATEGORY(USVPayloadLog, "USV.Payload")

USVPayloadFactGroup::USVPayloadFactGroup(QObject *parent)
    : FactGroup(1000, QStringLiteral(":/json/USVPayloadFactGroup.json"), parent)
    , _voltageFact    (0, QStringLiteral("voltage"),     FactMetaData::valueTypeFloat)
    , _absorbanceFact (0, QStringLiteral("absorbance"),  FactMetaData::valueTypeFloat)
    , _pumpXFact      (0, QStringLiteral("pumpX"),       FactMetaData::valueTypeFloat)
    , _pumpYFact      (0, QStringLiteral("pumpY"),       FactMetaData::valueTypeFloat)
    , _pumpZFact      (0, QStringLiteral("pumpZ"),       FactMetaData::valueTypeFloat)
    , _pumpAFact      (0, QStringLiteral("pumpA"),       FactMetaData::valueTypeFloat)
    , _statusFact     (0, QStringLiteral("status"),      FactMetaData::valueTypeUint32)
    , _linkActiveFact (0, QStringLiteral("linkActive"),  FactMetaData::valueTypeUint32)
    , _packetCountFact(0, QStringLiteral("packetCount"), FactMetaData::valueTypeFloat)
{
    _addFact(&_voltageFact);
    _addFact(&_absorbanceFact);
    _addFact(&_pumpXFact);
    _addFact(&_pumpYFact);
    _addFact(&_pumpZFact);
    _addFact(&_pumpAFact);
    _addFact(&_statusFact);
    _addFact(&_linkActiveFact);
    _addFact(&_packetCountFact);

    _timeoutTimer.setSingleShot(true);
    _timeoutTimer.setInterval(_timeoutMsecs);
    connect(&_timeoutTimer, &QTimer::timeout, this, &USVPayloadFactGroup::_telemetryTimeout);

    _latencyTimer.start();
}

void USVPayloadFactGroup::handleMessage(Vehicle *vehicle, const mavlink_message_t &message)
{
    Q_UNUSED(vehicle);

    // 统计所有到达此 FactGroup 的消息
    _rxMsgTotal++;
    _lastSysid  = message.sysid;
    _lastCompid = message.compid;

    switch (message.msgid) {
    case MAVLINK_MSG_ID_NAMED_VALUE_FLOAT:
        _rxNamedValue++;
        _handleNamedValueFloat(message);
        break;
    case MAVLINK_MSG_ID_DEBUG_VECT:
        _handleDebugVect(message);
        break;
    case MAVLINK_MSG_ID_DEBUG:
        _handleDebug(message);
        break;
    case MAVLINK_MSG_ID_HEARTBEAT:
        // 伴随计算机心跳: sysid=2 compid=191 (MAV_COMP_ID_ONBOARD_COMPUTER)
        if (message.sysid == 2 && message.compid == 191) {
            _rxHeartbeat++;
            _latencyMs = _latencyTimer.elapsed();
            _latencyTimer.restart();
        }
        break;
    default:
        break;
    }

    // 每 50 条消息发一次诊断变更通知 (避免过度刷新)
    if (_rxMsgTotal % 50 == 0) {
        emit diagnosticsChanged();
    }
}

void USVPayloadFactGroup::_handleNamedValueFloat(const mavlink_message_t &message)
{
    mavlink_named_value_float_t namedValue{};
    mavlink_msg_named_value_float_decode(&message, &namedValue);

    char nameBuf[11];
    memcpy(nameBuf, namedValue.name, sizeof(namedValue.name));
    nameBuf[sizeof(namedValue.name)] = '\0';
    const QString name = QString::fromLatin1(nameBuf);
    bool handled = false;

    // 电压/吸光度仍通过 NAMED_VALUE_FLOAT 接收 (兼容 + 主通道)
    if (name == QLatin1String("USV_VOLT")) {
        voltage()->setRawValue(namedValue.value);
        handled = true;
    } else if (name == QLatin1String("USV_ABS")) {
        absorbance()->setRawValue(namedValue.value);
        handled = true;
    }
    // 旧通道兼容: 如果仍有 PUMP_X/Y/Z/A/USV_STAT/USV_PKT 通过 NAMED_VALUE_FLOAT 到达也更新
    else if (name == QLatin1String("PUMP_X")) {
        pumpX()->setRawValue(namedValue.value); handled = true;
    } else if (name == QLatin1String("PUMP_Y")) {
        pumpY()->setRawValue(namedValue.value); handled = true;
    } else if (name == QLatin1String("PUMP_Z")) {
        pumpZ()->setRawValue(namedValue.value); handled = true;
    } else if (name == QLatin1String("PUMP_A")) {
        pumpA()->setRawValue(namedValue.value); handled = true;
    } else if (name == QLatin1String("USV_STAT")) {
        status()->setRawValue(static_cast<uint32_t>(namedValue.value)); handled = true;
    } else if (name == QLatin1String("USV_PKT")) {
        packetCount()->setRawValue(namedValue.value); handled = true;
    }

    if (handled) {
        _setTelemetryAvailable(true);
        _timeoutTimer.start();
        linkActive()->setRawValue(1);
    } else {
        qCDebug(USVPayloadLog) << "Unhandled NAMED_VALUE_FLOAT:" << name
                                << "sysid:" << message.sysid
                                << "compid:" << message.compid;
    }
}

void USVPayloadFactGroup::_handleDebugVect(const mavlink_message_t &message)
{
    mavlink_debug_vect_t dv{};
    mavlink_msg_debug_vect_decode(&message, &dv);

    char nameBuf[11];
    memcpy(nameBuf, dv.name, sizeof(dv.name));
    nameBuf[sizeof(dv.name)] = '\0';
    const QString name = QString::fromLatin1(nameBuf);

    if (name == QLatin1String("PUMP_XYZ")) {
        pumpX()->setRawValue(dv.x);
        pumpY()->setRawValue(dv.y);
        pumpZ()->setRawValue(dv.z);
    } else if (name == QLatin1String("PUMP_A__")) {
        pumpA()->setRawValue(dv.x);
        status()->setRawValue(static_cast<uint32_t>(dv.y));
        packetCount()->setRawValue(dv.z);
    } else {
        qCDebug(USVPayloadLog) << "Unhandled DEBUG_VECT:" << name;
        return;
    }

    _setTelemetryAvailable(true);
    _timeoutTimer.start();
    linkActive()->setRawValue(1);
}

void USVPayloadFactGroup::_handleDebug(const mavlink_message_t &message)
{
    mavlink_debug_t dbg{};
    mavlink_msg_debug_decode(&message, &dbg);

    // ind=0 -> 包计数
    if (dbg.ind == 0) {
        packetCount()->setRawValue(dbg.value);
    } else {
        qCDebug(USVPayloadLog) << "Unhandled DEBUG ind:" << dbg.ind;
        return;
    }

    _setTelemetryAvailable(true);
    _timeoutTimer.start();
    linkActive()->setRawValue(1);
}

void USVPayloadFactGroup::_telemetryTimeout()
{
    linkActive()->setRawValue(0);
    _setTelemetryAvailable(false);
    qCDebug(USVPayloadLog) << "Telemetry timeout - link inactive";
    emit diagnosticsChanged();
}

QString USVPayloadFactGroup::diagSummary() const
{
    return QStringLiteral("rx:%1 named:%2 hb:%3 sys:%4 comp:%5 lat:%6ms")
        .arg(_rxMsgTotal)
        .arg(_rxNamedValue)
        .arg(_rxHeartbeat)
        .arg(_lastSysid)
        .arg(_lastCompid)
        .arg(_latencyMs, 0, 'f', 0);
}
