/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "VehicleEscStatusFactGroup.h"
#include "Vehicle.h"

VehicleEscStatusFactGroup::VehicleEscStatusFactGroup(QObject *parent)
    : FactGroup(1000, QStringLiteral(":/json/Vehicle/EscStatusFactGroup.json"), parent)
{
    _addFact(&_indexFact);

    _addFact(&_rpmFirstFact);
    _addFact(&_rpmSecondFact);
    _addFact(&_rpmThirdFact);
    _addFact(&_rpmFourthFact);

    _addFact(&_currentFirstFact);
    _addFact(&_currentSecondFact);
    _addFact(&_currentThirdFact);
    _addFact(&_currentFourthFact);

    _addFact(&_voltageFirstFact);
    _addFact(&_voltageSecondFact);
    _addFact(&_voltageThirdFact);
    _addFact(&_voltageFourthFact);
}

void VehicleEscStatusFactGroup::handleMessage(Vehicle *vehicle, const mavlink_message_t &message)
{
    Q_UNUSED(vehicle);

    if (message.msgid != MAVLINK_MSG_ID_ESC_STATUS) {
        return;
    }

    mavlink_esc_status_t content{};
    mavlink_msg_esc_status_decode(&message, &content);

    index()->setRawValue(content.index);

    rpmFirst()->setRawValue(content.rpm[0]);
    rpmSecond()->setRawValue(content.rpm[1]);
    rpmThird()->setRawValue(content.rpm[2]);
    rpmFourth()->setRawValue(content.rpm[3]);

    currentFirst()->setRawValue(content.current[0]);
    currentSecond()->setRawValue(content.current[1]);
    currentThird()->setRawValue(content.current[2]);
    currentFourth()->setRawValue(content.current[3]);

    voltageFirst()->setRawValue(content.voltage[0]);
    voltageSecond()->setRawValue(content.voltage[1]);
    voltageThird()->setRawValue(content.voltage[2]);
    voltageFourth()->setRawValue(content.voltage[3]);

    _setTelemetryAvailable(true);
}
