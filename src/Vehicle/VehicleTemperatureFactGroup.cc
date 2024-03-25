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
/* TD test */
const char* VehicleTemperatureFactGroup::_temperature4FactName =      "temperature4";
const char* VehicleTemperatureFactGroup::_timeUAVMillisecondsFactName =      "UAVMilliseconds";
const char* VehicleTemperatureFactGroup::_timeUnixMillisecondsFactName =      "timeUnixMilliseconds";
const char* VehicleTemperatureFactGroup::_timeUAVBootMillisecondsFactName =      "timeUAVBootMilliseconds";
const char* VehicleTemperatureFactGroup::_altitudeMillimetersMSLFactName =      "altitudeMillimetersMSL";
const char* VehicleTemperatureFactGroup::_absolutePressureMillibarsFactName =      "absolutePressureMillibars";
const char* VehicleTemperatureFactGroup::_temperature0KelvinFactName =      "temperature0Kelvin";
const char* VehicleTemperatureFactGroup::_temperature1KelvinFactName =      "temperature1Kelvin";
const char* VehicleTemperatureFactGroup::_temperature2KelvinFactName =      "temperature2Kelvin";
const char* VehicleTemperatureFactGroup::_relativeHumidityFactName =      "relativeHumidity";
const char* VehicleTemperatureFactGroup::_relativeHumidity0FactName =      "relativeHumidity0";
const char* VehicleTemperatureFactGroup::_relativeHumidity1FactName =      "relativeHumidity1";
const char* VehicleTemperatureFactGroup::_relativeHumidity2FactName =      "relativeHumidity2";
const char* VehicleTemperatureFactGroup::_windSpeedMetersPerSecondFactName =      "windSpeedMetersPerSecond";
const char* VehicleTemperatureFactGroup::_windBearingDegreesFactName =      "windBearingDegrees";
const char* VehicleTemperatureFactGroup::_latitudeDegreesE7FactName =      "latitudeDegreesE7";
const char* VehicleTemperatureFactGroup::_longitudeDegreesE7FactName =      "longitudeDegreesE7";
const char* VehicleTemperatureFactGroup::_rollRadiansFactName =      "rollRadians";
const char* VehicleTemperatureFactGroup::_pitchRadiansFactName =      "pitchRadians";
const char* VehicleTemperatureFactGroup::_yawRadiansFactName =      "yawRadiansFact";
const char* VehicleTemperatureFactGroup::_rollRateRadiansPerSecondFactName =      "rollRateRadiansPerSecond";
const char* VehicleTemperatureFactGroup::_pitchRateRadiansPerSecondFactName =      "pitchRateRadiansPerSecond";
const char* VehicleTemperatureFactGroup::_yawRateRadiansPerSecondFactName =      "yawRateRadiansPerSecond";
const char* VehicleTemperatureFactGroup::_zVelocityMetersPerSecondInvertedFactName =      "zVelocityMetersPerSecondInverted";
const char* VehicleTemperatureFactGroup::_xVelocityMetersPerSecondFactName =      "xVelocityMetersPerSecond";
const char* VehicleTemperatureFactGroup::_yVelocityMetersPerSecondFactName =      "yVelocityMetersPerSecond";
const char* VehicleTemperatureFactGroup::_groundSpeedMetersPerSecondFactName =      "groundSpeedMetersPerSecond";
const char* VehicleTemperatureFactGroup::_heartBeatCustomModeFactName =      "heartBeatCustomMode";
const char* VehicleTemperatureFactGroup::_ascendingFactName =      "ascending";
const char* VehicleTemperatureFactGroup::_timeUAVSecondsFactName =      "timeUAVSeconds";
const char* VehicleTemperatureFactGroup::_timeUnixSecondsFactName =      "timeUnixSeconds";
const char* VehicleTemperatureFactGroup::_timeUAVBootSecondsFactName =      "timeUAVBootSeconds";
const char* VehicleTemperatureFactGroup::_altitudeMetersMSLFactName =      "altitudeMetersMSL";
const char* VehicleTemperatureFactGroup::_temperatureCelsiusFactName =      "temperatureCelsius";
const char* VehicleTemperatureFactGroup::_latitudeDegreesFactName =      "latitudeDegrees";
const char* VehicleTemperatureFactGroup::_longitudeDegreesFactName =      "longitudeDegrees";
const char* VehicleTemperatureFactGroup::_rollDegreesFactName =      "rollDegrees";
const char* VehicleTemperatureFactGroup::_pitchDegreesFactName =      "pitchDegrees";
const char* VehicleTemperatureFactGroup::_yawDegreesFactName =      "yawDegrees";
const char* VehicleTemperatureFactGroup::_rollRateDegreesPerSecondFactName =      "rollRateDegreesPerSecond";
const char* VehicleTemperatureFactGroup::_pitchRateDegreesPerSecondFactName =      "pitchRateDegreesPerSecond";
const char* VehicleTemperatureFactGroup::_yawRateDegreesPerSecondFactName =      "yawRateDegreesPerSecond";
const char* VehicleTemperatureFactGroup::_zVelocityMetersPerSecondFactName =      "zVelocityMetersPerSecond";


