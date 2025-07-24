/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "VehicleVibrationFactGroup.h"
#include "Vehicle.h"

VehicleVibrationFactGroup::VehicleVibrationFactGroup(QObject *parent)
    : FactGroup(1000, QStringLiteral(":/json/Vehicle/VibrationFact.json"), parent)
{
    _addFact(&_xAxisFact);
    _addFact(&_yAxisFact);
    _addFact(&_zAxisFact);
    _addFact(&_clipCount1Fact);
    _addFact(&_clipCount2Fact);
    _addFact(&_clipCount3Fact);

    _xAxisFact.setRawValue(qQNaN());
    _yAxisFact.setRawValue(qQNaN());
    _zAxisFact.setRawValue(qQNaN());
}

void VehicleVibrationFactGroup::handleMessage(Vehicle *vehicle, const mavlink_message_t &message)
{
    Q_UNUSED(vehicle);

    if (message.msgid != MAVLINK_MSG_ID_VIBRATION) {
        return;
    }

    mavlink_vibration_t vibration{};
    mavlink_msg_vibration_decode(&message, &vibration);

    xAxis()->setRawValue(vibration.vibration_x);
    yAxis()->setRawValue(vibration.vibration_y);
    zAxis()->setRawValue(vibration.vibration_z);
    clipCount1()->setRawValue(vibration.clipping_0);
    clipCount2()->setRawValue(vibration.clipping_1);
    clipCount3()->setRawValue(vibration.clipping_2);

    _setTelemetryAvailable(true);
}
