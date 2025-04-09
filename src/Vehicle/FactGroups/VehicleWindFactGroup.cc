/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "VehicleWindFactGroup.h"
#include "Vehicle.h"

#include <QtMath>

VehicleWindFactGroup::VehicleWindFactGroup(QObject *parent)
    : FactGroup(1000, QStringLiteral(":/json/Vehicle/WindFact.json"), parent)
{
    _addFact(&_directionFact);
    _addFact(&_speedFact);
    _addFact(&_verticalSpeedFact);

    _directionFact.setRawValue(qQNaN());
    _speedFact.setRawValue(qQNaN());
    _verticalSpeedFact.setRawValue(qQNaN());
}

void VehicleWindFactGroup::handleMessage(Vehicle *vehicle, const mavlink_message_t &message)
{
    Q_UNUSED(vehicle);

    switch (message.msgid) {
    case MAVLINK_MSG_ID_WIND_COV:
        _handleWindCov(message);
        break;
    case MAVLINK_MSG_ID_HIGH_LATENCY:
        _handleHighLatency(message);
        break;
    case MAVLINK_MSG_ID_HIGH_LATENCY2:
        _handleHighLatency2(message);
        break;
#ifndef QGC_NO_ARDUPILOT_DIALECT
    case MAVLINK_MSG_ID_WIND:
        _handleWind(message);
        break;
#endif
    default:
        break;
    }
}

void VehicleWindFactGroup::_handleHighLatency(const mavlink_message_t &message)
{
    mavlink_high_latency_t highLatency{};
    mavlink_msg_high_latency_decode(&message, &highLatency);

    speed()->setRawValue(static_cast<double>(highLatency.airspeed) / 5.0);

    _setTelemetryAvailable(true);
}

void VehicleWindFactGroup::_handleHighLatency2(const mavlink_message_t &message)
{
    mavlink_high_latency2_t highLatency2{};
    mavlink_msg_high_latency2_decode(&message, &highLatency2);

    direction()->setRawValue(static_cast<double>(highLatency2.wind_heading) * 2.0);
    speed()->setRawValue(static_cast<double>(highLatency2.windspeed) / 5.0);

    _setTelemetryAvailable(true);
}

void VehicleWindFactGroup::_handleWindCov(const mavlink_message_t &message)
{
    mavlink_wind_cov_t wind{};
    mavlink_msg_wind_cov_decode(&message, &wind);

    float windDirection = qRadiansToDegrees(qAtan2(wind.wind_y, wind.wind_x));
    if (windDirection < 0) {
        windDirection += 360;
    }
    direction()->setRawValue(windDirection);

    const float windSpeed = qSqrt(qPow(wind.wind_x, 2) + qPow(wind.wind_y, 2));
    speed()->setRawValue(windSpeed);

    verticalSpeed()->setRawValue(wind.wind_z);

    _setTelemetryAvailable(true);
}

#ifndef QGC_NO_ARDUPILOT_DIALECT
void VehicleWindFactGroup::_handleWind(const mavlink_message_t &message)
{
    mavlink_wind_t wind{};
    mavlink_msg_wind_decode(&message, &wind);

    // We don't want negative wind angles
    float windDirection = wind.direction;
    if (windDirection < 0) {
        windDirection += 360;
    }
    direction()->setRawValue(windDirection);
    speed()->setRawValue(wind.speed);
    verticalSpeed()->setRawValue(wind.speed_z);

    _setTelemetryAvailable(true);
}
#endif
