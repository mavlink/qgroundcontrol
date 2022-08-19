#include "EnvgoFactGroup.h"

const char* EnvgoFactGroup::_curTemperatureFactName =       "curTemperature";
const char* EnvgoFactGroup::_motorTemperatureFactName =     "motorTemperature";
const char* EnvgoFactGroup::_motorCtrlTemperatureFactName = "motorCtrlTemperature";
const char* EnvgoFactGroup::_batteryTemperatureFactName =   "batteryTemperature";
const char* EnvgoFactGroup::_servoTemperatureFactName =     "servoTemperature";

// const char* EnvgoFactGroup::_simultaneousSpeedFactName =    "simultaneousSpeed";
const char* EnvgoFactGroup::_averageSpeedFactName =         "averageSpeed";

const char* EnvgoFactGroup::_travelDirectionFactName =      "travelDirection";
const char* EnvgoFactGroup::_distanceTravelledFactName =    "distanceTravelled";
const char* EnvgoFactGroup::_remainBatteryFactName =        "remainBattery";
const char* EnvgoFactGroup::_timeTravelledFactName =        "timeTravelled";
const char* EnvgoFactGroup::_heightAboveWaterFactName =     "heightAboveWater";
const char* EnvgoFactGroup::_flightModeFactName =           "flightMode";
const char* EnvgoFactGroup::_gearFactName =                 "gear";


EnvgoFactGroup::EnvgoFactGroup(QObject* parent)
    : FactGroup(1000, ":/json/Vehicle/EnvgoFactGroup.json", parent)

    , _curTemperatureFact    (0, _curTemperatureFactName,         FactMetaData::valueTypeFloat)
    , _motorTemperatureFact    (0, _motorTemperatureFactName,       FactMetaData::valueTypeFloat)
    , _motorCtrlTemperatureFact    (0, _motorCtrlTemperatureFactName,   FactMetaData::valueTypeFloat)
    , _batteryTemperatureFact    (0, _batteryTemperatureFactName,     FactMetaData::valueTypeFloat)
    , _servoTemperatureFact    (0, _servoTemperatureFactName,       FactMetaData::valueTypeFloat)
    
    // , _temperature2Fact    (0, _simultaneousSpeedFactName,      FactMetaData::valueTypeDouble)
    , _averageSpeedFact    (0, _averageSpeedFactName,           FactMetaData::valueTypeDouble)
    
    , _travelDirectionFact    (0, _travelDirectionFactName,        FactMetaData::valueTypeDouble)
    , _distanceTravelledFact    (0, _distanceTravelledFactName,      FactMetaData::valueTypeDouble)
    , _remainBatteryFact    (0, _remainBatteryFactName,          FactMetaData::valueTypeDouble)
    , _timeTravelledFact    (0, _timeTravelledFactName,          FactMetaData::valueTypeElapsedTimeInSeconds)
    , _heightAboveWaterFact    (0, _heightAboveWaterFactName,       FactMetaData::valueTypeFloat)
    , _flightModeFact    (0, _flightModeFactName,             FactMetaData::valueTypeString)
    , _gearFactName    (0, _gearFactName,                   FactMetaData::valueTypeString)
{
    _addFact(&_curTemperatureFact,          _curTemperatureFactName);
    _addFact(&_motorTemperatureFact,        _motorTemperatureFactName);
    _addFact(&_motorCtrlTemperatureFact,    _motorCtrlTemperatureFactName);
    _addFact(&_batteryTemperatureFact,      _batteryTemperatureFactName);
    _addFact(&_servoTemperatureFact,        _servoTemperatureFactName);
    
    // _addFact(&_simultaneousSpeedFact,       _simultaneousSpeedFactName);
    _addFact(&_averageSpeedFact,            _averageSpeedFactName);
    
    _addFact(&_travelDirectionFact,         _travelDirectionFactName);
    _addFact(&_distanceTravelledFact,       _distanceTravelledFactName);
    _addFact(&_remainBatteryFact,           _remainBatteryFactName);
    _addFact(&_timeTravelledFact,           _timeTravelledFactName);
    _addFact(&_heightAboveWaterFact,        _heightAboveWaterFactName);
    _addFact(&_flightModeFact,              _flightModeFactName);
    _addFact(&_gearFact,                    _gearFactName);
}

// how to interpret message and set values accordingly?
// void VehicleTemperatureFactGroup::handleMessage(Vehicle* /* vehicle */, mavlink_message_t& message)
/*
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
*/
