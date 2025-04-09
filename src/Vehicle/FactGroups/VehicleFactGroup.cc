/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "VehicleFactGroup.h"
#include "Vehicle.h"
#include "QGC.h"

#include <QtGui/QQuaternion>
#include <QtGui/QVector3D>

VehicleFactGroup::VehicleFactGroup(QObject *parent)
    : FactGroup(100, QStringLiteral(":/json/Vehicle/VehicleFact.json"), parent)
{
    _addFact(&_rollFact);
    _addFact(&_pitchFact);
    _addFact(&_headingFact);
    _addFact(&_rollRateFact);
    _addFact(&_pitchRateFact);
    _addFact(&_yawRateFact);
    _addFact(&_groundSpeedFact);
    _addFact(&_airSpeedFact);
    _addFact(&_airSpeedSetpointFact);
    _addFact(&_climbRateFact);
    _addFact(&_altitudeRelativeFact);
    _addFact(&_altitudeAMSLFact);
    _addFact(&_altitudeAboveTerrFact);
    _addFact(&_altitudeTuningFact);
    _addFact(&_altitudeTuningSetpointFact);
    _addFact(&_xTrackErrorFact);
    _addFact(&_rangeFinderDistFact);
    _addFact(&_flightDistanceFact);
    _addFact(&_flightTimeFact);
    _addFact(&_distanceToHomeFact);
    _addFact(&_timeToHomeFact);
    _addFact(&_missionItemIndexFact);
    _addFact(&_headingToNextWPFact);
    _addFact(&_distanceToNextWPFact);
    _addFact(&_headingToHomeFact);
    _addFact(&_distanceToGCSFact);
    _addFact(&_hobbsFact);
    _addFact(&_throttlePctFact);
    _addFact(&_imuTempFact);

    _hobbsFact.setRawValue(QStringLiteral("0000:00:00"));
}

void VehicleFactGroup::handleMessage(Vehicle *vehicle, const mavlink_message_t &message)
{
    switch (message.msgid) {
    case MAVLINK_MSG_ID_ATTITUDE:
        _handleAttitude(vehicle, message);
        break;
    case MAVLINK_MSG_ID_ATTITUDE_QUATERNION:
        _handleAttitudeQuaternion(vehicle, message);
        break;
    case MAVLINK_MSG_ID_ALTITUDE:
        _handleAltitude(message);
        break;
    case MAVLINK_MSG_ID_VFR_HUD:
        _handleVfrHud(message);
        break;
    case MAVLINK_MSG_ID_NAV_CONTROLLER_OUTPUT:
        _handleNavControllerOutput(message);
        break;
    case MAVLINK_MSG_ID_RAW_IMU:
        _handleRawImuTemp(message);
        break;
#ifndef QGC_NO_ARDUPILOT_DIALECT
    case MAVLINK_MSG_ID_RANGEFINDER:
        _handleRangefinder(message);
        break;
#endif
    default:
        break;
    }
}

void VehicleFactGroup::_handleAttitudeWorker(double rollRadians, double pitchRadians, double yawRadians)
{
    double rollDegrees = QGC::limitAngleToPMPIf(rollRadians);
    double pitchDegrees = QGC::limitAngleToPMPIf(pitchRadians);
    double yawDegrees = QGC::limitAngleToPMPIf(yawRadians);

    rollDegrees = qRadiansToDegrees(rollDegrees);
    pitchDegrees = qRadiansToDegrees(pitchDegrees);
    yawDegrees = qRadiansToDegrees(yawDegrees);

    if (yawDegrees < 0.0) {
        yawDegrees += 360.0;
    }
    // truncate to integer so widget never displays 360
    yawDegrees = trunc(yawDegrees);

    roll()->setRawValue(rollDegrees);
    pitch()->setRawValue(pitchDegrees);
    heading()->setRawValue(yawDegrees);
}

void VehicleFactGroup::_handleAttitude(Vehicle *vehicle, const mavlink_message_t &message)
{
    if ((message.sysid != vehicle->id()) || (message.compid != vehicle->compId())) {
        return;
    }

    if (_receivingAttitudeQuaternion) {
        return;
    }

    mavlink_attitude_t attitude{};
    mavlink_msg_attitude_decode(&message, &attitude);

    _handleAttitudeWorker(attitude.roll, attitude.pitch, attitude.yaw);

    _setTelemetryAvailable(true);
}

void VehicleFactGroup::_handleAltitude(const mavlink_message_t &message)
{
    mavlink_altitude_t altitude{};
    mavlink_msg_altitude_decode(&message, &altitude);

    // Data from ALTITUDE message takes precedence over gps messages
    _altitudeMessageAvailable = true;
    altitudeRelative()->setRawValue(altitude.altitude_relative);
    altitudeAMSL()->setRawValue(altitude.altitude_amsl);

    _setTelemetryAvailable(true);
}

