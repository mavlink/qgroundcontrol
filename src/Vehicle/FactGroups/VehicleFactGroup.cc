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

VehicleFactGroup::VehicleFactGroup(QObject* parent)
    : FactGroup                     (_vehicleUIUpdateRateMSecs, ":/json/Vehicle/VehicleFact.json", parent)
    , _rollFact                     (0, _rollFactName,                      FactMetaData::valueTypeDouble)
    , _pitchFact                    (0, _pitchFactName,                     FactMetaData::valueTypeDouble)
    , _headingFact                  (0, _headingFactName,                   FactMetaData::valueTypeDouble)
    , _rollRateFact                 (0, _rollRateFactName,                  FactMetaData::valueTypeDouble)
    , _pitchRateFact                (0, _pitchRateFactName,                 FactMetaData::valueTypeDouble)
    , _yawRateFact                  (0, _yawRateFactName,                   FactMetaData::valueTypeDouble)
    , _groundSpeedFact              (0, _groundSpeedFactName,               FactMetaData::valueTypeDouble)
    , _airSpeedFact                 (0, _airSpeedFactName,                  FactMetaData::valueTypeDouble)
    , _airSpeedSetpointFact         (0, _airSpeedSetpointFactName,          FactMetaData::valueTypeDouble)
    , _climbRateFact                (0, _climbRateFactName,                 FactMetaData::valueTypeDouble)
    , _altitudeRelativeFact         (0, _altitudeRelativeFactName,          FactMetaData::valueTypeDouble)
    , _altitudeAMSLFact             (0, _altitudeAMSLFactName,              FactMetaData::valueTypeDouble)
    , _altitudeAboveTerrFact        (0, _altitudeAboveTerrFactName,         FactMetaData::valueTypeDouble)
    , _altitudeTuningFact           (0, _altitudeTuningFactName,            FactMetaData::valueTypeDouble)
    , _altitudeTuningSetpointFact   (0, _altitudeTuningSetpointFactName,    FactMetaData::valueTypeDouble)
    , _xTrackErrorFact              (0, _xTrackErrorFactName,               FactMetaData::valueTypeDouble)
    , _rangeFinderDistFact          (0, _rangeFinderDistFactName,           FactMetaData::valueTypeFloat)
    , _flightDistanceFact           (0, _flightDistanceFactName,            FactMetaData::valueTypeDouble)
    , _flightTimeFact               (0, _flightTimeFactName,                FactMetaData::valueTypeElapsedTimeInSeconds)
    , _distanceToHomeFact           (0, _distanceToHomeFactName,            FactMetaData::valueTypeDouble)
    , _timeToHomeFact               (0, _timeToHomeFactName,                FactMetaData::valueTypeDouble)
    , _missionItemIndexFact         (0, _missionItemIndexFactName,          FactMetaData::valueTypeUint16)
    , _headingToNextWPFact          (0, _headingToNextWPFactName,           FactMetaData::valueTypeDouble)
    , _distanceToNextWPFact         (0, _distanceToNextWPFactName,          FactMetaData::valueTypeDouble)
    , _headingToHomeFact            (0, _headingToHomeFactName,             FactMetaData::valueTypeDouble)
    , _distanceToGCSFact            (0, _distanceToGCSFactName,             FactMetaData::valueTypeDouble)
    , _hobbsFact                    (0, _hobbsFactName,                     FactMetaData::valueTypeString)
    , _throttlePctFact              (0, _throttlePctFactName,               FactMetaData::valueTypeUint16)
    , _imuTempFact                  (0, _imuTempFactName,                   FactMetaData::valueTypeInt16)
{
    _addFact(&_rollFact,                    _rollFactName);
    _addFact(&_pitchFact,                   _pitchFactName);
    _addFact(&_headingFact,                 _headingFactName);
    _addFact(&_rollRateFact,                _rollRateFactName);
    _addFact(&_pitchRateFact,               _pitchRateFactName);
    _addFact(&_yawRateFact,                 _yawRateFactName);
    _addFact(&_groundSpeedFact,             _groundSpeedFactName);
    _addFact(&_airSpeedFact,                _airSpeedFactName);
    _addFact(&_airSpeedSetpointFact,        _airSpeedSetpointFactName);
    _addFact(&_climbRateFact,               _climbRateFactName);
    _addFact(&_altitudeRelativeFact,        _altitudeRelativeFactName);
    _addFact(&_altitudeAMSLFact,            _altitudeAMSLFactName);
    _addFact(&_altitudeAboveTerrFact,       _altitudeAboveTerrFactName);
    _addFact(&_altitudeTuningFact,          _altitudeTuningFactName);
    _addFact(&_altitudeTuningSetpointFact,  _altitudeTuningSetpointFactName);
    _addFact(&_xTrackErrorFact,             _xTrackErrorFactName);
    _addFact(&_rangeFinderDistFact,         _rangeFinderDistFactName);
    _addFact(&_flightDistanceFact,          _flightDistanceFactName);
    _addFact(&_flightTimeFact,              _flightTimeFactName);
    _addFact(&_distanceToHomeFact,          _distanceToHomeFactName);
    _addFact(&_timeToHomeFact,              _timeToHomeFactName);
    _addFact(&_missionItemIndexFact,        _missionItemIndexFactName);
    _addFact(&_headingToNextWPFact,         _headingToNextWPFactName);
    _addFact(&_distanceToNextWPFact,        _distanceToNextWPFactName);
    _addFact(&_headingToHomeFact,           _headingToHomeFactName);
    _addFact(&_distanceToGCSFact,           _distanceToGCSFactName);
    _addFact(&_hobbsFact,                   _hobbsFactName);
    _addFact(&_throttlePctFact,             _throttlePctFactName);
    _addFact(&_imuTempFact,                 _imuTempFactName);

    _hobbsFact.setRawValue(QVariant(QString("0000:00:00")));
}