VehicleTemperatureFactGroup::VehicleTemperatureFactGroup(QObject* parent)
    : FactGroup(1000, ":/json/Vehicle/TemperatureFact.json", parent)
    , _temperature1Fact    (0, _temperature1FactName,     FactMetaData::valueTypeDouble)
    , _temperature2Fact    (0, _temperature2FactName,     FactMetaData::valueTypeDouble)
    , _temperature3Fact    (0, _temperature3FactName,     FactMetaData::valueTypeDouble)
    /* TD test */
    , _temperature4Fact    (0, _temperature4FactName,     FactMetaData::valueTypeDouble)
    , _timeUAVMillisecondsFact    (0, _timeUAVMillisecondsFactName,     FactMetaData::valueTypeUint32)
    , _timeUnixMillisecondsFact    (0, _timeUnixMillisecondsFactName,     FactMetaData::valueTypeUint64)
    , _timeUAVBootMillisecondsFact    (0, _timeUAVBootMillisecondsFactName,     FactMetaData::valueTypeUint64)
    , _altitudeMillimetersMSLFact    (0, _altitudeMillimetersMSLFactName,     FactMetaData::valueTypeInt32)
    , _absolutePressureMillibarsFact    (0, _absolutePressureMillibarsFactName,     FactMetaData::valueTypeFloat)
    , _temperature0KelvinFact    (0, _temperature0KelvinFactName,     FactMetaData::valueTypeFloat)
    , _temperature1KelvinFact    (0, _temperature1KelvinFactName,     FactMetaData::valueTypeFloat)
    , _temperature2KelvinFact    (0, _temperature2KelvinFactName,     FactMetaData::valueTypeFloat)
    , _relativeHumidityFact    (0, _relativeHumidityFactName,     FactMetaData::valueTypeFloat)
    , _relativeHumidity0Fact    (0, _relativeHumidity0FactName,     FactMetaData::valueTypeFloat)
    , _relativeHumidity1Fact    (0, _relativeHumidity1FactName,     FactMetaData::valueTypeFloat)
    , _relativeHumidity2Fact    (0, _relativeHumidity2FactName,     FactMetaData::valueTypeFloat)
    , _windSpeedMetersPerSecondFact    (0, _windSpeedMetersPerSecondFactName,     FactMetaData::valueTypeFloat)
    , _windBearingDegreesFact    (0, _windBearingDegreesFactName,     FactMetaData::valueTypeFloat)
    , _latitudeDegreesE7Fact    (0, _latitudeDegreesE7FactName,     FactMetaData::valueTypeInt32)
    , _longitudeDegreesE7Fact    (0, _longitudeDegreesE7FactName,     FactMetaData::valueTypeInt32)
    , _rollRadiansFact    (0, _rollRadiansFactName,     FactMetaData::valueTypeFloat)
    , _pitchRadiansFact    (0, _pitchRadiansFactName,     FactMetaData::valueTypeFloat)
    , _yawRadiansFact    (0, _yawRadiansFactName,     FactMetaData::valueTypeFloat)
    , _rollRateRadiansPerSecondFact    (0, _rollRateRadiansPerSecondFactName,     FactMetaData::valueTypeFloat)
    , _pitchRateRadiansPerSecondFact    (0, _pitchRateRadiansPerSecondFactName,     FactMetaData::valueTypeFloat)
    , _yawRateRadiansPerSecondFact    (0, _yawRateRadiansPerSecondFactName,     FactMetaData::valueTypeFloat)
    , _zVelocityMetersPerSecondInvertedFact    (0, _zVelocityMetersPerSecondInvertedFactName,     FactMetaData::valueTypeFloat)
    , _xVelocityMetersPerSecondFact    (0, _xVelocityMetersPerSecondFactName,     FactMetaData::valueTypeFloat)
    , _yVelocityMetersPerSecondFact    (0, _yVelocityMetersPerSecondFactName,     FactMetaData::valueTypeFloat)
    , _groundSpeedMetersPerSecondFact    (0, _groundSpeedMetersPerSecondFactName,     FactMetaData::valueTypeFloat)
    , _heartBeatCustomModeFact    (0, _heartBeatCustomModeFactName,     FactMetaData::valueTypeUint32)
    , _ascendingFact    (0, _ascendingFactName,     FactMetaData::valueTypeBool)
    , _timeUAVSecondsFact    (0, _timeUAVSecondsFactName,     FactMetaData::valueTypeDouble)
    , _timeUnixSecondsFact    (0, _timeUnixSecondsFactName,     FactMetaData::valueTypeDouble)
    , _timeUAVBootSecondsFact    (0, _timeUAVBootSecondsFactName,     FactMetaData::valueTypeDouble)
    , _altitudeMetersMSLFact    (0, _altitudeMetersMSLFactName,     FactMetaData::valueTypeDouble)
    , _temperatureCelsiusFact    (0, _temperatureCelsiusFactName,     FactMetaData::valueTypeFloat)
    , _latitudeDegreesFact    (0, _latitudeDegreesFactName,     FactMetaData::valueTypeDouble)
    , _longitudeDegreesFact    (0, _longitudeDegreesFactName,     FactMetaData::valueTypeDouble)
    , _rollDegreesFact    (0, _rollDegreesFactName,     FactMetaData::valueTypeFloat)
    , _pitchDegreesFact    (0, _pitchDegreesFactName,     FactMetaData::valueTypeFloat)
    , _yawDegreesFact    (0, _yawDegreesFactName,     FactMetaData::valueTypeFloat)
    , _rollRateDegreesPerSecondFact    (0, _rollRateDegreesPerSecondFactName,     FactMetaData::valueTypeFloat)
    , _pitchRateDegreesPerSecondFact    (0, _pitchRateDegreesPerSecondFactName,     FactMetaData::valueTypeFloat)
    , _yawRateDegreesPerSecondFact    (0, _yawRateDegreesPerSecondFactName,     FactMetaData::valueTypeFloat)
    , _zVelocityMetersPerSecondFact    (0, _zVelocityMetersPerSecondFactName,     FactMetaData::valueTypeFloat)
{
    _addFact(&_temperature1Fact,       _temperature1FactName);
    _addFact(&_temperature2Fact,       _temperature2FactName);
    _addFact(&_temperature3Fact,       _temperature3FactName);
    /* TD test */
    _addFact(&_temperature4Fact,       _temperature4FactName);
    _addFact(&_timeUAVMillisecondsFact,       _timeUAVMillisecondsFactName);
    _addFact(&_timeUnixMillisecondsFact,       _timeUnixMillisecondsFactName);
    _addFact(&_timeUAVBootMillisecondsFact,       _timeUAVBootMillisecondsFactName);
    _addFact(&_altitudeMillimetersMSLFact,       _altitudeMillimetersMSLFactName);
    _addFact(&_absolutePressureMillibarsFact,       _absolutePressureMillibarsFactName);
    _addFact(&_temperature0KelvinFact,       _temperature0KelvinFactName);
    _addFact(&_temperature1KelvinFact,       _temperature1KelvinFactName);
    _addFact(&_temperature2KelvinFact,       _temperature2KelvinFactName);
    _addFact(&_relativeHumidityFact,       _relativeHumidityFactName);
    _addFact(&_relativeHumidity0Fact,       _relativeHumidity0FactName);
    _addFact(&_relativeHumidity1Fact,       _relativeHumidity1FactName);
    _addFact(&_relativeHumidity2Fact,       _relativeHumidity2FactName);
    _addFact(&_windSpeedMetersPerSecondFact,       _windSpeedMetersPerSecondFactName);
    _addFact(&_windBearingDegreesFact,       _windBearingDegreesFactName);
    _addFact(&_latitudeDegreesE7Fact,       _latitudeDegreesE7FactName);
    _addFact(&_longitudeDegreesE7Fact,       _longitudeDegreesE7FactName);
    _addFact(&_rollRadiansFact,       _rollRadiansFactName);
    _addFact(&_pitchRadiansFact,       _pitchRadiansFactName);
    _addFact(&_yawRadiansFact,       _yawRadiansFactName);
    _addFact(&_rollRateRadiansPerSecondFact,       _rollRateRadiansPerSecondFactName);
    _addFact(&_pitchRateRadiansPerSecondFact,       _pitchRateRadiansPerSecondFactName);
    _addFact(&_yawRateRadiansPerSecondFact,       _yawRateRadiansPerSecondFactName);
    _addFact(&_zVelocityMetersPerSecondInvertedFact,       _zVelocityMetersPerSecondInvertedFactName);
    _addFact(&_xVelocityMetersPerSecondFact,       _xVelocityMetersPerSecondFactName);
    _addFact(&_yVelocityMetersPerSecondFact,       _yVelocityMetersPerSecondFactName);
    _addFact(&_groundSpeedMetersPerSecondFact,       _groundSpeedMetersPerSecondFactName);
    _addFact(&_heartBeatCustomModeFact,       _heartBeatCustomModeFactName);
    _addFact(&_ascendingFact,       _ascendingFactName);
    _addFact(&_timeUAVSecondsFact,       _timeUAVSecondsFactName);
    _addFact(&_timeUnixSecondsFact,       _timeUnixSecondsFactName);
    _addFact(&_timeUAVBootSecondsFact,       _timeUAVBootSecondsFactName);
    _addFact(&_altitudeMetersMSLFact,       _altitudeMetersMSLFactName);
    _addFact(&_temperatureCelsiusFact,       _temperatureCelsiusFactName);
    _addFact(&_latitudeDegreesFact,       _latitudeDegreesFactName);
    _addFact(&_longitudeDegreesFact,       _longitudeDegreesFactName);
    _addFact(&_rollDegreesFact,       _rollDegreesFactName);
    _addFact(&_pitchDegreesFact,       _pitchDegreesFactName);
    _addFact(&_yawDegreesFact,       _yawDegreesFactName);
    _addFact(&_rollRateDegreesPerSecondFact,       _rollRateDegreesPerSecondFactName);
    _addFact(&_pitchRateDegreesPerSecondFact,       _pitchRateDegreesPerSecondFactName);
    _addFact(&_yawRateDegreesPerSecondFact,       _yawRateDegreesPerSecondFactName);
    _addFact(&_zVelocityMetersPerSecondFact,       _zVelocityMetersPerSecondFactName);

    // Start out as not available "--.--"
    _temperature1Fact.setRawValue      (qQNaN());
    _temperature2Fact.setRawValue      (qQNaN());
    _temperature3Fact.setRawValue      (qQNaN());
    /* TD test */
    _temperature4Fact.setRawValue      (qQNaN());
    _timeUAVMillisecondsFact.setRawValue      (std::numeric_limits<unsigned int>::quiet_NaN());
    _timeUnixMillisecondsFact.setRawValue      (std::numeric_limits<unsigned int>::quiet_NaN());
    _timeUAVBootMillisecondsFact.setRawValue      (std::numeric_limits<unsigned int>::quiet_NaN());
    _altitudeMillimetersMSLFact.setRawValue      (std::numeric_limits<signed int>::quiet_NaN());
    _absolutePressureMillibarsFact.setRawValue      (qQNaN());
    _temperature0KelvinFact.setRawValue      (qQNaN());
    _temperature1KelvinFact.setRawValue      (qQNaN());
    _temperature2KelvinFact.setRawValue      (qQNaN());
    _relativeHumidityFact.setRawValue      (qQNaN());
    _relativeHumidity0Fact.setRawValue      (qQNaN());
    _relativeHumidity1Fact.setRawValue      (qQNaN());
    _relativeHumidity2Fact.setRawValue      (qQNaN());
    _windSpeedMetersPerSecondFact.setRawValue      (qQNaN());
    _windBearingDegreesFact.setRawValue      (qQNaN());
    _latitudeDegreesE7Fact.setRawValue      (std::numeric_limits<signed int>::quiet_NaN());
    _longitudeDegreesE7Fact.setRawValue      (std::numeric_limits<signed int>::quiet_NaN());
    _rollRadiansFact.setRawValue      (qQNaN());
    _pitchRadiansFact.setRawValue      (qQNaN());
    _yawRadiansFact.setRawValue      (qQNaN());
    _rollRateRadiansPerSecondFact.setRawValue      (qQNaN());
    _pitchRateRadiansPerSecondFact.setRawValue      (qQNaN());
    _yawRateRadiansPerSecondFact.setRawValue      (qQNaN());
    _zVelocityMetersPerSecondInvertedFact.setRawValue      (qQNaN());
    _xVelocityMetersPerSecondFact.setRawValue      (qQNaN());
    _yVelocityMetersPerSecondFact.setRawValue      (qQNaN());
    _groundSpeedMetersPerSecondFact.setRawValue      (qQNaN());
    _heartBeatCustomModeFact.setRawValue      (std::numeric_limits<unsigned int>::quiet_NaN());
    _ascendingFact.setRawValue      (false); /* not sure what this should default to */
    _timeUAVSecondsFact.setRawValue      (qQNaN());
    _timeUnixSecondsFact.setRawValue      (qQNaN());
    _timeUAVBootSecondsFact.setRawValue      (qQNaN());
    _altitudeMetersMSLFact.setRawValue      (qQNaN());
    _temperatureCelsiusFact.setRawValue      (qQNaN());
    _latitudeDegreesFact.setRawValue      (qQNaN());
    _longitudeDegreesFact.setRawValue      (qQNaN());
    _rollDegreesFact.setRawValue      (qQNaN());
    _pitchDegreesFact.setRawValue      (qQNaN());
    _yawDegreesFact.setRawValue      (qQNaN());
    _rollRateDegreesPerSecondFact.setRawValue      (qQNaN());
    _pitchRateDegreesPerSecondFact.setRawValue      (qQNaN());
    _yawRateDegreesPerSecondFact.setRawValue      (qQNaN());
    _zVelocityMetersPerSecondFact.setRawValue      (qQNaN());


}

