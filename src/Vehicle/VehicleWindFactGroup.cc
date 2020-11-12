/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "VehicleWindFactGroup.h"
#include "Vehicle.h"

#include <QtMath>

const char* VehicleWindFactGroup::_directionFactName =      "direction";
const char* VehicleWindFactGroup::_speedFactName =          "speed";
const char* VehicleWindFactGroup::_verticalSpeedFactName =  "verticalSpeed";

VehicleWindFactGroup::VehicleWindFactGroup(QObject* parent)
    : FactGroup(1000, ":/json/Vehicle/WindFact.json", parent)
    , _directionFact    (0, _directionFactName,     FactMetaData::valueTypeDouble)
    , _speedFact        (0, _speedFactName,         FactMetaData::valueTypeDouble)
    , _verticalSpeedFact(0, _verticalSpeedFactName, FactMetaData::valueTypeDouble)
{
    _addFact(&_directionFact,       _directionFactName);
    _addFact(&_speedFact,           _speedFactName);
    _addFact(&_verticalSpeedFact,   _verticalSpeedFactName);

    // Start out as not available "--.--"
    _directionFact.setRawValue      (qQNaN());
    _speedFact.setRawValue          (qQNaN());
    _verticalSpeedFact.setRawValue  (qQNaN());
}

void VehicleWindFactGroup::handleMessage(Vehicle* /* vehicle */, mavlink_message_t& message)
{
    switch (message.msgid) {
    case MAVLINK_MSG_ID_WIND_COV:
        _handleWindCov(message);
        break;
#if !defined(NO_ARDUPILOT_DIALECT)
    case MAVLINK_MSG_ID_WIND:
        _handleWind(message);
        break;
#endif
    case MAVLINK_MSG_ID_HIGH_LATENCY:
        _handleHighLatency(message);
        break;
    case MAVLINK_MSG_ID_HIGH_LATENCY2:
        _handleHighLatency2(message);
        break;
    default:
        break;
    }
}

void VehicleWindFactGroup::_handleHighLatency(mavlink_message_t& message)
{
    mavlink_high_latency_t highLatency;
    mavlink_msg_high_latency_decode(&message, &highLatency);
    speed()->setRawValue((double)highLatency.airspeed / 5.0);
    _setTelemetryAvailable(true);
}

void VehicleWindFactGroup::_handleHighLatency2(mavlink_message_t& message)
{
    mavlink_high_latency2_t highLatency2;
    mavlink_msg_high_latency2_decode(&message, &highLatency2);
    direction()->setRawValue((double)highLatency2.wind_heading * 2.0);
    speed()->setRawValue((double)highLatency2.windspeed / 5.0);
    _setTelemetryAvailable(true);
}

void VehicleWindFactGroup::_handleWindCov(mavlink_message_t& message)
{
    mavlink_wind_cov_t wind;
    mavlink_msg_wind_cov_decode(&message, &wind);

    float direction = qRadiansToDegrees(qAtan2(wind.wind_y, wind.wind_x));
    float speed = qSqrt(qPow(wind.wind_x, 2) + qPow(wind.wind_y, 2));

    if (direction < 0) {
        direction += 360;
    }

    this->direction()->setRawValue(direction);
    this->speed()->setRawValue(speed);
    verticalSpeed()->setRawValue(0);
    _setTelemetryAvailable(true);
}

#if !defined(NO_ARDUPILOT_DIALECT)
void VehicleWindFactGroup::_handleWind(mavlink_message_t& message)
{
    mavlink_wind_t wind;
    mavlink_msg_wind_decode(&message, &wind);

    // We don't want negative wind angles
    float direction = wind.direction;
    if (direction < 0) {
        direction += 360;
    }
    this->direction()->setRawValue(direction);
    speed()->setRawValue(wind.speed);
    verticalSpeed()->setRawValue(wind.speed_z);
    _setTelemetryAvailable(true);
}
#endif