void VehicleFactGroup::handleMessage(Vehicle* vehicle, mavlink_message_t& message)
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
    double roll = QGC::limitAngleToPMPIf(rollRadians);
    double pitch = QGC::limitAngleToPMPIf(pitchRadians);
    double yaw = QGC::limitAngleToPMPIf(yawRadians);

    roll = qRadiansToDegrees(roll);
    pitch = qRadiansToDegrees(pitch);
    yaw = qRadiansToDegrees(yaw);

    if (yaw < 0.0) {
        yaw += 360.0;
    }
    // truncate to integer so widget never displays 360
    yaw = trunc(yaw);

    _rollFact.setRawValue(roll);
    _pitchFact.setRawValue(pitch);
    _headingFact.setRawValue(yaw);
}

void VehicleFactGroup::_handleAttitude(Vehicle* vehicle, const mavlink_message_t &message)
{
    // only accept the attitude message from the vehicle's flight controller
    if (message.sysid != vehicle->id() || message.compid != vehicle->compId()) {
        return;
    }

    if (_receivingAttitudeQuaternion) {
        return;
    }

    mavlink_attitude_t attitude;
    mavlink_msg_attitude_decode(&message, &attitude);

    _handleAttitudeWorker(attitude.roll, attitude.pitch, attitude.yaw);
}

void VehicleFactGroup::_handleAltitude(const mavlink_message_t &message)
{
    mavlink_altitude_t altitude;
    mavlink_msg_altitude_decode(&message, &altitude);

    // Data from ALTITUDE message takes precedence over gps messages
    _altitudeMessageAvailable = true;
    _altitudeRelativeFact.setRawValue(altitude.altitude_relative);
    _altitudeAMSLFact.setRawValue(altitude.altitude_amsl);
}

