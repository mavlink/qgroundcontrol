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
#include <fstream>

const char* VehicleTemperatureFactGroup::_temperature1FactName =                        "temperature1";
const char* VehicleTemperatureFactGroup::_temperature2FactName =                        "temperature2";
const char* VehicleTemperatureFactGroup::_temperature3FactName =                        "temperature3";
/* TD test */
const char* VehicleTemperatureFactGroup::_temperature4FactName =                        "temperature4";
// needs to be capped this way so it's compatible with old flight logs
const char* VehicleTemperatureFactGroup::_timeUAVMillisecondsFactName =                 "uAVMilliseconds";
const char* VehicleTemperatureFactGroup::_timeUnixMillisecondsFactName =                "timeUnixMilliseconds";
const char* VehicleTemperatureFactGroup::_timeUAVBootMillisecondsFactName =             "timeUAVBootMilliseconds";
const char* VehicleTemperatureFactGroup::_altitudeMillimetersMSLFactName =              "altitudeMillimetersMSL";
const char* VehicleTemperatureFactGroup::_absolutePressureMillibarsFactName =           "absolutePressureMillibars";
const char* VehicleTemperatureFactGroup::_temperature0KelvinFactName =                  "temperature0Kelvin";
const char* VehicleTemperatureFactGroup::_temperature1KelvinFactName =                  "temperature1Kelvin";
const char* VehicleTemperatureFactGroup::_temperature2KelvinFactName =                  "temperature2Kelvin";
const char* VehicleTemperatureFactGroup::_relativeHumidityFactName =                    "relativeHumidity";
const char* VehicleTemperatureFactGroup::_relativeHumidity0FactName =                   "relativeHumidity0";
const char* VehicleTemperatureFactGroup::_relativeHumidity1FactName =                   "relativeHumidity1";
const char* VehicleTemperatureFactGroup::_relativeHumidity2FactName =                   "relativeHumidity2";
const char* VehicleTemperatureFactGroup::_windSpeedMetersPerSecondFactName =            "windSpeedMetersPerSecond";
const char* VehicleTemperatureFactGroup::_windBearingDegreesFactName =                  "windBearingDegrees";
const char* VehicleTemperatureFactGroup::_latitudeDegreesE7FactName =                   "latitudeDegreesE7";
const char* VehicleTemperatureFactGroup::_longitudeDegreesE7FactName =                  "longitudeDegreesE7";
const char* VehicleTemperatureFactGroup::_rollRadiansFactName =                         "rollRadians";
const char* VehicleTemperatureFactGroup::_pitchRadiansFactName =                        "pitchRadians";
const char* VehicleTemperatureFactGroup::_yawRadiansFactName =                          "yawRadiansFact";
const char* VehicleTemperatureFactGroup::_rollRateRadiansPerSecondFactName =            "rollRateRadiansPerSecond";
const char* VehicleTemperatureFactGroup::_pitchRateRadiansPerSecondFactName =           "pitchRateRadiansPerSecond";
const char* VehicleTemperatureFactGroup::_yawRateRadiansPerSecondFactName =             "yawRateRadiansPerSecond";
const char* VehicleTemperatureFactGroup::_zVelocityMetersPerSecondInvertedFactName =    "zVelocityMetersPerSecondInverted";
const char* VehicleTemperatureFactGroup::_xVelocityMetersPerSecondFactName =            "xVelocityMetersPerSecond";
const char* VehicleTemperatureFactGroup::_yVelocityMetersPerSecondFactName =            "yVelocityMetersPerSecond";
const char* VehicleTemperatureFactGroup::_groundSpeedMetersPerSecondFactName =          "groundSpeedMetersPerSecond";
const char* VehicleTemperatureFactGroup::_heartBeatCustomModeFactName =                 "heartBeatCustomMode";
const char* VehicleTemperatureFactGroup::_ascendingFactName =                           "ascending";
const char* VehicleTemperatureFactGroup::_timeUAVSecondsFactName =                      "timeUAVSeconds";
const char* VehicleTemperatureFactGroup::_timeUnixSecondsFactName =                     "timeUnixSeconds";
const char* VehicleTemperatureFactGroup::_timeUAVBootSecondsFactName =                  "timeUAVBootSeconds";
const char* VehicleTemperatureFactGroup::_altitudeMetersMSLFactName =                   "altitudeMetersMSL";
const char* VehicleTemperatureFactGroup::_temperatureCelsiusFactName =                  "temperatureCelsius";
const char* VehicleTemperatureFactGroup::_latitudeDegreesFactName =                     "latitudeDegrees";
const char* VehicleTemperatureFactGroup::_longitudeDegreesFactName =                    "longitudeDegrees";
const char* VehicleTemperatureFactGroup::_rollDegreesFactName =                         "rollDegrees";
const char* VehicleTemperatureFactGroup::_pitchDegreesFactName =                        "pitchDegrees";
const char* VehicleTemperatureFactGroup::_yawDegreesFactName =                          "yawDegrees";
const char* VehicleTemperatureFactGroup::_rollRateDegreesPerSecondFactName =            "rollRateDegreesPerSecond";
const char* VehicleTemperatureFactGroup::_pitchRateDegreesPerSecondFactName =           "pitchRateDegreesPerSecond";
const char* VehicleTemperatureFactGroup::_yawRateDegreesPerSecondFactName =             "yawRateDegreesPerSecond";
const char* VehicleTemperatureFactGroup::_zVelocityMetersPerSecondFactName =            "zVelocityMetersPerSecond";
const char* VehicleTemperatureFactGroup::_lastStateFactName =                           "lastState";
const char* VehicleTemperatureFactGroup::_ascentsFactName =                             "ascents";
/* Altitude Level message */
const char* VehicleTemperatureFactGroup::_aslFactName =                                 "asl";
const char* VehicleTemperatureFactGroup::_timeFactName =                                "time";
const char* VehicleTemperatureFactGroup::_pressureFactName =                            "pressure";
const char* VehicleTemperatureFactGroup::_airTempFactName =                             "airTemp";
const char* VehicleTemperatureFactGroup::_relHumFactName =                              "relHum";
const char* VehicleTemperatureFactGroup::_windSpeedFactName =                           "windSpeed";
const char* VehicleTemperatureFactGroup::_windDirectionFactName =                       "windDirection";
const char* VehicleTemperatureFactGroup::_latitudeFactName =                            "latitude";
const char* VehicleTemperatureFactGroup::_longitudeFactName =                           "longitude";
const char* VehicleTemperatureFactGroup::_rollFactName =                                "roll";
const char* VehicleTemperatureFactGroup::_rollRateFactName =                            "rollRate";
const char* VehicleTemperatureFactGroup::_pitchFactName =                               "pitch";
const char* VehicleTemperatureFactGroup::_pitchRateFactName =                           "pitchRate";
const char* VehicleTemperatureFactGroup::_yawFactName =                                 "yaw";
const char* VehicleTemperatureFactGroup::_yawRateFactName =                             "yawRate";
const char* VehicleTemperatureFactGroup::_ascentRateFactName =                          "ascentRate";
const char* VehicleTemperatureFactGroup::_speedOverGroundFactName =                     "speedOverGround";
const char* VehicleTemperatureFactGroup::_ALMIsProcessedFactName =                      "ALMIsProcessed";
const char* VehicleTemperatureFactGroup::_updateFactName =                              "update";

