/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "VehicleLocalPositionSetpointFactGroup.h"
#include "Vehicle.h"

VehicleLocalPositionSetpointFactGroup::VehicleLocalPositionSetpointFactGroup(QObject *parent)
    : FactGroup(1000, QStringLiteral(":/json/Vehicle/LocalPositionSetpointFact.json"), parent)
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

void VehicleLocalPositionSetpointFactGroup::handleMessage(Vehicle *vehicle, const mavlink_message_t &message)
{
    Q_UNUSED(vehicle);

    if (message.msgid != MAVLINK_MSG_ID_POSITION_TARGET_LOCAL_NED) {
        return;
    }

    mavlink_position_target_local_ned_t localPosition{};
    mavlink_msg_position_target_local_ned_decode(&message, &localPosition);

    x()->setRawValue(localPosition.x);
    y()->setRawValue(localPosition.y);
    z()->setRawValue(localPosition.z);

    vx()->setRawValue(localPosition.vx);
    vy()->setRawValue(localPosition.vy);
    vz()->setRawValue(localPosition.vz);

    _setTelemetryAvailable(true);
}
