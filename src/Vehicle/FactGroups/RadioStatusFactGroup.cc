#include "RadioStatusFactGroup.h"
#include "Vehicle.h"

#include <QtCore/QtMath>

RadioStatusFactGroup::RadioStatusFactGroup(QObject *parent)
    : FactGroup(1000, QStringLiteral(":/json/Vehicle/RadioStatusFact.json"), parent)
{
    _addFact(&_lrssiFact);
    _addFact(&_rrssiFact);
    _addFact(&_rxErrorsFact);
    _addFact(&_fixedFact);
    _addFact(&_txBufferFact);
    _addFact(&_lNoiseFact);
    _addFact(&_rNoiseFact);
}

void RadioStatusFactGroup::handleMessage(Vehicle *vehicle, const mavlink_message_t &message)
{
    Q_UNUSED(vehicle);

    if (message.msgid == MAVLINK_MSG_ID_RADIO_STATUS) {
        _handleRadioStatus(message);
    }
}

void RadioStatusFactGroup::_handleRadioStatus(const mavlink_message_t &message)
{
    mavlink_radio_status_t rstatus{};
    mavlink_msg_radio_status_decode(&message, &rstatus);

    int rssi   = rstatus.rssi;
    int remrssi = rstatus.remrssi;
    const int lnoise = static_cast<int>(static_cast<int8_t>(rstatus.noise));
    const int rnoise = static_cast<int>(static_cast<int8_t>(rstatus.remnoise));

    // 3DR Si1k radio reports raw register values; convert to dBm per the
    // Si1K datasheet figure 23.25 and SI AN474:
    //     inputPower = rssi * (10/19) - 127
    // Limit to the realistic range [-120, 0] dBm. Non-Si1k radios are assumed
    // to already be signed-8-bit dBm.
    if (message.sysid == '3' && message.compid == 'D') {
        rssi    = qBound(-120, qRound(static_cast<qreal>(rssi)    / 1.9 - 127.0), 0);
        remrssi = qBound(-120, qRound(static_cast<qreal>(remrssi) / 1.9 - 127.0), 0);
    } else {
        rssi    = static_cast<int>(static_cast<int8_t>(rstatus.rssi));
        remrssi = static_cast<int>(static_cast<int8_t>(rstatus.remrssi));
    }

    lrssi()->setRawValue(rssi);
    rrssi()->setRawValue(remrssi);
    rxErrors()->setRawValue(rstatus.rxerrors);
    fixed()->setRawValue(rstatus.fixed);
    txBuffer()->setRawValue(rstatus.txbuf);
    lNoise()->setRawValue(lnoise);
    rNoise()->setRawValue(rnoise);

    _setTelemetryAvailable(true);
}
