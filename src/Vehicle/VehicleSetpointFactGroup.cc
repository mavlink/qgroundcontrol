/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "VehicleSetpointFactGroup.h"
#include "Vehicle.h"

#include <QtMath>

const char* VehicleSetpointFactGroup::_rollFactName =       "roll";
const char* VehicleSetpointFactGroup::_pitchFactName =      "pitch";
const char* VehicleSetpointFactGroup::_yawFactName =        "yaw";
const char* VehicleSetpointFactGroup::_rollRateFactName =   "rollRate";
const char* VehicleSetpointFactGroup::_pitchRateFactName =  "pitchRate";
const char* VehicleSetpointFactGroup::_yawRateFactName =    "yawRate";

VehicleSetpointFactGroup::VehicleSetpointFactGroup(QObject* parent)
    : FactGroup     (1000, ":/json/Vehicle/SetpointFact.json", parent)
    , _rollFact     (0, _rollFactName,      FactMetaData::valueTypeDouble)
    , _pitchFact    (0, _pitchFactName,     FactMetaData::valueTypeDouble)
    , _yawFact      (0, _yawFactName,       FactMetaData::valueTypeDouble)
    , _rollRateFact (0, _rollRateFactName,  FactMetaData::valueTypeDouble)
    , _pitchRateFact(0, _pitchRateFactName, FactMetaData::valueTypeDouble)
    , _yawRateFact  (0, _yawRateFactName,   FactMetaData::valueTypeDouble)
{
    _addFact(&_rollFact,        _rollFactName);
    _addFact(&_pitchFact,       _pitchFactName);
    _addFact(&_yawFact,         _yawFactName);
    _addFact(&_rollRateFact,    _rollRateFactName);
    _addFact(&_pitchRateFact,   _pitchRateFactName);
    _addFact(&_yawRateFact,     _yawRateFactName);

    // Start out as not available "--.--"
    _rollFact.setRawValue(qQNaN());
    _pitchFact.setRawValue(qQNaN());
    _yawFact.setRawValue(qQNaN());
    _rollRateFact.setRawValue(qQNaN());
    _pitchRateFact.setRawValue(qQNaN());
    _yawRateFact.setRawValue(qQNaN());
}

void VehicleSetpointFactGroup::handleMessage(Vehicle* /* vehicle */, mavlink_message_t& message)
{
    if (message.msgid != MAVLINK_MSG_ID_ATTITUDE_TARGET) {
        return;
    }

    mavlink_attitude_target_t attitudeTarget;

    mavlink_msg_attitude_target_decode(&message, &attitudeTarget);

    float roll, pitch, yaw;
    mavlink_quaternion_to_euler(attitudeTarget.q, &roll, &pitch, &yaw);

    this->roll()->setRawValue   (qRadiansToDegrees(roll));
    this->pitch()->setRawValue  (qRadiansToDegrees(pitch));
    if (yaw < 0.f) yaw += 2.f * (float)M_PI; // bring to range [0, 2pi] to match the heading angle
    this->yaw()->setRawValue    (qRadiansToDegrees(yaw));

    rollRate()->setRawValue (qRadiansToDegrees(attitudeTarget.body_roll_rate));
    pitchRate()->setRawValue(qRadiansToDegrees(attitudeTarget.body_pitch_rate));
    yawRate()->setRawValue  (qRadiansToDegrees(attitudeTarget.body_yaw_rate));

    _setTelemetryAvailable(true);
}