void VehicleFactGroup::_handleAttitudeQuaternion(Vehicle* vehicle, const mavlink_message_t &message)
{
    // only accept the attitude message from the vehicle's flight controller
    if (message.sysid != vehicle->id() || message.compid != vehicle->compId()) {
        return;
    }

    _receivingAttitudeQuaternion = true;

    mavlink_attitude_quaternion_t attitudeQuaternion;
    mavlink_msg_attitude_quaternion_decode(&message, &attitudeQuaternion);

    QQuaternion quat(attitudeQuaternion.q1, attitudeQuaternion.q2, attitudeQuaternion.q3, attitudeQuaternion.q4);
    QVector3D rates(attitudeQuaternion.rollspeed, attitudeQuaternion.pitchspeed, attitudeQuaternion.yawspeed);
    QQuaternion repr_offset(attitudeQuaternion.repr_offset_q[0], attitudeQuaternion.repr_offset_q[1], attitudeQuaternion.repr_offset_q[2], attitudeQuaternion.repr_offset_q[3]);

    // if repr_offset is valid, rotate attitude and rates
    if (repr_offset.length() >= 0.5f) {
        quat = quat * repr_offset;
        rates = repr_offset * rates;
    }

    float roll, pitch, yaw;
    float q[] = { quat.scalar(), quat.x(), quat.y(), quat.z() };
    mavlink_quaternion_to_euler(q, &roll, &pitch, &yaw);

    _handleAttitudeWorker(roll, pitch, yaw);

    _rollRateFact.setRawValue(qRadiansToDegrees(rates[0]));
    _pitchRateFact.setRawValue(qRadiansToDegrees(rates[1]));
    _yawRateFact.setRawValue(qRadiansToDegrees(rates[2]));
}

void VehicleFactGroup::_handleNavControllerOutput(const mavlink_message_t &message)
{
    mavlink_nav_controller_output_t navControllerOutput;
    mavlink_msg_nav_controller_output_decode(&message, &navControllerOutput);

    _altitudeTuningSetpointFact.setRawValue(_altitudeTuningFact.rawValue().toDouble() - navControllerOutput.alt_error);
    _xTrackErrorFact.setRawValue(navControllerOutput.xtrack_error);
    _airSpeedSetpointFact.setRawValue(_airSpeedFact.rawValue().toDouble() - navControllerOutput.aspd_error);
    _distanceToNextWPFact.setRawValue(navControllerOutput.wp_dist);
}

void VehicleFactGroup::_handleVfrHud(const mavlink_message_t &message)
{
    mavlink_vfr_hud_t vfrHud;
    mavlink_msg_vfr_hud_decode(&message, &vfrHud);

    _airSpeedFact.setRawValue(qIsNaN(vfrHud.airspeed) ? 0 : vfrHud.airspeed);
    _groundSpeedFact.setRawValue(qIsNaN(vfrHud.groundspeed) ? 0 : vfrHud.groundspeed);
    _climbRateFact.setRawValue(qIsNaN(vfrHud.climb) ? 0 : vfrHud.climb);
    _throttlePctFact.setRawValue(static_cast<int16_t>(vfrHud.throttle));
    if (qIsNaN(_altitudeTuningOffset)) {
        _altitudeTuningOffset = vfrHud.alt;
    }
    _altitudeTuningFact.setRawValue(vfrHud.alt - _altitudeTuningOffset);
    if (!qIsNaN(vfrHud.groundspeed) && !qIsNaN(_distanceToHomeFact.cookedValue().toDouble())) {
      _timeToHomeFact.setRawValue(_distanceToHomeFact.cookedValue().toDouble() / vfrHud.groundspeed);
    }
}

void VehicleFactGroup::_handleRawImuTemp(const mavlink_message_t &message)
{
    mavlink_raw_imu_t imuRaw;
    mavlink_msg_raw_imu_decode(&message, &imuRaw);

    _imuTempFact.setRawValue(imuRaw.temperature == 0 ? 0 : imuRaw.temperature * 0.01);
}

#ifndef QGC_NO_ARDUPILOT_DIALECT
void VehicleFactGroup::_handleRangefinder(const mavlink_message_t &message)
{

    mavlink_rangefinder_t rangefinder;
    mavlink_msg_rangefinder_decode(&message, &rangefinder);

    _rangeFinderDistFact.setRawValue(qIsNaN(rangefinder.distance) ? 0 : rangefinder.distance);
}
#endif
