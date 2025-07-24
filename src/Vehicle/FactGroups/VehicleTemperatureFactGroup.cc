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

VehicleTemperatureFactGroup::VehicleTemperatureFactGroup(QObject *parent)
    : FactGroup(1000, QStringLiteral(":/json/Vehicle/TemperatureFact.json"), parent)
{
    _addFact(&_temperature1Fact);
    _addFact(&_temperature2Fact);
    _addFact(&_temperature3Fact);

    _temperature1Fact.setRawValue(qQNaN());
    _temperature2Fact.setRawValue(qQNaN());
    _temperature3Fact.setRawValue(qQNaN());
}

void VehicleTemperatureFactGroup::handleMessage(Vehicle *vehicle, const mavlink_message_t &message)
{
    Q_UNUSED(vehicle);

    switch (message.msgid) {
    case MAVLINK_MSG_ID_SCALED_PRESSURE:
        _handleScaledPressure(message);
        break;
    case MAVLINK_MSG_ID_SCALED_PRESSURE2:
        _handleScaledPressure2(message);
        break;
    case MAVLINK_MSG_ID_SCALED_PRESSURE3:
        _handleScaledPressure3(message);
        break;
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

void VehicleTemperatureFactGroup::_handleHighLatency(const mavlink_message_t &message)
{
    mavlink_high_latency_t highLatency{};
    mavlink_msg_high_latency_decode(&message, &highLatency);

    temperature1()->setRawValue(highLatency.temperature_air);

    _setTelemetryAvailable(true);
}

void VehicleTemperatureFactGroup::_handleHighLatency2(const mavlink_message_t &message)
{
    mavlink_high_latency2_t highLatency2{};
    mavlink_msg_high_latency2_decode(&message, &highLatency2);

    temperature1()->setRawValue(highLatency2.temperature_air);

    _setTelemetryAvailable(true);
}

void VehicleTemperatureFactGroup::_handleScaledPressure(const mavlink_message_t &message)
{
    mavlink_scaled_pressure_t pressure{};
    mavlink_msg_scaled_pressure_decode(&message, &pressure);

    temperature1()->setRawValue(pressure.temperature / 100.0);

    _setTelemetryAvailable(true);
}

void VehicleTemperatureFactGroup::_handleScaledPressure2(const mavlink_message_t &message)
{
    mavlink_scaled_pressure2_t pressure{};
    mavlink_msg_scaled_pressure2_decode(&message, &pressure);

    temperature2()->setRawValue(pressure.temperature / 100.0);

    _setTelemetryAvailable(true);
}

void VehicleTemperatureFactGroup::_handleScaledPressure3(const mavlink_message_t &message)
{
    mavlink_scaled_pressure3_t pressure{};
    mavlink_msg_scaled_pressure3_decode(&message, &pressure);

    temperature3()->setRawValue(pressure.temperature / 100.0);

    _setTelemetryAvailable(true);
}
