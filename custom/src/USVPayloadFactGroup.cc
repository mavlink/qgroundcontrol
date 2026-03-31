/****************************************************************************
 *
 * USV Payload Fact Group Implementation
 *
 ****************************************************************************/

#include "USVPayloadFactGroup.h"
#include "QGCMAVLink.h"

#include <cstring>

USVPayloadFactGroup::USVPayloadFactGroup(QObject *parent)
    : FactGroup(1000, QStringLiteral(":/json/USVPayloadFactGroup.json"), parent)
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

    _voltageFact.setRawValue(0);
    _absorbanceFact.setRawValue(0);
    _pumpXFact.setRawValue(0);
    _pumpYFact.setRawValue(0);
    _pumpZFact.setRawValue(0);
    _pumpAFact.setRawValue(0);
    _statusFact.setRawValue(0);
    _linkActiveFact.setRawValue(0);
    _packetCountFact.setRawValue(0);

    _timeoutTimer.setSingleShot(true);
    _timeoutTimer.setInterval(_timeoutMsecs);
    connect(&_timeoutTimer, &QTimer::timeout, this, &USVPayloadFactGroup::_telemetryTimeout);
}

void USVPayloadFactGroup::handleMessage(Vehicle *vehicle, const mavlink_message_t &message)
{
    Q_UNUSED(vehicle);

    switch (message.msgid) {
    case MAVLINK_MSG_ID_NAMED_VALUE_FLOAT:
        _handleNamedValueFloat(message);
        break;
    default:
        break;
    }
}

void USVPayloadFactGroup::_handleNamedValueFloat(const mavlink_message_t &message)
{
    mavlink_named_value_float_t namedValue{};
    mavlink_msg_named_value_float_decode(&message, &namedValue);

    // MAVLink NAMED_VALUE_FLOAT.name is a fixed 10-byte char array,
    // not guaranteed to be null-terminated. Copy to a null-terminated buffer.
    char nameBuf[11];
    memcpy(nameBuf, namedValue.name, sizeof(namedValue.name));
    nameBuf[sizeof(namedValue.name)] = '\0';
    const QString name = QString::fromLatin1(nameBuf);

    if (name == QLatin1String("USV_VOLT")) {
        voltage()->setRawValue(namedValue.value);
        _setTelemetryAvailable(true);
        _timeoutTimer.start();
        linkActive()->setRawValue(1);
    } else if (name == QLatin1String("USV_ABS")) {
        absorbance()->setRawValue(namedValue.value);
    } else if (name == QLatin1String("PUMP_X")) {
        pumpX()->setRawValue(namedValue.value);
    } else if (name == QLatin1String("PUMP_Y")) {
        pumpY()->setRawValue(namedValue.value);
    } else if (name == QLatin1String("PUMP_Z")) {
        pumpZ()->setRawValue(namedValue.value);
    } else if (name == QLatin1String("PUMP_A")) {
        pumpA()->setRawValue(namedValue.value);
    } else if (name == QLatin1String("USV_STAT")) {
        status()->setRawValue(static_cast<uint32_t>(namedValue.value));
    } else if (name == QLatin1String("USV_PKT")) {
        packetCount()->setRawValue(namedValue.value);
    }
}

void USVPayloadFactGroup::_telemetryTimeout()
{
    linkActive()->setRawValue(0);
    _setTelemetryAvailable(false);
}