void VehicleFactGroup::_handleAttitudeQuaternion(Vehicle *vehicle, const mavlink_message_t &message)
{
    // only accept the attitude message from the vehicle's flight controller
    if ((message.sysid != vehicle->id()) || (message.compid != vehicle->compId())) {
        return;
    }

    _receivingAttitudeQuaternion = true;

    mavlink_attitude_quaternion_t attitudeQuaternion{};
    mavlink_msg_attitude_quaternion_decode(&message, &attitudeQuaternion);

    QQuaternion quat(attitudeQuaternion.q1, attitudeQuaternion.q2, attitudeQuaternion.q3, attitudeQuaternion.q4);
    QVector3D rates(attitudeQuaternion.rollspeed, attitudeQuaternion.pitchspeed, attitudeQuaternion.yawspeed);
    QQuaternion repr_offset(attitudeQuaternion.repr_offset_q[0], attitudeQuaternion.repr_offset_q[1], attitudeQuaternion.repr_offset_q[2], attitudeQuaternion.repr_offset_q[3]);

    // if repr_offset is valid, rotate attitude and rates
    if (repr_offset.length() >= 0.5f) {
        quat *= repr_offset;
        rates = repr_offset * rates;
    }

    float attRoll, attPitch, attYaw;
    float q[] = { quat.scalar(), quat.x(), quat.y(), quat.z() };
    mavlink_quaternion_to_euler(q, &attRoll, &attPitch, &attYaw);

    _handleAttitudeWorker(attRoll, attPitch, attYaw);

    rollRate()->setRawValue(qRadiansToDegrees(rates[0]));
    pitchRate()->setRawValue(qRadiansToDegrees(rates[1]));
    yawRate()->setRawValue(qRadiansToDegrees(rates[2]));

    _setTelemetryAvailable(true);
}

void VehicleFactGroup::_handleNavControllerOutput(const mavlink_message_t &message)
{
    mavlink_nav_controller_output_t navControllerOutput{};
    mavlink_msg_nav_controller_output_decode(&message, &navControllerOutput);

    altitudeTuningSetpoint()->setRawValue(_altitudeTuningFact.rawValue().toDouble() - navControllerOutput.alt_error);
    xTrackError()->setRawValue(navControllerOutput.xtrack_error);
    airSpeedSetpoint()->setRawValue(_airSpeedFact.rawValue().toDouble() - navControllerOutput.aspd_error);
    distanceToNextWP()->setRawValue(navControllerOutput.wp_dist);

    _setTelemetryAvailable(true);
}

void VehicleFactGroup::_handleVfrHud(const mavlink_message_t &message)
{
    mavlink_vfr_hud_t vfrHud{};
    mavlink_msg_vfr_hud_decode(&message, &vfrHud);

    airSpeed()->setRawValue(qIsNaN(vfrHud.airspeed) ? 0 : vfrHud.airspeed);
    groundSpeed()->setRawValue(qIsNaN(vfrHud.groundspeed) ? 0 : vfrHud.groundspeed);
    climbRate()->setRawValue(qIsNaN(vfrHud.climb) ? 0 : vfrHud.climb);
    throttlePct()->setRawValue(static_cast<int16_t>(vfrHud.throttle));
    if (qIsNaN(_altitudeTuningOffset)) {
        _altitudeTuningOffset = vfrHud.alt;
    }
    altitudeTuning()->setRawValue(vfrHud.alt - _altitudeTuningOffset);
    if (!qIsNaN(vfrHud.groundspeed) && !qIsNaN(_distanceToHomeFact.cookedValue().toDouble())) {
      timeToHome()->setRawValue(_distanceToHomeFact.cookedValue().toDouble() / vfrHud.groundspeed);
    }

    _setTelemetryAvailable(true);
}

void VehicleFactGroup::_handleRawImuTemp(const mavlink_message_t &message)
{
    mavlink_raw_imu_t imuRaw{};
    mavlink_msg_raw_imu_decode(&message, &imuRaw);

    imuTemp()->setRawValue((imuRaw.temperature == 0) ? 0 : (imuRaw.temperature * 0.01));

    _setTelemetryAvailable(true);
}

#ifndef QGC_NO_ARDUPILOT_DIALECT
void VehicleFactGroup::_handleRangefinder(const mavlink_message_t &message)
{
    mavlink_rangefinder_t rangefinder{};
    mavlink_msg_rangefinder_decode(&message, &rangefinder);

    rangeFinderDist()->setRawValue(qIsNaN(rangefinder.distance) ? 0 : rangefinder.distance);

    _setTelemetryAvailable(true);
}
#endif
