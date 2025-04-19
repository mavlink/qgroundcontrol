/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "VehicleLocalPositionFactGroup.h"
#include "Vehicle.h"

VehicleLocalPositionFactGroup::VehicleLocalPositionFactGroup(QObject *parent)
    : FactGroup(1000, QStringLiteral(":/json/Vehicle/LocalPositionFact.json"), parent)
{
    _addFact(&_xFact);
    _addFact(&_yFact);
    _addFact(&_zFact);
    _addFact(&_vxFact);
    _addFact(&_vyFact);
    _addFact(&_vzFact);

    _xFact.setRawValue(qQNaN());
    _yFact.setRawValue(qQNaN());
    _zFact.setRawValue(qQNaN());
    _vxFact.setRawValue(qQNaN());
    _vyFact.setRawValue(qQNaN());
    _vzFact.setRawValue(qQNaN());
}

void VehicleLocalPositionFactGroup::handleMessage(Vehicle *vehicle, const mavlink_message_t &message)
{
    Q_UNUSED(vehicle);

    if (message.msgid != MAVLINK_MSG_ID_LOCAL_POSITION_NED) {
        return;
    }

    mavlink_local_position_ned_t localPosition{};
    mavlink_msg_local_position_ned_decode(&message, &localPosition);

    x()->setRawValue(localPosition.x);
    y()->setRawValue(localPosition.y);
    z()->setRawValue(localPosition.z);

    vx()->setRawValue(localPosition.vx);
    vy()->setRawValue(localPosition.vy);
    vz()->setRawValue(localPosition.vz);

    _setTelemetryAvailable(true);
}
