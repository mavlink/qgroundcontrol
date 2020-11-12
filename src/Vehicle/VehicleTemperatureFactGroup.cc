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

const char* VehicleTemperatureFactGroup::_temperature1FactName =      "temperature1";
const char* VehicleTemperatureFactGroup::_temperature2FactName =      "temperature2";
const char* VehicleTemperatureFactGroup::_temperature3FactName =      "temperature3";

VehicleTemperatureFactGroup::VehicleTemperatureFactGroup(QObject* parent)
    : FactGroup(1000, ":/json/Vehicle/TemperatureFact.json", parent)
    , _temperature1Fact    (0, _temperature1FactName,     FactMetaData::valueTypeDouble)
    , _temperature2Fact    (0, _temperature2FactName,     FactMetaData::valueTypeDouble)
    , _temperature3Fact    (0, _temperature3FactName,     FactMetaData::valueTypeDouble)
{
    _addFact(&_temperature1Fact,       _temperature1FactName);
    _addFact(&_temperature2Fact,       _temperature2FactName);
    _addFact(&_temperature3Fact,       _temperature3FactName);

    // Start out as not available "--.--"
    _temperature1Fact.setRawValue      (qQNaN());
    _temperature2Fact.setRawValue      (qQNaN());
    _temperature3Fact.setRawValue      (qQNaN());
}

void VehicleTemperatureFactGroup::handleMessage(Vehicle* /* vehicle */, mavlink_message_t& message)
{
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

void VehicleTemperatureFactGroup::_handleHighLatency(mavlink_message_t& message)
{
    mavlink_high_latency_t highLatency;
    mavlink_msg_high_latency_decode(&message, &highLatency);
    temperature1()->setRawValue(highLatency.temperature_air);
    _setTelemetryAvailable(true);
}

void VehicleTemperatureFactGroup::_handleHighLatency2(mavlink_message_t& message)
{
    mavlink_high_latency2_t highLatency2;
    mavlink_msg_high_latency2_decode(&message, &highLatency2);
    temperature1()->setRawValue(highLatency2.temperature_air);
    _setTelemetryAvailable(true);
}

void VehicleTemperatureFactGroup::_handleScaledPressure(mavlink_message_t& message)
{
    mavlink_scaled_pressure_t pressure;
    mavlink_msg_scaled_pressure_decode(&message, &pressure);
    temperature1()->setRawValue(pressure.temperature / 100.0);
    _setTelemetryAvailable(true);
}

void VehicleTemperatureFactGroup::_handleScaledPressure2(mavlink_message_t& message)
{
    mavlink_scaled_pressure2_t pressure;
    mavlink_msg_scaled_pressure2_decode(&message, &pressure);
    temperature2()->setRawValue(pressure.temperature / 100.0);
    _setTelemetryAvailable(true);
}

void VehicleTemperatureFactGroup::_handleScaledPressure3(mavlink_message_t& message)
{
    mavlink_scaled_pressure3_t pressure;
    mavlink_msg_scaled_pressure3_decode(&message, &pressure);
    temperature3()->setRawValue(pressure.temperature / 100.0);
    _setTelemetryAvailable(true);
}
