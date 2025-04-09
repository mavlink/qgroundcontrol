/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "VehicleSetpointFactGroup.h"
#include "Vehicle.h"

#include <QtMath>

VehicleSetpointFactGroup::VehicleSetpointFactGroup(QObject *parent)
    : FactGroup(1000, QStringLiteral(":/json/Vehicle/SetpointFact.json"), parent)
{
    _addFact(&_rollFact);
    _addFact(&_pitchFact);
    _addFact(&_yawFact);
    _addFact(&_rollRateFact);
    _addFact(&_pitchRateFact);
    _addFact(&_yawRateFact);

    _rollFact.setRawValue(qQNaN());
    _pitchFact.setRawValue(qQNaN());
    _yawFact.setRawValue(qQNaN());
    _rollRateFact.setRawValue(qQNaN());
    _pitchRateFact.setRawValue(qQNaN());
    _yawRateFact.setRawValue(qQNaN());
}

void VehicleSetpointFactGroup::handleMessage(Vehicle *vehicle, const mavlink_message_t &message)
{
    Q_UNUSED(vehicle);

    if (message.msgid != MAVLINK_MSG_ID_ATTITUDE_TARGET) {
        return;
    }

    mavlink_attitude_target_t attitudeTarget{};
    mavlink_msg_attitude_target_decode(&message, &attitudeTarget);

    float targetRoll, targetPitch, targetYaw;
    mavlink_quaternion_to_euler(attitudeTarget.q, &targetRoll, &targetPitch, &targetYaw);

    roll()->setRawValue(qRadiansToDegrees(targetRoll));
    pitch()->setRawValue(qRadiansToDegrees(targetPitch));
    if (targetYaw < 0.f) {
        targetYaw += 2.f * static_cast<float>(M_PI); // bring to range [0, 2pi] to match the heading angle
    }
    yaw()->setRawValue(qRadiansToDegrees(targetYaw));

    rollRate()->setRawValue(qRadiansToDegrees(attitudeTarget.body_roll_rate));
    pitchRate()->setRawValue(qRadiansToDegrees(attitudeTarget.body_pitch_rate));
    yawRate()->setRawValue(qRadiansToDegrees(attitudeTarget.body_yaw_rate));

    _setTelemetryAvailable(true);
}