VehicleTemperatureFactGroup::VehicleTemperatureFactGroup(QObject* parent)
    : FactGroup(1000, ":/json/Vehicle/TemperatureFact.json", parent, true)
    , _temperature1Fact    (0, _temperature1FactName,     FactMetaData::valueTypeDouble)
    , _temperature2Fact    (0, _temperature2FactName,     FactMetaData::valueTypeDouble)
    , _temperature3Fact    (0, _temperature3FactName,     FactMetaData::valueTypeDouble)
    /* TD test */
    , _temperature4Fact                         (0, _temperature4FactName,                          FactMetaData::valueTypeDouble)
    , _timeUAVMillisecondsFact                  (0, _timeUAVMillisecondsFactName,                   FactMetaData::valueTypeUint32)
    , _timeUnixMillisecondsFact                 (0, _timeUnixMillisecondsFactName,                  FactMetaData::valueTypeUint64)
    , _timeUAVBootMillisecondsFact              (0, _timeUAVBootMillisecondsFactName,               FactMetaData::valueTypeUint64)
    , _altitudeMillimetersMSLFact               (0, _altitudeMillimetersMSLFactName,                FactMetaData::valueTypeInt32)
    , _absolutePressureMillibarsFact            (0, _absolutePressureMillibarsFactName,             FactMetaData::valueTypeFloat)
    , _temperature0KelvinFact                   (0, _temperature0KelvinFactName,                    FactMetaData::valueTypeFloat)
    , _temperature1KelvinFact                   (0, _temperature1KelvinFactName,                    FactMetaData::valueTypeFloat)
    , _temperature2KelvinFact                   (0, _temperature2KelvinFactName,                    FactMetaData::valueTypeFloat)
    , _relativeHumidityFact                     (0, _relativeHumidityFactName,                      FactMetaData::valueTypeFloat)
    , _relativeHumidity0Fact                    (0, _relativeHumidity0FactName,                     FactMetaData::valueTypeFloat)
    , _relativeHumidity1Fact                    (0, _relativeHumidity1FactName,                     FactMetaData::valueTypeFloat)
    , _relativeHumidity2Fact                    (0, _relativeHumidity2FactName,                     FactMetaData::valueTypeFloat)
    , _windSpeedMetersPerSecondFact             (0, _windSpeedMetersPerSecondFactName,              FactMetaData::valueTypeFloat)
    , _windBearingDegreesFact                   (0, _windBearingDegreesFactName,                    FactMetaData::valueTypeFloat)
    , _latitudeDegreesE7Fact                    (0, _latitudeDegreesE7FactName,                     FactMetaData::valueTypeInt32)
    , _longitudeDegreesE7Fact                   (0, _longitudeDegreesE7FactName,                    FactMetaData::valueTypeInt32)
    , _rollRadiansFact                          (0, _rollRadiansFactName,                           FactMetaData::valueTypeFloat)
    , _pitchRadiansFact                         (0, _pitchRadiansFactName,                          FactMetaData::valueTypeFloat)
    , _yawRadiansFact                           (0, _yawRadiansFactName,                            FactMetaData::valueTypeFloat)
    , _rollRateRadiansPerSecondFact             (0, _rollRateRadiansPerSecondFactName,              FactMetaData::valueTypeFloat)
    , _pitchRateRadiansPerSecondFact            (0, _pitchRateRadiansPerSecondFactName,             FactMetaData::valueTypeFloat)
    , _yawRateRadiansPerSecondFact              (0, _yawRateRadiansPerSecondFactName,               FactMetaData::valueTypeFloat)
    , _zVelocityMetersPerSecondInvertedFact     (0, _zVelocityMetersPerSecondInvertedFactName,      FactMetaData::valueTypeFloat)
    , _xVelocityMetersPerSecondFact             (0, _xVelocityMetersPerSecondFactName,              FactMetaData::valueTypeFloat)
    , _yVelocityMetersPerSecondFact             (0, _yVelocityMetersPerSecondFactName,              FactMetaData::valueTypeFloat)
    , _groundSpeedMetersPerSecondFact           (0, _groundSpeedMetersPerSecondFactName,            FactMetaData::valueTypeFloat)
    , _heartBeatCustomModeFact                  (0, _heartBeatCustomModeFactName,                   FactMetaData::valueTypeUint32)
    , _ascendingFact                            (0, _ascendingFactName,                             FactMetaData::valueTypeBool)
    , _timeUAVSecondsFact                       (0, _timeUAVSecondsFactName,                        FactMetaData::valueTypeDouble)
    , _timeUnixSecondsFact                      (0, _timeUnixSecondsFactName,                       FactMetaData::valueTypeDouble)
    , _timeUAVBootSecondsFact                   (0, _timeUAVBootSecondsFactName,                    FactMetaData::valueTypeDouble)
    , _altitudeMetersMSLFact                    (0, _altitudeMetersMSLFactName,                     FactMetaData::valueTypeDouble)
    , _temperatureCelsiusFact                   (0, _temperatureCelsiusFactName,                    FactMetaData::valueTypeFloat)
    , _latitudeDegreesFact                      (0, _latitudeDegreesFactName,                       FactMetaData::valueTypeDouble)
    , _longitudeDegreesFact                     (0, _longitudeDegreesFactName,                      FactMetaData::valueTypeDouble)
    , _rollDegreesFact                          (0, _rollDegreesFactName,                           FactMetaData::valueTypeFloat)
    , _pitchDegreesFact                         (0, _pitchDegreesFactName,                          FactMetaData::valueTypeFloat)
    , _yawDegreesFact                           (0, _yawDegreesFactName,                            FactMetaData::valueTypeFloat)
    , _rollRateDegreesPerSecondFact             (0, _rollRateDegreesPerSecondFactName,              FactMetaData::valueTypeFloat)
    , _pitchRateDegreesPerSecondFact            (0, _pitchRateDegreesPerSecondFactName,             FactMetaData::valueTypeFloat)
    , _yawRateDegreesPerSecondFact              (0, _yawRateDegreesPerSecondFactName,               FactMetaData::valueTypeFloat)
    , _zVelocityMetersPerSecondFact             (0, _zVelocityMetersPerSecondFactName,              FactMetaData::valueTypeFloat)
    , _lastStateFact                            (0, _lastStateFactName,                             FactMetaData::valueTypeBool)
    , _ascentsFact                              (0, _ascentsFactName,                               FactMetaData::valueTypeUint32)
    /* Altitude Level message */
    , _aslFact                                  (0, _aslFactName,                                   FactMetaData::valueTypeFloat)
    , _timeFact                                 (0, _timeFactName,                                  FactMetaData::valueTypeDouble)
    , _pressureFact                             (0, _pressureFactName,                              FactMetaData::valueTypeFloat)
    , _airTempFact                              (0, _airTempFactName,                               FactMetaData::valueTypeFloat)
    , _relHumFact                               (0, _relHumFactName,                                FactMetaData::valueTypeFloat)
    , _windSpeedFact                            (0, _windSpeedFactName,                             FactMetaData::valueTypeFloat)
    , _windDirectionFact                        (0, _windDirectionFactName,                         FactMetaData::valueTypeFloat)
    , _latitudeFact                             (0, _latitudeFactName,                              FactMetaData::valueTypeDouble)
    , _longitudeFact                            (0, _longitudeFactName,                             FactMetaData::valueTypeDouble)
    , _rollFact                                 (0, _rollFactName,                                  FactMetaData::valueTypeFloat)
    , _rollRateFact                             (0, _rollRateFactName,                              FactMetaData::valueTypeFloat)
    , _pitchFact                                (0, _pitchFactName,                                 FactMetaData::valueTypeFloat)
    , _pitchRateFact                            (0, _pitchRateFactName,                             FactMetaData::valueTypeFloat)
    , _yawFact                                  (0, _yawFactName,                                   FactMetaData::valueTypeFloat)
    , _yawRateFact                              (0, _yawRateFactName,                               FactMetaData::valueTypeFloat)
    , _ascentRateFact                           (0, _ascentRateFactName,                            FactMetaData::valueTypeFloat)
    , _speedOverGroundFact                      (0, _speedOverGroundFactName,                       FactMetaData::valueTypeFloat)
    , _ALMIsProcessedFact                       (0, _ALMIsProcessedFactName,                        FactMetaData::valueTypeBool)
    , _updateFact                               (0, _updateFactName,                                FactMetaData::valueTypeInt32)
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
    _addFact(&_lastStateFact,       _lastStateFactName);
    _addFact(&_ascentsFact,       _ascentsFactName);
    /* Altitude Level message */
    _addFact(&_aslFact,       _aslFactName);
    _addFact(&_timeFact,       _timeFactName);
    _addFact(&_pressureFact,       _pressureFactName);
    _addFact(&_airTempFact,       _airTempFactName);
    _addFact(&_relHumFact,       _relHumFactName);
    _addFact(&_windSpeedFact,       _windSpeedFactName);
    _addFact(&_windDirectionFact,       _windDirectionFactName);
    _addFact(&_latitudeFact,       _latitudeFactName);
    _addFact(&_longitudeFact,       _longitudeFactName);
    _addFact(&_rollFact,       _rollFactName);
    _addFact(&_rollRateFact,       _rollRateFactName);
    _addFact(&_pitchFact,       _pitchFactName);
    _addFact(&_pitchRateFact,       _pitchRateFactName);
    _addFact(&_yawFact,       _yawFactName);
    _addFact(&_yawRateFact,       _yawRateFactName);
    _addFact(&_ascentRateFact,       _ascentRateFactName);
    _addFact(&_speedOverGroundFact,       _speedOverGroundFactName);
    _addFact(&_ALMIsProcessedFact,       _ALMIsProcessedFactName);
    _addFact(&_updateFact,       _updateFactName);

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
    _lastStateFact.setRawValue      (false);
    _ascentsFact.setRawValue      (std::numeric_limits<unsigned int>::quiet_NaN());
    /* Altitude Level message */
    _aslFact.setRawValue      (qQNaN());
    _timeFact.setRawValue      (qQNaN());
    _pressureFact.setRawValue      (qQNaN());
    _airTempFact.setRawValue      (qQNaN());
    _relHumFact.setRawValue      (qQNaN());
    _windSpeedFact.setRawValue      (qQNaN());
    _windDirectionFact.setRawValue      (qQNaN());
    _latitudeFact.setRawValue      (qQNaN());
    _longitudeFact.setRawValue      (qQNaN());
    _rollFact.setRawValue      (qQNaN());
    _rollRateFact.setRawValue      (qQNaN());
    _pitchFact.setRawValue      (qQNaN());
    _pitchRateFact.setRawValue      (qQNaN());
    _yawFact.setRawValue      (qQNaN());
    _yawRateFact.setRawValue      (qQNaN());
    _ascentRateFact.setRawValue      (qQNaN());
    _speedOverGroundFact.setRawValue      (qQNaN());
    _ALMIsProcessedFact.setRawValue      (true);
    _updateFact.setRawValue      (std::numeric_limits<signed int>::quiet_NaN());
}