void VehicleTemperatureFactGroup::handleMessage(Vehicle* /* vehicle */, mavlink_message_t& message)
{
    /* TD test */
    Fact iMetFactArray[42];
    iMetFactArray[0] = &_timeUAVMillisecondsFact;
    iMetFactArray[1] = &_timeUnixMillisecondsFact;
    iMetFactArray[2] = &_timeUAVBootMillisecondsFact;
    iMetFactArray[3] = &_altitudeMillimetersMSLFact;
    iMetFactArray[4] = &_absolutePressureMillibarsFact;
    iMetFactArray[5] = &_temperature0KelvinFact;
    iMetFactArray[6] = &_temperature1KelvinFact;
    iMetFactArray[7] = &_temperature2KelvinFact;
    iMetFactArray[8] = &_relativeHumidityFact;
    iMetFactArray[9] = &_relativeHumidity0Fact;
    iMetFactArray[10] = &_relativeHumidity1Fact;
    iMetFactArray[11] = &_relativeHumidity2Fact;
    iMetFactArray[12] = &_windSpeedMetersPerSecondFact;
    iMetFactArray[13] = &_windBearingDegreesFact;
    iMetFactArray[14] = &_latitudeDegreesE7Fact;
    iMetFactArray[15] = &_longitudeDegreesE7Fact;
    iMetFactArray[16] = &_rollRadiansFact;
    iMetFactArray[17] = &_pitchRadiansFact;
    iMetFactArray[18] = &_yawRadiansFact;
    iMetFactArray[19] = &_rollRateRadiansPerSecondFact;
    iMetFactArray[20] = &_pitchRateRadiansPerSecondFact;
    iMetFactArray[21] = &_yawRateRadiansPerSecondFact;
    iMetFactArray[22] = &_zVelocityMetersPerSecondInvertedFact;
    iMetFactArray[23] = &_xVelocityMetersPerSecondFact;
    iMetFactArray[24] = &_yVelocityMetersPerSecondFact;
    iMetFactArray[25] = &_groundSpeedMetersPerSecondFact;
    iMetFactArray[26] = &_heartBeatCustomModeFact;
    iMetFactArray[27] = &_ascendingFact;
    iMetFactArray[28] = &_timeUAVSecondsFact;
    iMetFactArray[29] = &_timeUnixSecondsFact;
    iMetFactArray[30] = &_timeUAVBootSecondsFact;
    iMetFactArray[31] = &_altitudeMetersMSLFact;
    iMetFactArray[32] = &_temperatureCelsiusFact;
    iMetFactArray[33] = &_latitudeDegreesFact;
    iMetFactArray[34] = &_longitudeDegreesFact;
    iMetFactArray[35] = &_rollDegreesFact;
    iMetFactArray[36] = &_pitchDegreesFact;
    iMetFactArray[37] = &_yawDegreesFact;
    iMetFactArray[38] = &_rollRateDegreesPerSecondFact;
    iMetFactArray[39] = &_pitchRateDegreesPerSecondFact;
    iMetFactArray[40] = &_yawRateDegreesPerSecondFact;
    iMetFactArray[41] = &_zVelocityMetersPerSecondFact;

    balancer.update(&message, temperature4(), iMetFactArray);

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