void VehicleTemperatureFactGroup::handleMessage(Vehicle* /* vehicle */, mavlink_message_t& message)
{
    balancer.update(&message, timeUAVMilliseconds(), timeUnixMilliseconds(), timeUAVBootMilliseconds(), altitudeMillimetersMSL(), absolutePressureMillibars(),
                    temperature0Kelvin(), temperature1Kelvin(), temperature2Kelvin(), relativeHumidity(), relativeHumidity0(), relativeHumidity1(), relativeHumidity2(),
                    windSpeedMetersPerSecond(), windBearingDegrees(), latitudeDegreesE7(), longitudeDegreesE7(), rollRadians(), pitchRadians(), yawRadians(),
                    rollRateRadiansPerSecond(), pitchRateRadiansPerSecond(), yawRateRadiansPerSecond(), zVelocityMetersPerSecondInverted(), xVelocityMetersPerSecond(),
                    yVelocityMetersPerSecond(), groundSpeedMetersPerSecond(), heartBeatCustomMode(), ascending(), timeUAVSeconds(), timeUnixSeconds(),
                    timeUAVBootSeconds(), altitudeMetersMSL(), temperatureCelsius(), latitudeDegrees(), longitudeDegrees(), rollDegrees(), pitchDegrees(),
                    yawDegrees(), rollRateDegreesPerSecond(), pitchRateDegreesPerSecond(), yawRateDegreesPerSecond(), zVelocityMetersPerSecond(), lastState(),
                    ascents());

    switch(balancer.updateALM()){
    case DataBalancer::DATA_NOT_INITIALIZED:
        break;
    case DataBalancer::NOT_ASCENDING:
        break;
    case DataBalancer::ALTITUDE_CHANGE_TOO_SMALL:
        break;
    case DataBalancer::SUCCESS:
        balancer.onALMUpdate(asl(), time(), pressure(), airTemp(), relHum(), windSpeed(), windDirection(), latitude(), longitude(),
                             roll(), rollRate(), pitch(), pitchRate(), yaw(), yawRate(), ascentRate(), speedOverGround(), update());
        break;
    }

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
